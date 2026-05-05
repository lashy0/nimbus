#include "bme680_nvs.h"

#include "bme680_internal.h"
#include "bsec_interface.h"
#include "esp_log.h"
#include "nvs.h"

static const char* TAG = "bme680_nvs";

#define BSEC_STATE_SAVE_INTERVAL_SEC (15U * 60U)
#define BSEC_STATE_SAVE_INTERVAL_BOOTSTRAP_SEC 60U

#define BSEC_NVS_NAMESPACE "bme680"
#define BSEC_NVS_KEY_STATE "bsec_state"
#define BSEC_NVS_KEY_STATE_LEN "bsec_len"

void bme680_nvs_clear_state(void)
{
    if (!s_ctx.state_persistence_enabled) {
        return;
    }

    nvs_handle_t nvs = 0;
    if (nvs_open(BSEC_NVS_NAMESPACE, NVS_READWRITE, &nvs) != ESP_OK) {
        return;
    }

    nvs_erase_key(nvs, BSEC_NVS_KEY_STATE);
    nvs_erase_key(nvs, BSEC_NVS_KEY_STATE_LEN);
    nvs_commit(nvs);
    nvs_close(nvs);
}

void bme680_nvs_load_state(void)
{
    if (!s_ctx.state_persistence_enabled) {
        return;
    }

    nvs_handle_t nvs = 0;
    if (nvs_open(BSEC_NVS_NAMESPACE, NVS_READONLY, &nvs) != ESP_OK) {
        return;
    }

    uint32_t state_len = 0;
    esp_err_t ret = nvs_get_u32(nvs, BSEC_NVS_KEY_STATE_LEN, &state_len);
    if (ret != ESP_OK || state_len == 0U || state_len > BSEC_MAX_STATE_BLOB_SIZE) {
        nvs_close(nvs);
        return;
    }

    uint8_t state_blob[BSEC_MAX_STATE_BLOB_SIZE] = {0};
    size_t blob_size = state_len;
    ret = nvs_get_blob(nvs, BSEC_NVS_KEY_STATE, state_blob, &blob_size);
    nvs_close(nvs);
    if (ret != ESP_OK || blob_size != state_len) {
        return;
    }

    uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};
    bsec_library_return_t bsec_ret = bsec_set_state(state_blob, state_len, work_buffer, BSEC_MAX_WORKBUFFER_SIZE);
    if (bme680_check_bsec_rslt("bsec_set_state", bsec_ret) == ESP_OK) {
        ESP_LOGI(TAG, "Loaded BSEC state from NVS (%lu bytes)", (unsigned long)state_len);
    }
}

void bme680_nvs_save_state(bool force)
{
    if (!s_ctx.state_persistence_enabled || !s_ctx.initialized) {
        return;
    }

    int64_t now = bme680_now_us();
    bool calibration_ready =
        (s_ctx.iaq_accuracy >= 2U) && s_ctx.last_output.stabilization_done && s_ctx.last_output.run_in_done;
    uint32_t interval_sec = calibration_ready ? BSEC_STATE_SAVE_INTERVAL_SEC : BSEC_STATE_SAVE_INTERVAL_BOOTSTRAP_SEC;
    int64_t interval_us = (int64_t)interval_sec * 1000000LL;
    if (!force && s_ctx.last_state_save_time_us != 0 && (now - s_ctx.last_state_save_time_us) < interval_us) {
        return;
    }

    uint8_t state_blob[BSEC_MAX_STATE_BLOB_SIZE] = {0};
    uint8_t work_buffer[BSEC_MAX_WORKBUFFER_SIZE] = {0};
    uint32_t state_len = BSEC_MAX_STATE_BLOB_SIZE;

    bsec_library_return_t bsec_ret =
        bsec_get_state(0, state_blob, BSEC_MAX_STATE_BLOB_SIZE, work_buffer, BSEC_MAX_WORKBUFFER_SIZE, &state_len);
    if (bme680_check_bsec_rslt("bsec_get_state", bsec_ret) != ESP_OK) {
        return;
    }

    nvs_handle_t nvs = 0;
    esp_err_t ret = nvs_open(BSEC_NVS_NAMESPACE, NVS_READWRITE, &nvs);
    if (ret != ESP_OK) {
        ESP_LOGW(TAG, "Failed to open NVS for BSEC state save: %s", esp_err_to_name(ret));
        return;
    }

    ret = nvs_set_blob(nvs, BSEC_NVS_KEY_STATE, state_blob, state_len);
    if (ret == ESP_OK) {
        ret = nvs_set_u32(nvs, BSEC_NVS_KEY_STATE_LEN, state_len);
    }
    if (ret == ESP_OK) {
        ret = nvs_commit(nvs);
    }

    if (ret == ESP_OK) {
        s_ctx.last_state_save_time_us = now;
    } else {
        ESP_LOGW(TAG, "Failed to persist BSEC state: %s", esp_err_to_name(ret));
    }

    nvs_close(nvs);
}

void bme680_nvs_save_state_on_progress(void)
{
    if (!s_ctx.state_persistence_enabled || !s_ctx.initialized) {
        return;
    }

    bool has_progress = (s_ctx.iaq_accuracy > s_ctx.last_saved_iaq_accuracy) ||
                        (s_ctx.last_output.stabilization_done && !s_ctx.last_saved_stabilization_done) ||
                        (s_ctx.last_output.run_in_done && !s_ctx.last_saved_run_in_done);
    if (!has_progress) {
        return;
    }

    bme680_nvs_save_state(true);
    s_ctx.last_saved_iaq_accuracy = s_ctx.iaq_accuracy;
    s_ctx.last_saved_stabilization_done = s_ctx.last_output.stabilization_done;
    s_ctx.last_saved_run_in_done = s_ctx.last_output.run_in_done;
}
