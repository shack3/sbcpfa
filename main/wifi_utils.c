#include "wifi_utils.h"

#define EXAMPLE_ESP_WIFI_SSID "MOVISTAR_Casa"
#define EXAMPLE_ESP_WIFI_PASS  "609360246626814563"


static const char *TAG = "[WIFI]";

static EventGroupHandle_t s_wifi_event_group;

static int s_retry_num = 5;

void wifi_event_handler(void* arg, esp_event_base_t event_base,  int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_retry_num < 5) {
            esp_wifi_connect(); // Intenta reconectar el wifi hasta el maximo numero de intentos.
            s_retry_num++;
            ESP_LOGI(TAG, "retry to connect to the AP");
        }
        ESP_LOGI(TAG,"connect to the AP fail");
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
    } else if (event_id == WIFI_EVENT_AP_STACONNECTED) {
		return;
    } else if (event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        return;
    }
	
}



void wifi_init_sta()
{
    s_wifi_event_group = xEventGroupCreate(); // Crear grupo de eventos del WIFI.

    ESP_ERROR_CHECK(esp_netif_init()); // Inicializa el TCP/IP. LLamar una vez en la app.
	

    ESP_ERROR_CHECK(esp_event_loop_create_default()); // Crea un loop de eventos por defecto.
    esp_netif_create_default_wifi_sta(); // Crea una configuracion para la estacion WiFi por defecto. Devuelve un puntero a la config por defecto.

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // Parametros por defecto para la inicializacion del WiFi.
    ESP_ERROR_CHECK(esp_wifi_init(&cfg)); // Inicia el wifi con los parametros por defecto.

    esp_event_handler_instance_t instance_any_id; // Identificador de un Event Handler.
    esp_event_handler_instance_t instance_got_ip; // Same
	
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            .threshold.authmode = WIFI_AUTH_OPEN,
            .sae_pwe_h2e = WPA3_SAE_PWE_HUNT_AND_PECK,
            
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); 
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "wifi_init_sta finished.");
}