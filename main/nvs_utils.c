#include "nvs_utils.h"


nvs_handle_t app_nvs_handle;

// Abre la NVS en modo lectura escritura, de no conseguirlo satisfactoriamente 
// se logea el error. Se abre la particion especificada de la configuracion.
esp_err_t open_nvs(char * NVS_PARTITION) 
{
	esp_err_t error = nvs_open(NVS_PARTITION, NVS_READWRITE, &app_nvs_handle);
	if(error != ESP_OK) 
	{
		const char * error_name = esp_err_to_name(error);
		ESP_LOGE(NVS_PARTITION, "Error abriendo NVS. %s", error_name);
	} 
	else 
	{
		ESP_LOGI(NVS_PARTITION, "NVS abierta satisfactoriamente.");
	}
	
	return error;
}


esp_err_t read_str_nvs(char * NVS_PARTITION, char* key, char** out_value)
{
	esp_err_t error;
	size_t required_size;
	
	// Cogemos el tamaño requerido del string.
	error = nvs_get_str(app_nvs_handle, key, NULL, &required_size);
	
	if(error != ESP_OK) 
	{
		const char * error_name = esp_err_to_name(error);
		ESP_LOGE(NVS_PARTITION, "Error leyendo string de NVS. %s", error_name);
	}
	if(error == ESP_OK) 
	{
		// Asignamos la longitud del buffer.
		*out_value = malloc(required_size);
		// Al pasar a nvs_get_str el puntero de out_value, este lo asigna.
		error = nvs_get_str(app_nvs_handle, key, *out_value, &required_size);
		if(error != ESP_OK) 
		{
			const char * error_name = esp_err_to_name(error);
			ESP_LOGE(NVS_PARTITION, "Error asignando str de NVS. %s | el tamaño era: %d", error_name, required_size);
			// Liberar la memoria en caso de error
            free(*out_value);
            *out_value = NULL;
		} 
		ESP_LOGI(NVS_PARTITION, "Valor leido de NVS: %s", *out_value);
	}
	
	return error;
}


esp_err_t write_str_nvs(char * NVS_PARTITION, char* key, char* value)
{
	esp_err_t error = nvs_set_str(app_nvs_handle, key, value);

	if(error == ESP_OK)
	{
		ESP_LOGI(NVS_PARTITION, "Valor escrito en NVS: %s", value);
	}
	
	return error;
}