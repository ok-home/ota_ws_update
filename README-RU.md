[En](/README.md)

| Supported Targets |  
| ESP32 ESP32S3 ESP32C3 | 
| ----------------- |

# ESP32 OTA обновление через WebsSocket с простым WEB интерфейсом. Опционально PreEncrypted режим.
 - Подключается как компонент к вашей программе
 - Не требует внешних серверов для хранения прошивок OTA, предназначен в первую очередь для работы в локальной сети.
 - Использует WebsSocket или WebsSocket Secure протокол.
 - Подключается к любому web серверу на esp32, использующему WebSocket протокол, например (esp-idf examples/protocols/http_server/ws_echo_server) или (esp-idf examples/protocols/https_server/wss_server) как URI handler.
   - в зависимости от протокола сервера (http/https) будет выбран протокол обмена (ws/wss)
 - режим PreEncrypted (espressif/esp_encrypted_img) подключается в Menuconfig
   - для PreEncrypted режима требуется увеличение размера стека httpd_config_t config.stack_size = 4096*4;
   - Подробности использования PreEncrypted режима https://components.espressif.com/components/espressif/esp_encrypted_img
 - Пример - example_ota_ws
 - Web интерфейс
   - Выбор файла прошивки
   - Загрузка прошивки в esp32
   - Контроль загрузки прошивки
   - После обновления прошивки - подтверждение обновления или откат на предыдущую версию
 - Выбор URI страницы OTA в Menuconfig
 - Обновление скачивается частями, размер фрагмента закачки в Menuconfig
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
   - PARTITION_TABLE_CUSTOM=y
   - PARTITION_TABLE_CUSTOM_FILENAME="partitions.csv"
   - HTTPD_WS_SUPPORT=y
   - APP_ROLLBACK_ENABLE=y
   - OTA_DEFAULT_URI - адрес web интерфейса OTA
   - OTA_DEFAULT_WS_URI - адрес ws/wss интерфейса OTA
   - OTA_CHUNK_SIZE - размер фрагментов заказчки
   - OTA_PRE_ENCRYPTED_MODE - включение PreEncrypted режима
     - OTA_PRE_ENCRYPTED_RSA_KEY_LOCATION - место хранения rsa_private_key ( в каталоге компонента или в каталоге проекта )
     - OTA_PRE_ENCRYPTED_RSA_KEY_DIRECTORY - директория хранения rsa_private_key
  
