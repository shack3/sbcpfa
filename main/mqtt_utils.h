#ifndef MQTT_UTILS_H
#define MQTT_UTILS_H

#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_system.h"

extern const uint8_t broker_certificate_start[] asm("_binary_root_cert_auth_crt_start");
extern const uint8_t broker_certificate_end[] asm("_binary_root_cert_auth_crt_end");
extern const uint8_t device_certificate_start[] asm("_binary_client_crt_start");
extern const uint8_t device_certificate_end[] asm("_binary_client_crt_end");
extern const uint8_t device_private_key_start[] asm("_binary_client_key_start");
extern const uint8_t device_private_key_end[] asm("_binary_client_key_end");

typedef void (*DataEventHandler)(int topic_length, char** topic, int data_length, char** data);
typedef void (*ConnectedEventHandler)();


extern DataEventHandler mqtt_data_event_handler;
extern ConnectedEventHandler mqtt_connected_event_handler;

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
esp_mqtt_client_handle_t mqtt_app_start();

#endif