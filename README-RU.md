[En](/README.md)

| Supported Targets |  
| ESP32 ESP32S3 ESP32C3 | 
| ----------------- |

# ESP32 OTA обновление через websocket с простым WEB интерфейсом
 - Подключается как компонент к вашей программе
 - Использует websocket протокол
 - Подключается к любому web серверу на esp32, использующему websocket протокол, например (esp-idf examples/protocols/http_server/ws_echo_server)
 - Пример - example_ota_ws
 - Web интерфейс
   - Выбор файла прошивки
   - Загрузка прошивки в esp32
   - Контроль загрузки прошивки
   - После обновления прошивки - подтверждение обновления или откат на предыдущую версию
 - Выбор URI страницы OTA в menuconfig
 - Обновление скачивается частями, размер фрагмента закачки в menuconfig
 - Пример подключения
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
 - Пример partitions.csv
```
# Name,   Type, SubType,  Offset,   Size,  Flags
nvs,      data, nvs,      0x9000,  0x4000
otadata,  data, ota,      ,  0x2000
phy_init, data, phy,      ,  0x1000
ota_0,    app,  ota_0,    ,  1M
ota_1,    app,  ota_1,    ,  1M
```
 - Параметры menuconfig
   - CONFIG_PARTITION_TABLE_CUSTOM=y
   - CONFIG_PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
   - CONFIG_WS_TRANSPORT=y
   - CONFIG_APP_ROLLBACK_ENABLE=y
  
