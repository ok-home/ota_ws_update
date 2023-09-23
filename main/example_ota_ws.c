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

//#include <esp_log.h>

#include "prv_wifi_connect.h"
static const char *TAG = "ota_ws";

#define MDNS
#ifdef MDNS
#include "mdns.h"
#include "lwip/apps/netbiosns.h"
#endif //MDNS
#ifdef MDNS
static void initialise_mdns(void)
{
    mdns_init();
    mdns_hostname_set("esp");
    mdns_instance_name_set("esp home web server");

    mdns_txt_item_t serviceTxtData[] = {
        {"board", "esp32"},
        {"path", "/"}
    };

    ESP_ERROR_CHECK(mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData,
                                     sizeof(serviceTxtData) / sizeof(serviceTxtData[0])));
}
#endif // MDNS


void app_main(void)
{
    prv_wifi_connect();                     // return with error ?

#ifdef MDNS
    initialise_mdns();
    netbiosns_init();
    netbiosns_set_name("esp");
#endif // MDNS

    prv_start_http_server(PRV_MODE_STAY_ACTIVE,NULL); // run server
    //example_echo_ws_server();
}
