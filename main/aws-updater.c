#include "aws-updater.h"

// receive buffer
char rcv_buffer[1024];

// server certificates
extern const char server_cert_pem_start[] asm("_binary_root_cert_auth_crt_start");
extern const char server_cert_pem_end[] asm("_binary_root_cert_auth_crt_end");

esp_err_t _http_event_handler(esp_http_client_event_t *evt) {
    
	switch(evt->event_id) {
        case HTTP_EVENT_ERROR:
            break;
        case HTTP_EVENT_ON_CONNECTED:
            break;
        case HTTP_EVENT_HEADER_SENT:
            break;
        case HTTP_EVENT_ON_HEADER:
            break;
        case HTTP_EVENT_ON_DATA:
			printf("\n%.*s\n",evt->data_len,(char*)evt->data);
            if (!esp_http_client_is_chunked_response(evt->client)) {
				strncpy(rcv_buffer, (char*)evt->data, evt->data_len);
            }
            break;
        case HTTP_EVENT_ON_FINISH:
            break;
        case HTTP_EVENT_DISCONNECTED:
            break;
		case HTTP_EVENT_REDIRECT:
            break;
    }
    return ESP_OK;
}

void check_aws_update_task(void *pvParameter) {
	
	while(1) {
        
		printf("Buscando nuevo firmware...\n");
	
		// configure the esp_http_client
		esp_http_client_config_t config = {
			.url = UPDATE_JSON_URL,
			.event_handler = _http_event_handler,
			.cert_pem = server_cert_pem_start,
		};
		esp_http_client_handle_t client = esp_http_client_init(&config);
	
		// downloading the json file
		esp_err_t err = esp_http_client_perform(client);
		if(err == ESP_OK) {
			
			// parse the json file	
			cJSON *json = cJSON_Parse(rcv_buffer);
			if(json == NULL) printf("el archivo descargado no es un json valido, abortando...\n");
			else {	
				cJSON *version = cJSON_GetObjectItemCaseSensitive(json, "version");
				cJSON *file = cJSON_GetObjectItemCaseSensitive(json, "file");
				
				// check the version
				if(!cJSON_IsNumber(version)) printf("no es posible leer la nueva version, abortando...\n");
				else {
					
					double new_version = version->valuedouble;
					if(new_version > FIRMWARE_VERSION) {
						
						printf("la version actual del firmware (%.1f) es menor que la disponible (%.1f), actualizando...\n", FIRMWARE_VERSION, new_version);
						if(cJSON_IsString(file) && (file->valuestring != NULL)) {
							printf("descargando e instalando nuevo firmware (%s)...\n", file->valuestring);
							
							esp_http_client_config_t client_config = {
								.url = file->valuestring,
								.cert_pem = server_cert_pem_start,
							};
							
							esp_https_ota_config_t ota_config = {
								.http_config = &client_config,
							};
							
							esp_err_t ret = esp_https_ota(&ota_config);
							if (ret == ESP_OK) {
								printf("OTA OK, reiniciando...\n");
								esp_restart();
							} else {
								printf("OTA fallida...\n");
							}
						}
						else printf("no es posible leer el nuevo enlace de firmware, abortando...\n");
					}
					else printf("la actual version del firmware (%.1f) es mayor o igual que la disponible (%.1f), nada que hacer...\n", FIRMWARE_VERSION, new_version);
				}
			}
		}
		else printf("imposible descargar el json de version, abortando...\n");
		
		// cleanup
		esp_http_client_cleanup(client);
		
		printf("\n");
        vTaskDelay(30000 / portTICK_PERIOD_MS);
    }
}

