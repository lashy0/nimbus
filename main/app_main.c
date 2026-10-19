#include "esp_log.h"
#include "esp_system.h"

void app_main(void)
{
    ESP_LOGI("nimbus", "Nimbus v2: clean firmware foundation");
    ESP_LOGI("nimbus", "ESP-IDF %s", esp_get_idf_version());
    ESP_LOGI("nimbus", "Board drivers, UI and application services are not implemented yet");
}
