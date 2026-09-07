#include "ntp.h"
#include "../config.h"

#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_sntp.h"
#include "esp_timer.h"
#include "esp_wifi.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"

#include <atomic>
#include <cstdint>
#include <cstdlib>
#include <cstring>

namespace {

constexpr char TAG[] = "ntp";
constexpr time_t MIN_VALID_EPOCH = 1577836800; // 2020-01-01 00:00:00 UTC

std::atomic_bool s_wifiHasIp{false};
bool s_wifiInitialized{false};
bool s_wifiStarted{false};
bool s_sntpInitialized{false};
std::atomic_bool s_syncPending{false};
std::atomic_bool s_resyncRequired{true};
bool s_timeValid{false};
uint64_t s_nextReconnectAtMs{0};
uint64_t s_nextSyncAttemptAtMs{0};

uint64_t nowMs() {
	return static_cast<uint64_t>(esp_timer_get_time() / 1000ULL);
}

bool copyWifiValue(uint8_t* destination, size_t capacity, const char* source) {
	const size_t length = std::strlen(source);
	if (length >= capacity) {
		return false;
	}

	std::memcpy(destination, source, length);
	destination[length] = 0;
	return true;
}

void wifiEventHandler(void* /*arg*/, esp_event_base_t eventBase, int32_t eventId, void* /*eventData*/) {
	if (eventBase == WIFI_EVENT && eventId == WIFI_EVENT_STA_DISCONNECTED) {
		s_wifiHasIp = false;
		s_resyncRequired = true;
		s_syncPending = false;
		return;
	}

	if (eventBase == IP_EVENT && eventId == IP_EVENT_STA_GOT_IP) {
		s_wifiHasIp = true;
		s_resyncRequired = true;
	}
}

bool initWifi() {
	if (s_wifiInitialized) {
		return true;
	}

	esp_err_t result = nvs_flash_init();
	if (result == ESP_ERR_NVS_NO_FREE_PAGES || result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		result = nvs_flash_erase();
		if (result == ESP_OK) {
			result = nvs_flash_init();
		}
	}
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "NVS init failed: %s", esp_err_to_name(result));
		return false;
	}

	result = esp_netif_init();
	if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
		ESP_LOGE(TAG, "Network interface init failed: %s", esp_err_to_name(result));
		return false;
	}

	result = esp_event_loop_create_default();
	if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
		ESP_LOGE(TAG, "Event loop init failed: %s", esp_err_to_name(result));
		return false;
	}

	if (esp_netif_create_default_wifi_sta() == nullptr) {
		ESP_LOGE(TAG, "Could not create WiFi station interface");
		return false;
	}

	wifi_init_config_t wifiInitConfig = WIFI_INIT_CONFIG_DEFAULT();
	result = esp_wifi_init(&wifiInitConfig);
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "WiFi init failed: %s", esp_err_to_name(result));
		return false;
	}

	result = esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_STA_DISCONNECTED, &wifiEventHandler, nullptr);
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "Could not register WiFi event handler: %s", esp_err_to_name(result));
		return false;
	}

	result = esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifiEventHandler, nullptr);
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "Could not register IP event handler: %s", esp_err_to_name(result));
		return false;
	}

	wifi_config_t wifiConfig{};
	if (!copyWifiValue(wifiConfig.sta.ssid, sizeof(wifiConfig.sta.ssid), WIFI_SSID) ||
		!copyWifiValue(wifiConfig.sta.password, sizeof(wifiConfig.sta.password), WIFI_PASS)) {
		ESP_LOGE(TAG, "WiFi credentials exceed ESP32 limits");
		return false;
	}

	result = esp_wifi_set_mode(WIFI_MODE_STA);
	if (result == ESP_OK) {
		result = esp_wifi_set_config(WIFI_IF_STA, &wifiConfig);
	}
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "WiFi configuration failed: %s", esp_err_to_name(result));
		return false;
	}

	result = esp_wifi_start();
	if (result != ESP_OK) {
		ESP_LOGE(TAG, "WiFi start failed: %s", esp_err_to_name(result));
		return false;
	}

	s_wifiInitialized = true;
	s_wifiStarted = true;
	return true;
}

