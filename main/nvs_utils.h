#ifndef NVS_UTILS_H
#define NVS_UTILS_H

#include "nvs.h"
#include "esp_log.h"

esp_err_t open_nvs(char * NVS_PARTITION);
esp_err_t read_str_nvs(char * NVS_PARTITION, char* key, char** out_value);
esp_err_t write_str_nvs(char * NVS_PARTITION, char* key, char* value);

#endif