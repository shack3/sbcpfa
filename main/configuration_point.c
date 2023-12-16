#include "configuration_point.h"

char* ssid;
char* password;

char * const WEB_PAGE = "\
<!DOCTYPE html>\
<html lang=\"es\">\
<head>\
  <meta charset=\"UTF-8\">\
  <meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\">\
  <style>\
    body {\
      display: flex;\
      align-items: center;\
      justify-content: center;\
      height: 100vh;\
      margin: 0;\
      background-color: #f4f4f4;\
    }\
\
    #container {\
      text-align: center;\
      border-radius: 10px;\
      padding: 20px;\
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);\
      background-color: #fff;\
    }\
\
    input {\
      padding: 10px;\
      margin: 10px 0;\
      width: 200px;\
      border: 1px solid #ccc;\
      border-radius: 5px;\
    }\
\
    button {\
      padding: 10px 20px;\
      background-color: #4caf50;\
      color: #fff;\
      border: none;\
      border-radius: 5px;\
      cursor: pointer;\
    }\
\
	p {\
	  font-family: 'Arial', sans-serif;\
      font-size: 18px;\
      line-height: 1.6;\
      color: #333;\
      max-width: 600px;\
      margin: 0 auto;\
    }\
\
</style>\
</head>\
<body>\
  <div id=\"container\">\
  <p>\
  <b>Access Point Grupo 1 SBC 2023</b>\
  </p>\
  <p>\
	Valor actual SSID: MovistarPlus0D0C\
  </p>\
  <p>\
	Valor de la contraseña actual: SDcqwnmono123kmqweqw\
  </p>\
    <input type=\"text\" id=\"usuario\" placeholder=\"SSID\">\
    <br>\
    <input type=\"password\" id=\"contrasena\" placeholder=\"Contraseña\">\
    <br>\
    <button onclick=\"enviarDatos()\">Enviar</button>\
  </div>\
\
  <script>\
    function enviarDatos() {\
      var usuario = document.getElementById(\"usuario\").value;\
      var contrasena = document.getElementById(\"contrasena\").value;\
\
      var formData = new FormData();\
      formData.append(\"usuario\", usuario);\
      formData.append(\"contrasena\", contrasena);\
\
      fetch(\"post_form\", {\
        method: \"POST\",\
        body: formData\
      })\
      .then(response => response.json())\
      .then(data => {\
        console.log(data);\
      })\
      .catch(error => {\
        console.error(\"Error:\", error);\
      });\
    }\
  </script>\
</body>\
</html>";

static esp_err_t root_handler(httpd_req_t *req)
{
    esp_err_t error;
	ESP_LOGI("[WIFI_AP]", "Pagina ROOT enviada.");
	const char * response = (const char *) req->user_ctx;
	error = httpd_resp_send(req, response, strlen(response));
	if(error != ESP_OK) {
		ESP_LOGI("[WIFI_AP]", "Error %d while sending Response", error);
	} else ESP_LOGI("[WIFI_AP]", "Response sent succesfully");
	return error;
}

static int parseForm(char * buffer)
{
	char * pch;
	pch = strtok (buffer,"\n");
	int i = 0;
    while (pch != NULL) 
    {
		//pch es la linea completa del form.
		if(strstr(pch, "WebKitFormBoundary") == NULL && strstr(pch, "Content-Disposition") == NULL) {
			if(i == 3 || i == 7) 
			{
				//printf ("i: %d | %s\n",i,pch);
				if(i==3) ssid = pch;
				if(i==7) password = pch;
			}
				
		}
        pch = strtok (NULL, "\n");
		i++;
    }
	
	return 0;
}

static esp_err_t post_form_handler(httpd_req_t *req)
{
   /* Destination buffer for content of HTTP POST request.
     * httpd_req_recv() accepts char* only, but content could
     * as well be any binary data (needs type casting).
     * In case of string data, null termination will be absent, and
     * content length would give length of string */
    char content[1024];

    /* Truncate if content length larger than the buffer */
    size_t recv_size = MIN(req->content_len, sizeof(content));

    int ret = httpd_req_recv(req, content, recv_size); // Almacena el contenido que llega en el body del post en el buffer content.
	
    if (ret <= 0) {  /* 0 return value indicates connection closed */
        /* Check if timeout occurred */
        if (ret == HTTPD_SOCK_ERR_TIMEOUT) {
            /* In case of timeout one can choose to retry calling
             * httpd_req_recv(), but to keep it simple, here we
             * respond with an HTTP 408 (Request Timeout) error */
            httpd_resp_send_408(req);
        }
        /* In case of error, returning ESP_FAIL will
         * ensure that the underlying socket is closed */
        return ESP_FAIL;
    }
	// En este punto el contenido del Body es valido y hay que tratarlo.
	//printf("%s\n", content);
	parseForm(content);
	// Grabar en la NVS
	//printf("SSID recibido: %s\n", ssid);
	//write_str_nvs("ssid", ssid);
	//printf("Password recibida: %s\n", password);
	//write_str_nvs("password", password);
	// Inicializar WIFI station
    /* Send a simple response */
    const char resp[] = "URI POST Response";
    httpd_resp_send(req, resp, HTTPD_RESP_USE_STRLEN);
    return ESP_OK;
}


static const httpd_uri_t root = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = WEB_PAGE
};

static const httpd_uri_t post_form = {
    .uri       = "/post_form",
    .method    = HTTP_POST,
    .handler   = post_form_handler
};


static httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;

    // Start the httpd server
    ESP_LOGI("[WIFI_AP]", "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI("[WIFI_AP]", "Registering URI handlers");
        httpd_register_uri_handler(server, &root);
		httpd_register_uri_handler(server, &post_form);
        return server;
    }

    ESP_LOGI("[WIFI_AP]", "Error starting server!");
    return NULL;
}


static esp_err_t stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    return httpd_stop(server);
}


static void disconnect_handler(void* arg, esp_event_base_t event_base,
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI("[WIFI_AP]", "Stopping webserver");
        if (stop_webserver(*server) == ESP_OK) {
            *server = NULL;
        } else {
            ESP_LOGE("[WIFI_AP]", "Failed to stop http server");
        }
    }
}

static void connect_handler(void* arg, esp_event_base_t event_base,
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI("[WIFI_AP]", "Starting webserver");
        *server = start_webserver();
    }
}


esp_netif_t *wifi_init_softap(void)
{
    esp_netif_t *esp_netif_ap = esp_netif_create_default_wifi_ap();

    wifi_config_t wifi_ap_config = {
        .ap = {
            .ssid = "ESP32 CONFIGURATION AP GRUPO 1",
            .ssid_len = strlen("ESP32 CONFIGURATION AP GRUPO 1"),
            .channel = 0,
            .password = "1234567890",
            .max_connection = 2,
            .authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                    .required = true,
            },
        },
    };

    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_ap_config));

    ESP_LOGI("[WIFI_AP]", "Acces Point Wifi finished. SSID:%s password:%s channel:%d",
             wifi_ap_config.ap.ssid, wifi_ap_config.ap.password, wifi_ap_config.ap.channel);

    return esp_netif_ap;
}


esp_err_t init_web_server(){
	
	esp_err_t ret = esp_wifi_set_mode(WIFI_MODE_APSTA); 
	
	ESP_LOGI("[WIFI_AP]", "ESP_WIFI_MODE_AP");
    wifi_init_softap();

	// Registrar eventos y handlers para el servidor web.
	static httpd_handle_t server = NULL;
	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STACONNECTED, &connect_handler, &server)); 
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, WIFI_EVENT_AP_STADISCONNECTED, &disconnect_handler, &server));

	return ret;
}


