# Embed the server root certificate into the final binary

idf_component_register(
                    SRCS 
                      example_ota_ws.c
                      ../source/ota_ws_esp.c
                      ../source/ota_ws_http.c 
                    INCLUDE_DIRS 
                      ../include
                    PRIV_INCLUDE_DIRS
                      ../private_include
                    REQUIRES
                      prv_Wifi_Connect
                      esp_http_server
                      mdns
                      app_update
                      esp_wifi
                    EMBED_FILES 
                      ../source/ota_ws.html
)