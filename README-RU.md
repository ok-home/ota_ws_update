[En](/README.md)

| Supported Targets |  
| ESP32 ESP32S3 ESP32C3 | 
| ----------------- |

# ESP32 OTA обновление через websocket с простым WEB интерфейсом
 - Подключается как компонент к вашей программе
 - Использует websocket протокол
 - Подключается к любому web серверу на esp32, использующему websocket протокол, например (examples/protocols/http_server/ws_echo_server)
 - Пример - example_ota_ws
 - Web интерфейс
 -- Выбор файла прошивки
 -- Загрузка прошивки в esp32
 -- Контроль загрузки прошивки
 -- После обновления прошивки - подтверждение обновления или откат на предыдущую версию
 -- выбор URI страницы в menuconfig
 - Обновление скачивается частями, размер фрагмента закачки в menuconfig


``
static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Registering the ws handler
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &ws);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}
``

Функция ```console.log()```
выводит текст в консоль.

Сумму двух переменных
можно вывести так:
```javascript
const a = 1
const b = 2

console.log(a + b)
```
