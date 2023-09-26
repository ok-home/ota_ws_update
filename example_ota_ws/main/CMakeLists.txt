# Embed the server root certificate into the final binary

idf_component_register(
                    SRCS 
                      example_ota_ws_update.c
                    REQUIRES
                      ota_ws_update
                      prv_Wifi_Connect
                      esp_http_server
                      mdns
                      esp_wifi
)