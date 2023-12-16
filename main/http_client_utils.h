#ifndef HTTP_CLIENT_UTILS_H
#define HTTP_CLIENT_UTILS_H

#include "esp_http_client.h"
#include "esp_log.h"

void post(char* body);
void get_actuators_from_bbdd(char* system_id, char* device_id);
esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt);
esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt);

typedef void (*GetActuatorsEventHandler)(char** response, int response_lenght);

extern GetActuatorsEventHandler http_get_actuators_event_handler;

#endif