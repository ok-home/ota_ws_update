#pragma once

#ifdef __cplusplus
extern "C"
{
#endif

/*
*   @brief  register ota_ws httpd handlers ( web page & ws handlers) on existing  httpd server with ws support
*           uri page -> CONFIG_OTA_DEFAULT_WS_URI
*   @param  httpd_handle_t server -> existing server handle
*   @return
*           ESP_OK      -> register OK
*           ESP_FAIL    -> register FAIL
*/
esp_err_t ota_ws_register_uri_handler(httpd_handle_t server);

#ifdef __cplusplus
}
#endif