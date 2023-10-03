/* 
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/* cmake cmd

create_esp_enc_img(${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.bin
    ${project_dir}/rsa_key/private.pem ${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}_secure.bin app)

key cmd
openssl genrsa -out rsa_key/private.pem 3072

*/


#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"
#include "esp_image_format.h"
#include "esp_encrypted_img.h"

#include "ota_ws_update_private.h"

static const char *TAG = "ota_ws_esp";

static const esp_partition_t *update_partition = NULL;
static bool image_header_was_checked = false;
static esp_ota_handle_t update_handle = 0;
// pre-encrypted handle
static esp_decrypt_handle_t enc_handle; // handle
static esp_decrypt_cfg_t enc_cfg = {}; // cfg
static  pre_enc_decrypt_arg_t enc_arg = {}; // arg

extern const char rsa_private_pem_start[] asm("_binary_private_pem_start");
extern const char rsa_private_pem_end[]   asm("_binary_private_pem_end");

esp_err_t start_ota_ws(void)
{
    //return ESP_OK; // debug return

    esp_err_t err;
    ESP_LOGI(TAG, "Starting OTA");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();
    if(configured==NULL || running == NULL)
    {
        ESP_LOGE(TAG,"OTA data not found");
        return ESP_FAIL;
    }

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

    image_header_was_checked = false;

    enc_cfg.rsa_priv_key = rsa_private_pem_start;
    enc_cfg.rsa_priv_key_len = rsa_private_pem_end-rsa_private_pem_start;

    enc_handle = esp_encrypted_img_decrypt_start(&enc_cfg);
    if(enc_handle == NULL)
    {
        ESP_LOGE(TAG, "eesp_encrypted_img_decrypt_start failed ");
        abort_ota_ws();
        return ESP_FAIL;
    }
    ESP_LOGI(TAG, "esp_ota_begin succeeded");
    return ESP_OK;
}
esp_err_t write_ota_ws(int enc_data_read, uint8_t *enc_ota_write_data)
{
     //return ESP_OK; // debug return
    enc_arg.data_in = (char*)enc_ota_write_data;
    enc_arg.data_in_len = enc_data_read;
    esp_err_t ret = esp_encrypted_img_decrypt_data(enc_handle, &enc_arg);
    if(ret)
    {
            ESP_LOGE(TAG, "data decrypt err %x",ret);
            abort_ota_ws();
            return ret;
    }
    int data_read = enc_arg.data_out_len;
    uint8_t *ota_write_data = (uint8_t*)enc_arg.data_out;

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
            ESP_LOGE(TAG, "Received package is not fit len");
            abort_ota_ws();
            ret = ESP_FAIL;
            goto _ret_free;
        }
    }
    ret = esp_ota_write(update_handle, (const void *)ota_write_data, data_read);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_ota_write err");
        abort_ota_ws();
        goto _ret_free;
    }
    ret = ESP_OK;

_ret_free:
    free(enc_arg.data_out);
    return ret;
}
esp_err_t end_ota_ws(void)
{
        //return ESP_OK; // debug return
    esp_err_t ret = esp_encrypted_img_decrypt_end(enc_handle);
    if(ret)  
    {
        ESP_LOGE(TAG, "esp_encrypted_img_decrypt_end (%s)!", esp_err_to_name(ret));
        abort_ota_ws();
        return ret;
    }

    ret = esp_ota_end(update_handle);
    if (ret != ESP_OK) {
        if (ret == ESP_ERR_OTA_VALIDATE_FAILED) {
            ESP_LOGE(TAG, "Image validation failed, image is corrupted");
            abort_ota_ws();
            return ret;
        }
        ESP_LOGE(TAG, "esp_ota_end failed (%s)!", esp_err_to_name(ret));
        abort_ota_ws();
        return ret;
    }
    ret = esp_ota_set_boot_partition(update_partition);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(ret));
        abort_ota_ws();
        return ret;
    }
    return  ESP_OK;
}
esp_err_t abort_ota_ws(void)
{
    esp_err_t ret = esp_encrypted_img_decrypt_abort(enc_handle);
    if(ret)
     {return ret;}
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
// rollback == false - app valid? confirm update -> no rollback
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