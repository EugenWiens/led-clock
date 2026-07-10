#include "led_hal.h"
#include "../config.h"

#include "driver/rmt_tx.h"
#include "driver/rmt_encoder.h"
#include "rom/ets_sys.h"       // ets_delay_us
#include "freertos/FreeRTOS.h" // portMAX_DELAY

// ---------------------------------------------------------------------------
// WS2812B timing at 10 MHz RMT clock (1 tick = 100 ns)
// ---------------------------------------------------------------------------
//  T0H = 400 ns → 4 ticks HIGH     T0L = 800 ns → 8 ticks LOW
//  T1H = 800 ns → 8 ticks HIGH     T1L = 400 ns → 4 ticks LOW
//  Reset: data line LOW for > 50 µs (handled by ets_delay_us after TX)
// ---------------------------------------------------------------------------
static constexpr uint32_t RMT_RESOLUTION_HZ = 10'000'000; // 10 MHz

void EspLedStripHal::init(CRGB* leds, uint16_t count) {
    m_leds  = leds;
    m_count = count;

    // --- RMT TX channel ---
    rmt_tx_channel_config_t tx_cfg{};
    tx_cfg.gpio_num          = static_cast<gpio_num_t>(LED_DATA_PIN);
    tx_cfg.clk_src           = RMT_CLK_SRC_DEFAULT;
    tx_cfg.resolution_hz     = RMT_RESOLUTION_HZ;
    tx_cfg.mem_block_symbols = 64;
    tx_cfg.trans_queue_depth = 4;
    rmt_new_tx_channel(&tx_cfg, &m_txChan);

    // --- Bytes encoder: WS2812B bit-level timing (10 MHz → 1 tick = 100 ns) ---
    // bit0: HIGH 400 ns (4 ticks), LOW 800 ns (8 ticks)
    // bit1: HIGH 800 ns (8 ticks), LOW 400 ns (4 ticks)
    rmt_bytes_encoder_config_t enc_cfg{};
    enc_cfg.bit0.duration0 = 4;  enc_cfg.bit0.level0 = 1;
    enc_cfg.bit0.duration1 = 8;  enc_cfg.bit0.level1 = 0;
    enc_cfg.bit1.duration0 = 8;  enc_cfg.bit1.level0 = 1;
    enc_cfg.bit1.duration1 = 4;  enc_cfg.bit1.level1 = 0;
    enc_cfg.flags.msb_first = 1; // WS2812B sends MSB first
    rmt_new_bytes_encoder(&enc_cfg, &m_bytesEnc);

    rmt_enable(m_txChan);
}

void EspLedStripHal::show() {
    // Build GRB buffer (WS2812B uses G-R-B byte order), scaled by brightness.
    static uint8_t s_grb[LED_COUNT * 3];
    for (uint16_t i = 0; i < m_count; ++i) {
        s_grb[i * 3 + 0] = static_cast<uint8_t>(m_leds[i].g * m_brightness / 255u);
        s_grb[i * 3 + 1] = static_cast<uint8_t>(m_leds[i].r * m_brightness / 255u);
        s_grb[i * 3 + 2] = static_cast<uint8_t>(m_leds[i].b * m_brightness / 255u);
    }

    rmt_transmit_config_t tx_cfg{};
    tx_cfg.loop_count = 0;
    tx_cfg.flags.eot_level = 0;         // line stays LOW after TX → WS2812B reset
    rmt_transmit(m_txChan, m_bytesEnc, s_grb, m_count * 3u, &tx_cfg);
    rmt_tx_wait_all_done(m_txChan, portMAX_DELAY);

    // WS2812B reset: hold data line LOW for > 50 µs.
    // After rmt_tx_wait_all_done() the line is already LOW (eot_level = 0).
    ets_delay_us(60);
}

void EspLedStripHal::setBrightness(uint8_t brightness) {
    m_brightness = brightness;
}
