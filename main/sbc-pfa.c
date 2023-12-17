#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_adc/adc_cali_scheme.h"
#include "esp_adc/adc_oneshot.h"

// Librerias propias
#include "wifi_utils.h"
#include "http_client_utils.h"
#include "mqtt_utils.h"
#include "nvs_utils.h"
#include "configuration_point.h"
#include "aws_updater.h"

#define SYSTEM_ID "18001840-2CEF-4025-956F-C180E99B927A" // Deben proporcionarte tu ID de sistema.
#define DEVICE_ID "4C289BFE-4F3E-498A-8A5D-CF8DD39372AA" // Deben proporcionarte tu ID de este dispositivo en concreto.
#define SEND_MEASUREMENTS_RATIO 10000 // 10000 = 10s, ajustar al dispositivo en concreto.


#define ACTUATORS_PARTITION "ACTUATORS"

char http_client_json_buffer[1024];

esp_mqtt_client_handle_t mqtt_client;

int actuators_length;
char** actuators_names;
int* actuators_ports;
int* actuators_status;

// Importante, este metodo es usado para enviar los datos que registra tu ESP32, 
// si tu dispositivo debe enviar telemetrias, este JSON debe ser rellenado,
// la funcion de envio está comentada en el main, descomentarla y ajustar el JSON
// para tu dispositivo en concreto, se proporciona uno de ejemplo.
void send_measurements(void* parameters)
{
	while(1) 
	{
		vTaskDelay(pdMS_TO_TICKS(SEND_MEASUREMENTS_RATIO)); 
		
		// Debes modificar el JSON con tu sistema.
		//sprintf(http_client_json_buffer, "{\
			\"system_id\" : \"%s\",\
			\"measurements\" : [\
				{\
					\"name\"  : \"Corriente\",\
					\"unit\"  : \"mA/h\",\
					\"value\" : %f\
				},\
				{\
					\"name\"  : \"Temperatura\",\
					\"unit\"  : \"Celsius\",\
					\"value\" : %f\
				},\
				{\
					\"name\"  : \"Luminosidad\",\
					\"unit\"  : \"Lumens\",\
					\"value\" : %f\
				}\
			]\
		}", system_id, current, temperature, luminosity);
		
		post(http_client_json_buffer);
		
	}
}

// Este método settea todos los actuadores de tu dispositivo a su estado. 
void set_actuators() {
	
	for(int i=0; i < actuators_length; i++) {
		gpio_set_direction(actuators_ports[i], GPIO_MODE_OUTPUT);
		gpio_set_level(actuators_ports[i], (uint32_t)actuators_status[i]);
	}
	
}

// Lee los actuadores grabados en NVS, por si no hay conexion a internet, que siga funcionando 
// correctamente en el ultimo estado en que se quedó. Tambien transforma los valores leidos en
// NVS a estructuras de array, para poder trabajar con ellos más fácilmente.
esp_err_t get_actuators() {
	// Partición: ACTUATORS
	// ACTUATORS_LENGTH -> 3 || Numero de actuadores en el sistema.
	// ACTUATOR_0 -> LamparaSalon || Nombre del actuador.
	// LamparaSalon -> 23 || Puerto del actuador.
	// LamparaSalonS -> 0 || Estado del actuador.
	esp_err_t response = open_nvs(ACTUATORS_PARTITION);
	
	if(response != ESP_OK) return response;
	
	char* temp = NULL;
	
	response = read_str_nvs(ACTUATORS_PARTITION, "ACTUATORS_LEN", &temp);
	
	if(response != ESP_OK) {
		if(response == ESP_ERR_NVS_NOT_FOUND || response == ESP_ERR_NVS_INVALID_NAME) {
			response = write_str_nvs(ACTUATORS_PARTITION, "ACTUATORS_LEN", "0");
		}
		ESP_LOGE("[NVS]", "Error leyendo la NVS. %s", esp_err_to_name(response));
		return response;
	}
	
	actuators_length = atoi(temp);
	actuators_names = malloc(actuators_length * sizeof(char *));
	actuators_ports = malloc(actuators_length * sizeof(int));
	actuators_status = malloc(actuators_length * sizeof(int));
	
	
	for(int i=0; i < actuators_length; i++) {
		// Nombre del actuador
		char nvs_key [16] = "ACTUATOR_";
		strcat(nvs_key, itoa(i, temp, 10));
		read_str_nvs(ACTUATORS_PARTITION, nvs_key, &actuators_names[i]);
		
		// Puerto del actuador
		read_str_nvs(ACTUATORS_PARTITION, actuators_names[i], &temp);
		actuators_ports[i] = atoi(temp);
		
		// Status del actuador
		memset(nvs_key, '\0', sizeof(nvs_key)); // Limpiamos buffer de nvs_key
		strcpy(nvs_key, actuators_names[i]);
		strcat(nvs_key, "S");
		read_str_nvs(ACTUATORS_PARTITION, nvs_key, &temp);
		actuators_status[i] = atoi(temp);
	}
	
	printf("\nACTUADORES:\n");
	for(int i=0; i<actuators_length; i++) {
		printf("\nActuador[%d]: %s %d %d\n", i, actuators_names[i], actuators_ports[i], actuators_status[i]);
	}
	
	return ESP_OK;
}

