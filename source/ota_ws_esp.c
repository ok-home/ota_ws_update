
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_image_format.h"

#include "ota_ws_private.h"

static const char *TAG = "ota_ws_esp";
/*an ota data write buffer ready to write to the flash*/
//static char ota_write_data[BUFFSIZE + 1] = {0};
static const esp_partition_t *update_partition = NULL;
static bool image_header_was_checked = false;
static int binary_file_length = 0;
static esp_ota_handle_t update_handle = 0;

esp_err_t start_ota_ws(void)
{
    //return ESP_OK; // debug return

    esp_err_t err;
    ESP_LOGI(TAG, "Starting OTA");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();

    if (configured != running)
    {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08lx, but running from offset 0x%08lx",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08lx)",
             running->type, running->subtype, running->address);

    update_partition = esp_ota_get_next_update_partition(NULL);
    ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%lx",
             update_partition->subtype, update_partition->address);

    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_begin failed ");
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");

    image_header_was_checked = false;
    return ESP_OK;
}

esp_err_t write_ota_ws(int data_read, uint8_t *ota_write_data)
{
    //return ESP_OK; // debug return


    if (image_header_was_checked == false) // first segment
    {
        esp_app_desc_t new_app_info;
        if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t))
        {
            // check current version with downloading
            memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
            ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

            image_header_was_checked = true;
        }
        else
        {
            ESP_LOGE(TAG, "received package is not fit len");
            return ESP_FAIL;
        }
    }
    esp_err_t err = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
    if (err != ESP_OK)
    {
        return ESP_FAIL;
    }
    binary_file_length += data_read;
    ESP_LOGD(TAG, "Written image length %d", binary_file_length);
    return ESP_OK;
}

esp_err_t end_ota_ws(void)
{
        //return ESP_OK; // debug return

    esp_err_t err = esp_ota_end(update_handle);
    if (err != ESP_OK) {
        if (err == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
        }
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(err));
        return ESP_FAIL;
    }
    err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
        return ESP_FAIL;
    }
    return  ESP_OK;
}
esp_err_t abort_ota_ws(void)
{
    return esp_ota_abort(update_handle);
}
// false - rollback disable
// true - rollback enable
bool check_ota_ws_rollback_enable(void)
{
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    esp_ota_img_states_t ota_state_running_part;
    const esp_partition_t *running = esp_ota_get_running_partition();
    if (esp_ota_get_state_partition(running, &ota_state_running_part) == ESP_OK) {
        if (ota_state_running_part == ESP_OTA_IMG_PENDING_VERIFY) {
            ESP_LOGI(TAG, "Running app has ESP_OTA_IMG_PENDING_VERIFY state");
            return true;
        }
    }
#endif
    return false;    
}
// rollback == true - rollback
// rollback == false - app valid? no rollback
esp_err_t rollback_ota_ws(bool rollback)
{
#ifdef CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    if(rollback == false)
    {
        return esp_ota_mark_app_valid_cancel_rollback(); // app valid
    }
    else
    {
        return esp_ota_mark_app_invalid_rollback_and_reboot(); // app rolback & reboot
    }
#endif
    return ESP_FAIL;
}