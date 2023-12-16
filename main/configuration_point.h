#ifndef CONFIGURATION_POINT_H
#define CONFIGURATION_POINT_H

#include "esp_log.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_http_server.h"
#include "freertos/event_groups.h"
#include "freertos/stream_buffer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"



#define MIN(a, b) ((a) < (b) ? (a) : (b))
// Variables de configuracion.
extern char* config_wifi_ssid;
extern char* config_wifi_password;

esp_err_t init_web_server();

#endif