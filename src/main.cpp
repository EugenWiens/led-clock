#include "Application.h"

extern "C" void app_main() {
    static Application application;
    application.run();
}
