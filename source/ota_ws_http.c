#include "ota_ws_private.h"
#include "ota_ws.h"

#include "freertos/task.h"
#include "freertos/queue.h"

#include "jsmn.h"

#define OTA_DEFAULT_WS_URI "/ws"
#define OTA_DEFAULT_URI "/"

static const char *TAG = "ota_ws";

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
static esp_err_t send_json_string(char *str, httpd_req_t *req)
{
    httpd_ws_frame_t ws_pkt;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    ws_pkt.payload = (uint8_t *)str;
    ws_pkt.len = strlen(str);
    return httpd_ws_send_frame(req, &ws_pkt);
}
static esp_err_t ota_ws_handler(httpd_req_t *req)
{
    static int ota_size;
    static int ota_start_chunk;

    if (req->method == HTTP_GET)
    {
        ESP_LOGI(TAG, "Handshake done, the new connection was opened");
        return ESP_OK;
    }
    char json_key[64]={0};
    char json_value[64]={0};
    char json_str[64] = {0};

    httpd_ws_frame_t ws_pkt;
    uint8_t *buf = NULL;
    memset(&ws_pkt, 0, sizeof(httpd_ws_frame_t));
    // ws_pkt.type = HTTPD_WS_TYPE_TEXT;
    /* Set max_len = 0 to get the frame len */
    esp_err_t ret = httpd_ws_recv_frame(req, &ws_pkt, 0);
    if (ret != ESP_OK)
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame failed to get frame len with %d", ret);
        return ret;
    }
    if (ws_pkt.len)
    {
        /* ws_pkt.len + 1 is for NULL termination as we are expecting a string */
        buf = calloc(1, ws_pkt.len + 1);
        if (buf == NULL)
        {
            ESP_LOGE(TAG, "Failed to calloc memory for buf");
            return ESP_ERR_NO_MEM;
        }
        ws_pkt.payload = buf;
        /* Set max_len = ws_pkt.len to get the frame payload */
        ret = httpd_ws_recv_frame(req, &ws_pkt, ws_pkt.len);
        if (ret != ESP_OK)
        {
            ESP_LOGE(TAG, "httpd_ws_recv_frame failed with %d", ret);
            goto _recv_ret;
        }
    }
    ret = ESP_OK;
    if (ws_pkt.type == HTTPD_WS_TYPE_TEXT)
    {
        ESP_LOGI(TAG, "Rcv txt msg len=%d data=%s", ws_pkt.len, buf);
        if (json_to_str_parm((char*)buf,json_key,json_value))
        {
            ESP_LOGE(TAG,"error json str: %s",buf);
            goto _recv_ret;
        }
        if(strcmp(json_key,OTA_SIZE_START)==0)// start ota
        {
            ota_size = atoi(json_value);
            ota_start_chunk = 0;
            snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\":%d}", OTA_SET_CHUNK_SIZE, OTA_CHUNK_SIZE);
            send_json_string(json_str,req);
            snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\":%d}", OTA_GET_CHUNK, ota_start_chunk);
            send_json_string(json_str,req);
        }

    }
    else if (ws_pkt.type == HTTPD_WS_TYPE_BINARY)
    {
        ESP_LOGI(TAG, "Rcv bin msg len=%d", ws_pkt.len);
        ota_start_chunk += ws_pkt.len;
        if(ota_start_chunk < ota_size)
        {
            snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\": %d }", OTA_GET_CHUNK, ota_start_chunk);
            send_json_string(json_str,req);

        }
        else{
// end ota
            ota_size = 0;
            ota_start_chunk = 0;
            snprintf(json_str, sizeof(json_str), "{\"name\":\"%s\",\"value\":\"%s\" }", OTA_END, "OK");
            send_json_string(json_str,req);

        }
    }
    else
    {
        ESP_LOGE(TAG, "httpd_ws_recv_frame unknown frame type %d", ws_pkt.type);
        ret = ESP_FAIL;
        goto _recv_ret;
    }
_recv_ret:
    free(buf);
    return ret;
}
static esp_err_t ota_get_handler(httpd_req_t *req)
{
    extern const unsigned char ota_ws_html_start[] asm("_binary_ota_ws_html_start");
    extern const unsigned char ota_ws_html_end[] asm("_binary_ota_ws_html_end");
    const size_t ota_ws_html_size = (ota_ws_html_end - ota_ws_html_start);

    httpd_resp_send_chunk(req, (const char *)ota_ws_html_start, ota_ws_html_size);
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
