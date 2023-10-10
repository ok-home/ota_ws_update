[Ru](/README-RU.md)

| Supported Targets |  
| ESP32 ESP32S3 ESP32C3 | 
| ----------------- |

# ESP32 OTA update via WebSocket with a simple WEB interface. Optional PreEncrypted mode.
  - Connects as a component to your program
  - Does not require external servers for storing OTA firmware, designed primarily for working on a local network.
  - Uses WebsSocket or WebsSocket Secure protocol.
  - Connects to any web server on esp32 that uses the WebSocket protocol, for example (esp-idf examples/protocols/http_server/ws_echo_server) or (esp-idf examples/protocols/https_server/wss_server) as a URI handler.
    - depending on the server protocol (http/https), the exchange protocol (ws/wss) will be selected
  - PreEncrypted mode (espressif/esp_encrypted_img) is enabled in Menuconfig
    - PreEncrypted mode requires an increase in the stack size httpd_config_t config.stack_size = 4096*4;
    - Details of using PreEncrypted mode https://components.espressif.com/components/espressif/esp_encrypted_img
  - Example - example_ota_ws
  - Web interface
    - Select firmware file
    - Upload firmware to esp32
    - Firmware download control
    - After updating the firmware - confirm the update or roll back to the previous version
  - Select OTA page URI in Menuconfig
  - The update is downloaded in parts, the size of the download fragment is in Menuconfig
  - Connection example
```
#include "ota_ws_update.h" // handler definition

// start webserver from esp-idf "examples/protocols/http_server/ws_echo_server"
static httpd_handle_t start_webserver(void)
{
     httpd_handle_t server = NULL;
     httpd_config_t config = HTTPD_DEFAULT_CONFIG();
     // Start the httpd server
     ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
     if (httpd_start(&server, &config) == ESP_OK) {
         ESP_LOGI(TAG, "Registering URI handlers");
     /****************** Registering the ws handler ****************/
         ota_ws_register_uri_handler(server);
         // end register ota_ws handler
         return server;
     }
     ESP_LOGI(TAG, "Error starting server!");
     return NULL;
}
```
  - Example partitions.csv
```
# Name, Type, SubType, Offset, Size, Flags
nvs, data, nvs, 0x9000, 0x4000
otadata, data, ota, , 0x2000
phy_init, data, phy, , 0x1000
ota_0, app, ota_0, , 1M
ota_1, app, ota_1, , 1M
```
  - menuconfig parameters
    - PARTITION_TABLE_CUSTOM=y
    - PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
    - HTTPD_WS_SUPPORT=y
    - APP_ROLLBACK_ENABLE=y
    - OTA_DEFAULT_URI - OTA web interface address
    - OTA_DEFAULT_WS_URI - ws/wss address of the OTA interface
    - OTA_CHUNK_SIZE - size of order fragments
    - OTA_PRE_ENCRYPTED_MODE - enable PreEncrypted mode
      - OTA_PRE_ENCRYPTED_RSA_KEY_LOCATION - rsa_private_key storage location (in the component directory or in the project directory)
      - OTA_PRE_ENCRYPTED_RSA_KEY_DIRECTORY - rsa_private_key storage directory