#include "http_client_utils.h"


#define proxy_url "https://bl7kss3skedjvhfijmyserk36i0fwvjr.lambda-url.eu-west-1.on.aws/"

//static const char *TAG = "[HTTP_CLIENT]";

extern const char server_cert_pem_start[] asm("_binary_root_cert_auth_crt_start");
extern const char server_cert_pem_end[] asm("_binary_root_cert_auth_crt_end");

GetActuatorsEventHandler http_get_actuators_event_handler = NULL;

esp_err_t client_event_get_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
		case HTTP_EVENT_ON_DATA:
			//printf("[HTTP_GET_CLIENT]: %.*s\n", evt->data_len, (char *)evt->data);
			if(http_get_actuators_event_handler != NULL) {
				char * prueba = (char *)evt->data;
				http_get_actuators_event_handler(&prueba, (evt->data_len));
			}
			break;
		default:
			break;
    }
    return ESP_OK;
}

esp_err_t client_event_post_handler(esp_http_client_event_handle_t evt)
{
    switch (evt->event_id)
    {
		case HTTP_EVENT_ON_DATA:
			printf("[HTTP_POST_CLIENT]: %.*s\n", evt->data_len, (char *)evt->data);
			break;
		default:
			break;
    }
    return ESP_OK;
}

void get_actuators_from_bbdd(char* system_id, char* device_id) {
	
	char full_url[256];
    snprintf(full_url, sizeof(full_url), "%s?system_id=%s&device_id=%s", proxy_url, system_id, device_id);

	esp_http_client_config_t config_get = {
		.url = full_url,
        .method = HTTP_METHOD_GET,
        .cert_pem = server_cert_pem_start,
        .event_handler = client_event_get_handler
	};
	
	esp_http_client_handle_t client = esp_http_client_init(&config_get);
	
    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}


void post(char* body) {
	
	esp_http_client_config_t config_post = {
		.url = proxy_url,
        .method = HTTP_METHOD_POST,
        .cert_pem = server_cert_pem_start,
        .event_handler = client_event_post_handler
	};
	
	esp_http_client_handle_t client = esp_http_client_init(&config_post);

    esp_http_client_set_post_field(client, body, strlen(body));
    esp_http_client_set_header(client, "Content-Type", "application/json");

    esp_http_client_perform(client);
    esp_http_client_cleanup(client);
}
