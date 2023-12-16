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

#define SYSTEM_ID "18001840-2CEF-4025-956F-C180E99B927A"
#define DEVICE_ID "4C289BFE-4F3E-498A-8A5D-CF8DD39372AA" 
#define SEND_MEASUREMENTS_RATIO 10000//300000


#define ACTUATORS_PARTITION "ACTUATORS"

char http_client_json_buffer[1024];

float current;
float temperature;
float luminosity;

esp_mqtt_client_handle_t mqtt_client;

int actuators_length;
char** actuators_names;
int* actuators_ports;
int* actuators_status;


void send_measurements(void* parameters)
{
	while(1) 
	{
		vTaskDelay(pdMS_TO_TICKS(SEND_MEASUREMENTS_RATIO)); // 300000 = 5 min
		
		sprintf(http_client_json_buffer, "{\
			\"system_id\" : \"18001840-2CEF-4025-956F-C180E99B927A\",\
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
		}", current, temperature, luminosity);
		
		post(http_client_json_buffer);
		
	}
}

void set_actuators() {
	
	for(int i=0; i < actuators_length; i++) {
		gpio_set_direction(actuators_ports[i], GPIO_MODE_OUTPUT);
		gpio_set_level(actuators_ports[i], (uint32_t)actuators_status[i]);
	}
	
}

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
void OnMQTTConnected() {
	esp_mqtt_client_subscribe(mqtt_client, "actuators/18001840-2CEF-4025-956F-C180E99B927A", 0);
	esp_mqtt_client_subscribe(mqtt_client, "new_actuators/18001840-2CEF-4025-956F-C180E99B927A", 0);
}

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

#define AC_SENSOR_SAMPLING_NUMBER 100
#define AC_SENSOR_ATTENUATION ADC_ATTEN_DB_0
#define DELAY_SENSOR_READ 5000

adc_oneshot_unit_handle_t adc1_handle;
	
adc_oneshot_unit_init_cfg_t init_config1 = {
	.unit_id = ADC_UNIT_1,
	.ulp_mode = ADC_ULP_MODE_DISABLE,
};


void LeerCorrientePotenciometro(void* parameters)
{

	adc_cali_handle_t adc_cali_handle;

	adc_oneshot_chan_cfg_t adc1_config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = AC_SENSOR_ATTENUATION,
	};


	adc_cali_line_fitting_config_t calibration_config = {
		.unit_id = ADC_UNIT_1,
		.atten = AC_SENSOR_ATTENUATION,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.default_vref = 1100
	};
	
	//ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_6, &adc1_config));
	
	// SUPPORTEA enumerator ADC_CALI_SCHEME_VER_CURVE_FITTING Curve fitting scheme.

	ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&calibration_config, &adc_cali_handle));
	
	int raw;
	int voltage_result;
	int voltage_temp;
	
	while (1) {
		raw = 0;
		voltage_result = 0;
		voltage_temp = 0;
		for(int i = 0; i < AC_SENSOR_SAMPLING_NUMBER; i++) 
		{
			adc_oneshot_read(adc1_handle, ADC_CHANNEL_6, &raw);
			adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_temp);
			
			voltage_result += voltage_temp;
			
			voltage_temp = 0;
			raw = 0;
			vTaskDelay(1);
		}
		
		voltage_result = (voltage_result / AC_SENSOR_SAMPLING_NUMBER); 
		//amperage_result = ((((voltage_result/1000) * 60)/1.1f) - 30.0f);
		
		current = (voltage_result * 5000)/1100.0f;
		printf("Corriente:%f\n", current);
		vTaskDelay(DELAY_SENSOR_READ / portTICK_PERIOD_MS);
	}
}

