#include "composition.h"

#include "esp_app_desc.h"
#include "esp_idf_version.h"
#include "esp_log.h"

static const char* TAG = "nimbus";

void app_main(void)
{
    ESP_LOGI(TAG, "Nimbus %s, ESP-IDF %s", esp_app_get_description()->version, esp_get_idf_version());

    // No restart on failure: a reset loop would not fix a board-level error.
    const esp_err_t err = composition_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Composition failed: %s", esp_err_to_name(err));
    }
}