// Esto es un handler de la libreria http_client_utils, este metodo, es llamado 
// cuando se recibe la informacion de la peticion GET actuators. La peticion 
// se realiza cuando se enciende el dispositivo, si tiene conexion a internet,
// cuando recibe los valores, se guardan en la memoria NVS.
void OnGetActuators(char** response, int response_lenght)
{
	char respuesta[512];
	strncpy(respuesta, *response, response_lenght);
	respuesta[response_lenght] = '\0';
	printf("Respuesta desde el main: %s", respuesta);
	
	open_nvs(ACTUATORS_PARTITION);
	
	char * token = strtok(respuesta, ",");
	actuators_length = atoi(token);
	
	write_str_nvs(ACTUATORS_PARTITION, "ACTUATORS_LEN", token);
	
	// Partición: ACTUATORS
	// ACTUATORS_LEN -> 3 || Numero de actuadores en el sistema.
	// ACTUATOR_0 -> LamparaSalon || Nombre del actuador.
	// LamparaSalon -> 23 || Puerto del actuador.
	// LamparaSalonS -> 0 || Estado del actuador.
	for(int i = 0; i<actuators_length; i++) {
		token = strtok(NULL, ",");
		// ACTUATOR_0 = token
		char key[16] = "ACTUATOR_";
		char temp[16];
		itoa(i, temp, 10);
		strcat(key, temp); // ACTUATOR_i
		write_str_nvs(ACTUATORS_PARTITION, key, token); // ACTUATOR_i = nombre_actuador
		
		//Limpiar key
		memset(key, '\0', sizeof(key));
		strcpy(key, token); // Key = nombre_actuador
		token = strtok(NULL, ","); // token = puerto_actuador
		write_str_nvs(ACTUATORS_PARTITION, key, token);
		
		//Anexamos S al nombre del actuador
		strcat(key, "S"); // NombreActuadorS
		token = strtok(NULL, ","); // token = estado_actuador
		write_str_nvs(ACTUATORS_PARTITION, key, token);
	}
}

// Cada vez que se conecta al Broker, se subscribe a los temas.
// Habria que cambiar el system_id por el define SYSTEM_ID
void OnMQTTConnected() {
	esp_mqtt_client_subscribe(mqtt_client, "actuators/18001840-2CEF-4025-956F-C180E99B927A", 0);
	esp_mqtt_client_subscribe(mqtt_client, "new_actuators/18001840-2CEF-4025-956F-C180E99B927A", 0);
}

