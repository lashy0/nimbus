#include "esp_log.h"
#include "esp_system.h"
#include "sdkconfig.h"

#if CONFIG_NIMBUS_BOARD_DIAGNOSTICS
#include "board_diagnostics.h"
#endif

void app_main(void)
{
    ESP_LOGI("nimbus", "Nimbus v2: clean firmware foundation");
    ESP_LOGI("nimbus", "ESP-IDF %s", esp_get_idf_version());
#if CONFIG_NIMBUS_BOARD_DIAGNOSTICS
    board_diagnostics_run();
#else
    ESP_LOGI("nimbus", "Application services are not implemented; enable NIMBUS_BOARD_DIAGNOSTICS for stage 1");
#endif
}
