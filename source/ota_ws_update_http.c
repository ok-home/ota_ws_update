/* 
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include "ota_ws_update_private.h"
#include "ota_ws_update.h"

#include "jsmn.h"

#define OTA_DEFAULT_WS_URI CONFIG_OTA_DEFAULT_WS_URI
#define OTA_DEFAULT_URI CONFIG_OTA_DEFAULT_URI
#define OTA_CHUNK_SIZE (CONFIG_OTA_CHUNK_SIZE & ~0xf)


static const char *TAG = "ota_ws_http";

static int ota_size;  // ota firmware size
static int ota_start_chunk; // start address of http chunk
static int ota_started; // ota download started

static esp_err_t json_to_str_parm(char *jsonstr, char *nameStr, char *valStr);
static esp_err_t send_json_string(char *str, httpd_req_t *req);
static esp_err_t ota_ws_handler(httpd_req_t *req);
static void ota_error(httpd_req_t *req, char *code, char *msg);

// abort OTA, send error/cancel msg to ws
static void ota_error(httpd_req_t *req, char *code, char *msg)
{
    char json_str[128];
    ota_size = ota_start_chunk = ota_started = 0;
    abort_ota_ws();
    ESP_LOGE(TAG, "%s %s", code, msg);
    snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\":\"%s\"}", code, msg);
    send_json_string(json_str, req);
}

// simple json parse -> only one parametr name/val
static esp_err_t json_to_str_parm(char *jsonstr, char *nameStr, char *valStr) // распаковать строку json в пару  name/val
{
    int r; // количество токенов
    jsmn_parser p;
    jsmntok_t t[5]; // только 2 пары параметров и obj

    jsmn_init(&p);
    r = jsmn_parse(&p, jsonstr, strlen(jsonstr), t, sizeof(t) / sizeof(t[0]));
    if (r < 2)
    {
        valStr[0] = 0;
        nameStr[0] = 0;
        return ESP_FAIL;
    }
    strncpy(nameStr, jsonstr + t[2].start, t[2].end - t[2].start);
    nameStr[t[2].end - t[2].start] = 0;
    if (r > 3)
    {
        strncpy(valStr, jsonstr + t[4].start, t[4].end - t[4].start);
        valStr[t[4].end - t[4].start] = 0;
    }
    else
        valStr[0] = 0;
    return ESP_OK;
}
// send string to ws
static esp_err_t send_json_string(char *str, httpd_req_t *req)
{
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = (uint8_t *)str;
    ws_pkt.len = strlen(str);
    return httpd_ws_send_frame(req, &ws_pkt);
}
// main ws OTA handler
// Handshake and process OTA
static esp_err_t ota_ws_handler(httpd_req_t *req)
{
    char json_key[64] = {0};
    char json_value[64] = {0};
    char json_str[128] = {0};

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;


    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        if(check_ota_ws_rollback_enable()) // check rollback enable, send cmd to enable rollback dialog on html
        {
        snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\":\"%s\" }", OTA_CHECK_ROLLBACK, "true");
        send_json_string(json_str, req);
        }
        return ESP_OK;
    }
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    // Set max_len = 0 to get the frame len 
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ota_error(req, OTA_ERROR, "httpd_ws_recv_frame failed to get frame len");
        return ret;
    }
    if (ws_pkt.len)
    {
        // ws_pkt.len + 1 is for NULL termination as we are expecting a string 
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ota_error(req, OTA_ERROR, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        // Set max_len = ws_pkt.len to get the frame payload 
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ota_error(req, OTA_ERROR, "httpd_ws_recv_frame failed");
            goto _recv_ret;
        }
    }
    ret = ESP_OK;
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT) // process json cmd
    {
        if (json_to_str_parm((char *)buf, json_key, json_value)) // decode json to key/value parm
        {
            ota_error(req, OTA_ERROR, "Error json str");
            goto _recv_ret;
        }
        if (strncmp(json_key, OTA_SIZE_START, sizeof(OTA_SIZE_START)) == 0) // start ota
        {
            ota_size = atoi(json_value);
            if (ota_size == 0)
            {
                ota_error(req, OTA_ERROR, "Error ota size = 0");
                goto _recv_ret;
            }
            ret = start_ota_ws();
            if (ret)
            {
                ota_error(req, OTA_ERROR, "Error start ota");
                goto _recv_ret;
            }
            ota_started = 1;
            ota_start_chunk = 0;
            snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\":%d}", OTA_SET_CHUNK_SIZE, OTA_CHUNK_SIZE); // set download chunk
            send_json_string(json_str, req);
            snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\":%d}", OTA_GET_CHUNK, ota_start_chunk); // cmd -> send first chunk with start addresss = 0
            send_json_string(json_str, req);
        }
        if (strncmp(json_key, OTA_CANCEL, sizeof(OTA_CANCEL)) == 0) // cancel ota
        {
            ota_error(req, OTA_CANCEL, "Cancel command");
            ret = ESP_OK;
            goto _recv_ret;
        }
        if (strncmp(json_key, OTA_ERROR, sizeof(OTA_ERROR)) == 0) // error ota
        {
            ota_error(req, OTA_ERROR, "Error command");
            ret = ESP_OK;
            goto _recv_ret;
        }
        if (strncmp(json_key, OTA_PROCESS_ROLLBACK, sizeof(OTA_PROCESS_ROLLBACK)) == 0) // process rollback &
        {
            if(strncmp(json_value,"true",sizeof("true")) == 0)
            {
                ESP_LOGI(TAG,"Rollback and restart");
                ret = rollback_ota_ws(true); // rollback and restart
            }
            else
            {
            ESP_LOGI(TAG,"App veryfied, fix ota update");
                ret = rollback_ota_ws(false);  // app veryfied
            }
            goto _recv_ret;
        }
        if (strncmp(json_key, OTA_RESTART_ESP, sizeof(OTA_RESTART_ESP)) == 0) // cancel ota
        {
            esp_restart();
        }
        
    }
    else if (ws_pkt.type == HTTPD_WS_TYPE_BINARY && ota_started) // download OTA firmware with chunked part
    {

        if (ota_start_chunk + ws_pkt.len < ota_size) //read chuk of ota
        {
            ret = write_ota_ws(ws_pkt.len, buf);  // write chunk of ota
            if (ret)
            {
                ota_error(req, OTA_ERROR, "Error write ota");
                goto _recv_ret;
            }
            ota_start_chunk += ws_pkt.len;
            snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\": %d }", OTA_GET_CHUNK, ota_start_chunk); // cmd -> next chunk
            send_json_string(json_str, req);

        }
        else        // last chunk and end ota
        {
            ret = write_ota_ws(ws_pkt.len, buf); // write last chunk of ota
            if (ret)
            {
                ota_error(req, OTA_ERROR, "Error write ota");
                goto _recv_ret;
            }
            ret = end_ota_ws(); // end ota
            if (ret)
            {
                ota_error(req, OTA_ERROR, "Error end ota");
                goto _recv_ret;
            }
            ota_size = 0;
            ota_start_chunk = 0;
            ota_started = 0;
            ESP_LOGI(TAG,"OTA END OK");
            snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\":\"%s\" }", OTA_END, "OK"); // send ota end cmd ( ota ok )
            send_json_string(json_str, req);
        }
    }
_recv_ret:
    free(buf);
    return ret;
}
// main http get handler
// send http initial page and js code
static esp_err_t ota_get_handler(httpd_req_t *req)
{
    extern const unsigned char ota_ws_update_html_start[] asm("_binary_ota_ws_update_html_start");
    extern const unsigned char ota_ws_update_html_end[] asm("_binary_ota_ws_update_html_end");
    const size_t ota_ws_update_html_size = (ota_ws_update_html_end - ota_ws_update_html_start);

    httpd_resp_send_chunk(req, (const char *)ota_ws_update_html_start, ota_ws_update_html_size);
    httpd_resp_sendstr_chunk(req, NULL);
    return ESP_OK;
}
static const httpd_uri_t gh = {
    .uri = OTA_DEFAULT_URI,
    .method = HTTP_GET,
    .handler = ota_get_handler,
    .user_ctx = NULL};
static const httpd_uri_t ws = {
    .uri = OTA_DEFAULT_WS_URI,
    .method = HTTP_GET,
    .handler = ota_ws_handler,
    .user_ctx = NULL,
    .is_websocket = true};

// register all ota uri handler
esp_err_t ota_ws_register_uri_handler(httpd_handle_t server)
{
    esp_err_t ret = ESP_OK;
    ret = httpd_register_uri_handler(server, &gh);
    if (ret)
        goto _ret;
    ret = httpd_register_uri_handler(server, &ws);
    if (ret)
        goto _ret;
_ret:
    return ret;
}