void requestWifiConnection(uint64_t currentMs) {
	if (!s_wifiStarted || s_wifiHasIp || currentMs < s_nextReconnectAtMs) {
		return;
	}

	const esp_err_t result = esp_wifi_connect();
	if (result != ESP_OK && result != ESP_ERR_WIFI_CONN) {
		ESP_LOGW(TAG, "WiFi connect request failed: %s", esp_err_to_name(result));
	}
	s_nextReconnectAtMs = currentMs + WIFI_RECONNECT_INTERVAL_MS;
}

bool configTzTime() {
	if (setenv("TZ", TIMEZONE, 1) != 0) {
		ESP_LOGE(TAG, "Could not set timezone");
		return false;
	}
	tzset();
	return true;
}

bool startSntp() {
	if (!s_wifiHasIp) {
		return false;
	}

	if (s_sntpInitialized) {
		esp_netif_sntp_deinit();
		s_sntpInitialized = false;
	}

	esp_sntp_config_t sntpConfig = ESP_NETIF_SNTP_DEFAULT_CONFIG(NTP_SERVER);
	sntpConfig.wait_for_sync = true;
	sntpConfig.start = true;

	const esp_err_t result = esp_netif_sntp_init(&sntpConfig);
	if (result != ESP_OK) {
		ESP_LOGW(TAG, "SNTP init failed: %s", esp_err_to_name(result));
		return false;
	}

	s_sntpInitialized = true;
	s_syncPending = true;
	return true;
}

bool readSystemTime(struct tm& timeInfo) {
	time_t currentTime = 0;
	if (time(&currentTime) < MIN_VALID_EPOCH) {
		return false;
	}
	return localtime_r(&currentTime, &timeInfo) != nullptr;
}

bool pollForSync() {
	if (!s_syncPending || !s_sntpInitialized) {
		return s_timeValid;
	}

	if (esp_netif_sntp_sync_wait(0) != ESP_OK) {
		return false;
	}

	struct tm timeInfo{};
	if (!readSystemTime(timeInfo)) {
		return false;
	}

	s_syncPending = false;
	s_resyncRequired = false;
	s_timeValid = true;
	return true;
}

bool getLocalTime(struct tm& timeInfo, uint32_t timeoutMs) {
	const uint64_t deadline = nowMs() + timeoutMs;
	do {
		if (pollForSync() && readSystemTime(timeInfo)) {
			return true;
		}
		if (s_timeValid && readSystemTime(timeInfo)) {
			return true;
		}
		if (timeoutMs == 0u || nowMs() >= deadline) {
			return false;
		}
		vTaskDelay(pdMS_TO_TICKS(NTP_POLL_INTERVAL_MS));
	} while (true);
}

bool waitForWifi(uint32_t timeoutMs) {
	const uint64_t deadline = nowMs() + timeoutMs;
	do {
		requestWifiConnection(nowMs());
		if (s_wifiHasIp) {
			return true;
		}
		if (nowMs() >= deadline) {
			return false;
		}
		vTaskDelay(pdMS_TO_TICKS(NTP_POLL_INTERVAL_MS));
	} while (true);
}

} // namespace

bool ntpBegin() {
	if (!initWifi() || !configTzTime() || !waitForWifi(WIFI_CONNECT_TIMEOUT_MS)) {
		ESP_LOGW(TAG, "WiFi connection unavailable; NTP sync deferred");
		return false;
	}

	s_resyncRequired = true;
	s_nextSyncAttemptAtMs = 0;
	if (!startSntp()) {
		return false;
	}

	struct tm timeInfo{};
	const bool synchronized = getLocalTime(timeInfo, NTP_SYNC_TIMEOUT_MS);
	if (!synchronized) {
		ESP_LOGW(TAG, "NTP sync timed out; keeping background retry active");
	}
	return synchronized;
}

bool ntpGetTime(struct tm& timeInfo) {
	return getLocalTime(timeInfo, 0);
}

void ntpMaintain() {
	if (!s_wifiInitialized) {
		return;
	}

	const uint64_t currentMs = nowMs();
	if (!s_wifiHasIp) {
		requestWifiConnection(currentMs);
		return;
	}

	if (s_syncPending) {
		(void)pollForSync();
		return;
	}

	if (s_resyncRequired && currentMs >= s_nextSyncAttemptAtMs) {
		s_nextSyncAttemptAtMs = currentMs + NTP_RETRY_INTERVAL_MS;
		(void)startSntp();
	}
}
