#ifndef AWS_UPDATER_H
#define AWS_UPDATER_H

#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "wifi_utils.h"


#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#define FIRMWARE_VERSION	0.0
#define UPDATE_JSON_URL		"https://firmware-esp32-sbc.s3.eu-west-1.amazonaws.com/hito5-6-updates.json"


esp_err_t _http_event_handler(esp_http_client_event_t *evt);
void check_aws_update_task(void *pvParameter);

#endif