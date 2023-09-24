#pragma once

#include <esp_log.h>
#include <esp_system.h>
#include "esp_http_server.h"

#define OTA_RESTART_ESP "otaRestartEsp"
#define OTA_SIZE_START "otaSize"
#define OTA_SET_CHUNK_SIZE "otaSetChunkSize"
#define OTA_GET_CHUNK "otaGetChunk"
#define OTA_END "otaEnd"
#define OTA_ERROR "otaError"
#define OTA_CANCEL "otaCancel"

#define OTA_CHUNK_SIZE 4096

esp_err_t start_ota_ws(void);
esp_err_t write_ota_ws(int data_read, uint8_t *ota_write_data);
esp_err_t end_ota_ws(void);