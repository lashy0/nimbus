#include "esp_app_desc.h"
#include "esp_idf_version.h"
#include "esp_log.h"

static const char* TAG = "nimbus";

void app_main(void)
{
    ESP_LOGI(TAG, "Nimbus %s, ESP-IDF %s", esp_app_get_description()->version, esp_get_idf_version());
}
