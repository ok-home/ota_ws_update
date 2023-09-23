#pragma once

#include "ota_ws_private.h"

#ifdef __cplusplus
extern "C"
{
#endif


/*
*   @brief  register provision handlers ( web page & ws handlers) on existing  httpd server with ws support
*           uri page -> CONFIG_DEFAULT_URI
*   @param  httpd_handle_t server -> existing server handle
*   @return
*           ESP_OK      -> register OK
*           ESP_FAIL    -> register FAIL
*/
esp_err_t ota_ws_register_uri_handler(httpd_handle_t server);

#ifdef __cplusplus
}
#endif