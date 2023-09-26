[Ru](/README-RU.md)

| Supported Targets |  
| ESP32 ESP32S3 ESP32C3 | 
| ----------------- |

# ESP32 OTA update via websocket with a simple WEB interface
  - Connects as a component to your program
  - Uses websocket protocol
  - Connects to any web server on esp32 that uses the websocket protocol, for example (esp-idf examples/protocols/http_server/ws_echo_server)
  - Example - example_ota_ws
  - Web interface
    - Select firmware file
    - Upload firmware to esp32
    - Firmware download control
    - After updating the firmware - confirm the update or roll back to the previous version
  - Select OTA page URI in menuconfig
  - The update is downloaded in fragments, the size of the download fragment is in menuconfig
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
    /****************** Registering the ws handler ****************/
        ESP_LOGI(TAG, "Registering URI handlers");
        //register  ota_ws handler
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
# Name,   Type, SubType,  Offset,   Size,  Flags
nvs,      data, nvs,      0x9000,  0x4000
otadata,  data, ota,      ,  0x2000
phy_init, data, phy,      ,  0x1000
ota_0,    app,  ota_0,    ,  1M
ota_1,    app,  ota_1,    ,  1M
```
 - menuconfig options
   - CONFIG_PARTITION_TABLE_CUSTOM=y
   - CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
   - CONFIG_WS_TRANSPORT=y
   - CONFIG_APP_ROLLBACK_ENABLE=y
  
