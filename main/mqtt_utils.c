#include "mqtt_utils.h"

#define BROKER_URL "mqtts://a1jc4iy8w69cwq-ats.iot.eu-west-1.amazonaws.com:8883"
#define ACTUATORS_URL "actuators/18001840-2CEF-4025-956F-C180E99B927A"

static const char *TAG = "[MQTT]";
DataEventHandler mqtt_data_event_handler = NULL;
ConnectedEventHandler mqtt_connected_event_handler = NULL;

void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id) {
		case MQTT_EVENT_CONNECTED:
			ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			if(mqtt_connected_event_handler != NULL) {
				mqtt_connected_event_handler();
			}
			//esp_mqtt_client_subscribe(mqtt_client, "ESP32A/prueba-conectividad", 0);
			//esp_mqtt_client_publish(mqtt_client, "ESP32A/prueba-conectividad", "{\"message\": \"Prueba de conectividad.\"}", 0, 1, 0);
			break;
		case MQTT_EVENT_DISCONNECTED:
			//ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			break;
		case MQTT_EVENT_SUBSCRIBED:
			ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_UNSUBSCRIBED:
			//ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_PUBLISHED:
			//ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
			break;
		case MQTT_EVENT_DATA:
			//ESP_LOGI(TAG, "MQTT_EVENT_DATA");
			//printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
			//printf("DATA=%.*s\r\n", event->data_len, event->data);
			
			if(mqtt_data_event_handler != NULL) {
				mqtt_data_event_handler(event->topic_len, &event->topic, event->data_len, &event->data);
			}
			
			break;
		case MQTT_EVENT_BEFORE_CONNECT:
			ESP_LOGI(TAG, "MQTT_EVENT_BEFORE_CONNECT:%d", event->event_id);
			break;
		case MQTT_EVENT_ERROR:
			ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
			if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
				ESP_LOGE(TAG, "reported from esp-tls: 0x%x", event->error_handle->esp_tls_last_esp_err);
				ESP_LOGE(TAG, "reported from tls stack: 0x%x", event->error_handle->esp_tls_stack_err);
				ESP_LOGE(TAG, "captured as transport's socket errno: 0x%x", event->error_handle->esp_transport_sock_errno);
				ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));
			} 
			break;
		default:
			ESP_LOGI(TAG, "Other event id=%d", event->event_id);
			break;
    }
}



esp_mqtt_client_handle_t mqtt_app_start()
{	
	esp_mqtt_client_config_t mqtt_cfg = {
		.broker = {
			.address = {
				.uri = BROKER_URL,
				.hostname = "",
				.path = "",
				.port = 8883
			},
			.verification = {
				.use_global_ca_store = false,
				.certificate = (const char *)broker_certificate_start,
				.certificate_len = (const char *)broker_certificate_end - (const char *)broker_certificate_start,
				.skip_cert_common_name_check = true
			},
		},
		.credentials = {
			.username = "ESP32A",
			.client_id = "ESP32A",
			.set_null_client_id = false,
			.authentication = {
				.password = "",
				.certificate = (const char *)device_certificate_start,
				.certificate_len = (const char *)device_certificate_end - (const char *)device_certificate_start,
				.key = (const char *)device_private_key_start,
				.key_len = (const char *)device_private_key_end - (const char *)device_private_key_start,
				.key_password = "",
				.key_password_len = 0,
				.use_secure_element = false,
			},
		},
		.session = {
			.disable_clean_session = false,
			.keepalive = 120,
			.disable_keepalive = true,
			.protocol_ver = MQTT_PROTOCOL_V_3_1_1,
			.message_retransmit_timeout = 10,
			.last_will = {
				.topic = "last-will",
				.msg = "Abandono conexion",
				.msg_len = sizeof("Abandono conexion"),
				.qos = 0,
				.retain = 0
			}
		},
		.network = {
			.reconnect_timeout_ms = 10000,
			.timeout_ms = 10000,
			.refresh_connection_after_ms = 100000,
			.disable_auto_reconnect = true,
		},
		.task = {
			.priority = 10,
			.stack_size = 8192,
		},
		.buffer = {
			.size = 4096,
			.out_size = 4096
		}
	};

    esp_mqtt_client_handle_t mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    ESP_ERROR_CHECK(esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL)); // Registra un evento ESP_EVENT_ANY_ID
    ESP_ERROR_CHECK(esp_mqtt_client_start(mqtt_client));
	
	return mqtt_client;
}