void LeerLuminosidadPotenciometro(void* parameters)
{

	adc_cali_handle_t adc_cali_handle;

	adc_oneshot_chan_cfg_t adc1_config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = AC_SENSOR_ATTENUATION,
	};


	adc_cali_line_fitting_config_t calibration_config = {
		.unit_id = ADC_UNIT_1,
		.atten = AC_SENSOR_ATTENUATION,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.default_vref = 1100
	};
	
	//ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_4, &adc1_config));
	
	// SUPPORTEA enumerator ADC_CALI_SCHEME_VER_CURVE_FITTING Curve fitting scheme.

	ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&calibration_config, &adc_cali_handle));
	
	int raw;
	int voltage_result;
	int voltage_temp;
	
	while (1) {
		raw = 0;
		voltage_result = 0;
		voltage_temp = 0;
		for(int i = 0; i < AC_SENSOR_SAMPLING_NUMBER; i++) 
		{
			adc_oneshot_read(adc1_handle, ADC_CHANNEL_4, &raw);
			adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_temp);
			
			voltage_result += voltage_temp;
			
			voltage_temp = 0;
			raw = 0;
			vTaskDelay(1);
		}
		
		voltage_result = (voltage_result / AC_SENSOR_SAMPLING_NUMBER); 
		//amperage_result = ((((voltage_result/1000) * 60)/1.1f) - 30.0f);
		
		luminosity = (voltage_result * 20)/1100.0f;
		printf("Luminosidad:%f\n", luminosity);
		vTaskDelay(DELAY_SENSOR_READ / portTICK_PERIOD_MS);
	}
}

void LeerTemperaturaPotenciometro(void* parameters)
{

	
	adc_cali_handle_t adc_cali_handle;

	adc_oneshot_chan_cfg_t adc1_config = {
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.atten = AC_SENSOR_ATTENUATION,
	};


	adc_cali_line_fitting_config_t calibration_config = {
		.unit_id = ADC_UNIT_1,
		.atten = AC_SENSOR_ATTENUATION,
		.bitwidth = ADC_BITWIDTH_DEFAULT,
		.default_vref = 1100
	};
	
	ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, ADC_CHANNEL_7, &adc1_config));
	
	// SUPPORTEA enumerator ADC_CALI_SCHEME_VER_CURVE_FITTING Curve fitting scheme.

	ESP_ERROR_CHECK(adc_cali_create_scheme_line_fitting(&calibration_config, &adc_cali_handle));
	
	int raw;
	int voltage_result;
	int voltage_temp;
	
	while (1) {
		raw = 0;
		voltage_result = 0;
		voltage_temp = 0;
		for(int i = 0; i < AC_SENSOR_SAMPLING_NUMBER; i++) 
		{
			adc_oneshot_read(adc1_handle, ADC_CHANNEL_7, &raw);
			adc_cali_raw_to_voltage(adc_cali_handle, raw, &voltage_temp);
			
			voltage_result += voltage_temp;
			
			voltage_temp = 0;
			raw = 0;
			vTaskDelay(1);
		}
		
		voltage_result = (voltage_result / AC_SENSOR_SAMPLING_NUMBER); 
		//amperage_result = ((((voltage_result/1000) * 60)/1.1f) - 30.0f);
		
		temperature = ((voltage_result * 65)/1100.0f) - 15.0f;
		printf("Temperatura:%f\n", temperature);
		vTaskDelay(DELAY_SENSOR_READ / portTICK_PERIOD_MS);
	}
}



void app_main(void)
{
	mqtt_data_event_handler = OnMQTTDataRecived;
	mqtt_connected_event_handler = OnMQTTConnected;
	http_get_actuators_event_handler = OnGetActuators;
	ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
	
	
	wifi_init_sta();
	
	vTaskDelay(8000 / portTICK_PERIOD_MS);
	//ESP_ERROR_CHECK(init_web_server());
	mqtt_client = mqtt_app_start();
	
	vTaskDelay(8000 / portTICK_PERIOD_MS);
	
	get_actuators_from_bbdd(SYSTEM_ID, DEVICE_ID); // Recogemos los actuadores de base de datos y los grabamos en la NVS.
	get_actuators(); // Leemos esos actuadores de la NVS y los guardamos como variables.
	set_actuators(); // Ponemos esos actuadores como se quedaron.
	
	ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config1, &adc1_handle));
	
	xTaskCreate(LeerTemperaturaPotenciometro, "LeerTemperaturaPotenciometro", 2048, NULL, 5, NULL);
	xTaskCreate(LeerCorrientePotenciometro, "LeerCorrientePotenciometro", 2048, NULL, 5, NULL);
	xTaskCreate(LeerLuminosidadPotenciometro, "LeerLuminosidadPotenciometro", 2048, NULL, 5, NULL);
	
	xTaskCreate(send_measurements, "send_measurements", 4096, NULL, 5, NULL);
	
}
