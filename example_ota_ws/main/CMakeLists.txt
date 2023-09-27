# Embed the server root certificate into the final binary

idf_component_register(
                    SRCS 
                      example_ota_ws_update.c
                      example_echo_ws_server.c
                    REQUIRES
                      ota_ws_update
                      nvs_wifi_connect
                      esp_http_server
                      mdns
                      esp_wifi
                    EMBED_FILES 
                      example_echo_ws_server.html
)