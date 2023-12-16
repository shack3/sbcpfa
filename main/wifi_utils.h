#ifndef WIFI_UTILS_H
#define WIFI_UTILS_H

#include "esp_wifi.h"
#include "esp_log.h"
#include "freertos/event_groups.h"
#include "freertos/stream_buffer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include <stdint.h>


void wifi_init_sta();
void wifi_event_handler(void* arg, esp_event_base_t event_base,  int32_t event_id, void* event_data);

#endif