// Handler para las suscripciones del dispositivo, cuando se recibe un mensaje por 
// los temas en concreto, se analiza ese mensaje en este metodo.
void OnMQTTDataRecived(int topic_length, char** topic, int data_length, char** data) {
    // Realiza la operación que necesitas desde el main con los parámetros
    printf("Operación desde el main con parámetros...\n");
	char tema[128];
	char datos[1024];
	strncpy(tema, *topic, topic_length);
	tema[topic_length] = '\0';
	strncpy(datos, *data, data_length);
	datos[data_length] = '\0';


	//switch del tema, dependiendo del tema realizamos una accion u otra... 
	printf("\n\nTEMA: %s\n\n", tema);
	printf("\n\nDATOS: %s\n\n", datos);
	
	//4C289BFE-4F3E-498A-8A5D-CF8DD39372AA/NombreActuador/1
	char * topic_token = strtok(tema, "/");
	
	printf("\n\n%s\n\n", topic_token);
	if(strcmp(topic_token, "actuators") == 0) {
		char* token = strtok(datos, "/");
		
		if(strcmp(token, DEVICE_ID) == 0) {
			// El mensaje de actuacion va para nosotros.
			token = strtok(NULL, "/");
			
			bool encontrado = false;
			int i;
			for(i = 0; i < actuators_length; i++) {
				if(strcmp(actuators_names[i], token) == 0) {
					encontrado = true;
					break;
				}
			}
			
			if(encontrado) {
				token = strtok(NULL, "/");
				actuators_status[i] = atoi(token); // GUARDAR EN NVS
				char nvs_key[32];
				strcpy(nvs_key, actuators_names[i]);
				strcat(nvs_key, "S");
				
				char status_value[8];
				itoa(actuators_status[i], status_value, 10);
				write_str_nvs(ACTUATORS_PARTITION, nvs_key, status_value);
				set_actuators();
				//Enviar HTTP de que se ha cambiado el estado de este actuador.
				memset(http_client_json_buffer, '\0', sizeof(http_client_json_buffer));
				sprintf(http_client_json_buffer, "{\
						\"system_id\" : \"%s\",\
						\"device_id\": \"%s\",\
						\"actuator_name\" : \"%s\",\
						\"actuator_status\": %d\
						}", SYSTEM_ID, DEVICE_ID, actuators_names[i], actuators_status[i]);
				post(http_client_json_buffer);
				memset(http_client_json_buffer, '\0', sizeof(http_client_json_buffer));
			}
		}
	} else if(strcmp(topic_token, "new_actuators") == 0) {
		get_actuators_from_bbdd(SYSTEM_ID, DEVICE_ID);
		get_actuators(); // Leemos esos actuadores de la NVS y los guardamos como variables.
		set_actuators();
	}
	
}


void app_main(void)
{
	// Funciones externas de las librerias.
	mqtt_data_event_handler = OnMQTTDataRecived;
	mqtt_connected_event_handler = OnMQTTConnected;
	http_get_actuators_event_handler = OnGetActuators;
	
	ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
	
	// Importante! Estaría bien, como mejora, añadir funcionalidad
	// para que el sistema espere para que se conecte al wifi. Puede que no se
	// conecte al WiFi en 8 segundos.
	wifi_init_sta();
	
	vTaskDelay(8000 / portTICK_PERIOD_MS); // Puede que debas modificarlo, dado a que tu sistema no se conecte a tiempo a la estacion wifi.
	
	// Falta la funcionalidad para que guarde los valores que recoge el Form de la pagina web 
	// a las variables de ssid y password.
	//ESP_ERROR_CHECK(init_web_server()); 
	
	mqtt_client = mqtt_app_start(); // Inicializacion de driver MQTT
	
	vTaskDelay(8000 / portTICK_PERIOD_MS); // Espera activa para que se complete la inicializacion de MQTT.
	
	get_actuators_from_bbdd(SYSTEM_ID, DEVICE_ID); // Recogemos los actuadores de base de datos y los grabamos en la NVS.
	get_actuators(); // Leemos esos actuadores de la NVS y los guardamos como variables.
	set_actuators(); // Ponemos esos actuadores como se quedaron.
	

	// Crear tareas correspondientes si tu sistema envia mediciones. 
	//xTaskCreate(LeerTemperaturaPotenciometro, "LeerTemperaturaPotenciometro", 2048, NULL, 5, NULL);
	//xTaskCreate(LeerCorrientePotenciometro, "LeerCorrientePotenciometro", 2048, NULL, 5, NULL);
	//xTaskCreate(LeerLuminosidadPotenciometro, "LeerLuminosidadPotenciometro", 2048, NULL, 5, NULL);
	
	// Descomentar si tu dispositivo va a enviar mediciones al sistema. 
	//xTaskCreate(send_measurements, "send_measurements", 4096, NULL, 5, NULL);
	
	// Descomentar si tu sistema va a realizar actualizaciones OTA.
	//xTaskCreate(check_aws_update_task, "aws_update_task", 2048, NULL, 5, NULL);
	
}
