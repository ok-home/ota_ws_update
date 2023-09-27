/* OTA example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_wifi.h"
#include "esp_http_server.h"

// #include <esp_log.h>

#include "nvs_wifi_connect.h"
#include "ota_ws_update.h"

extern esp_err_t example_register_uri_handler(httpd_handle_t server);
void example_echo_ws_server(void);

//static const char *TAG = "ota_ws";

#define MDNS
#ifdef MDNS
#define HOST_NAME "esp-ota"
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#endif // MDNS
#ifdef MDNS
static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set(HOST_NAME);
    mdns_instance_name_set("esp home web server");

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}};

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
    netbiosns_init();
    netbiosns_set_name(HOST_NAME);
}
#endif // MDNS



void app_main(void)
{
    nvs_wifi_connect(); // return with error ?

#ifdef MDNS
    initialise_mdns();
#endif // MDNS

    example_echo_ws_server();
    //prv_start_http_server(PRV_MODE_STAY_ACTIVE, ota_ws_register_uri_handler); // run server
}
