#include <stdio.h>

#include "globales.h"
#include "wifihardcoded.h"
#include "i2clcddisplay.h"
#include "rc522.h"

static const char *TAG = "main";

//Flags
uint8_t mqtt_conectado = 0;
uint8_t estado=0; //Indica si el wifi esta conectado o no...
uint8_t contador=0; //Contador
uint8_t msgDisplay=0;

/* Flags para comunicarse llevar el estado del wifi*/
EventGroupHandle_t flagsWifi;

/* Variable para manejar el cliente mqtt */
esp_mqtt_client_handle_t clienteMQTT = NULL;

char mensajeDisplay[21];

/*Rutina del timer cada 1 segundo */
static void  rutinaTIMER_ISR(void *args)
{
	if (0==estado){
		//Esperando conexion del wifi.. titila led cada un segundo.
		gpio_set_level(LED_PIN,1);
		estado=1;
	} else if (1==estado){
		gpio_set_level(LED_PIN,0);
		estado=0;
	} else if (2==estado){
		gpio_set_level(LED_PIN,0);
	} else {
		gpio_set_level(LED_PIN,1);
	}

}

char bufferMQTT[100];
//Manejador de eventos MQTT
static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id){
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "Cliente MQTT Conectado!");
        mqtt_conectado=1;
        //Subscripcion al topico
        msg_id = esp_mqtt_client_subscribe(client, "grupo6/topic_pub", 0);
        ESP_LOGI(TAG, "suscribiendo al topico grupo6/topic_sub, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "Cliente MQTT Desconectado!");
        mqtt_conectado=0;
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "Suscripto al topico grupo6/topic_sub, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "No suscripto al topico grupo6/topic_sub, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "Publicacion en MQTT, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "Evento MQTT recibido");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        memcpy(bufferMQTT,event->data,event->data_len);
        if (strstr(bufferMQTT, "display_i2c") != NULL){
        	//Mensaje para I2c
        	uint8_t fin =0;
        	uint8_t actual=0;
        	while(!fin){
        		mensajeDisplay[actual]=bufferMQTT[actual+31];
        		if (bufferMQTT[actual+31]=='\"'){
        			fin=1;
        			msgDisplay=1;
        			mensajeDisplay[actual]='\0';
        		}
        		actual++;
        	}
        }
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "Error en MQTT");
        break;
    default:
        ESP_LOGI(TAG, "Evento no manejado MQTT id:%d", event->event_id);
        break;
    }
}


static void mqtt_app_start(void)
{
    ESP_LOGI(TAG, "Iniciando cliente MQTT...");
    esp_mqtt_client_config_t mqttConfig = { .broker.address.uri= "mqtt://IP:8741",.credentials.client_id="Display"};
    clienteMQTT = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(clienteMQTT, ESP_EVENT_ANY_ID, mqtt_event_handler, clienteMQTT);
    esp_mqtt_client_start(clienteMQTT);
}

static rc522_handle_t scanner;

char bufferSalida[100];
static void rc522_handler(void* arg, esp_event_base_t base, int32_t event_id, void* event_data)
{
    rc522_event_data_t* data = (rc522_event_data_t*) event_data;

    switch(event_id) {
        case RC522_EVENT_TAG_SCANNED: {
                rc522_tag_t* tag = (rc522_tag_t*) data->ptr;
                ESP_LOGI(TAG, "Tarjeta detectada[%llu]", tag->serial_number);
                if(mqtt_conectado) {
                	sprintf(bufferSalida,"{\"tipo\":\"sensor_4\",\"valor\":\"%llu\"}",tag->serial_number);
       		        esp_mqtt_client_publish(clienteMQTT, "grupo6/topic_sub", bufferSalida, strlen(bufferSalida), 0, 0);
                }
            }
            break;
    }
}

void app_main(void)
{
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN,GPIO_MODE_OUTPUT);

	rc522_config_t config = {
		        .spi.host = 1,
		        .spi.miso_gpio = 4,
		        .spi.mosi_gpio = 5,
		        .spi.sck_gpio = 6,
		        .spi.sda_gpio = 7,
		    };
	rc522_create(&config, &scanner);
	rc522_register_events(scanner, RC522_EVENT_ANY, rc522_handler, NULL);
	rc522_start(scanner);

	I2CLCD_InitPort();
	I2CLCDDisplay display;
	display.Address = 0x4E;
	display.Backlight = 1;
    I2CLCD_Init(display);
    I2CLCD_WriteLine(display, 0, "Grupo 6: Iniciando..");
    I2CLCD_WriteLine(display, 1, "Conectando a WIFI   ");
    I2CLCD_WriteLine(display, 2, "                    ");
    I2CLCD_WriteLine(display, 3, "                    ");

	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
		nvs_flash_erase();
		ret = nvs_flash_init();
	}

	//Creo una rutina de timer ISR para que interrumpa cada 1 segundo
	const esp_timer_create_args_t variableTimer = {
			.callback = &rutinaTIMER_ISR,
		    .name = "Rutina del timer"
	};
	esp_timer_handle_t timer_handler;
	esp_timer_create(&variableTimer, &timer_handler);
	esp_timer_start_periodic(timer_handler, 1000000);
	ESP_LOGI(TAG, "Comenzando FSM de WIFI");
	wifi_init_sta();
	mqtt_app_start();
	I2CLCD_WriteLine(display, 0, "Grupo 6: Conectado  ");
	I2CLCD_WriteLine(display, 1, "recibiendo...       ");
	I2CLCD_WriteLine(display, 2, "                    ");
	I2CLCD_WriteLine(display, 3, "                    ");

	while(1){
		if (msgDisplay){
			msgDisplay=0;
			I2CLCD_WriteLine(display, 2, "                    ");
			I2CLCD_WriteLine(display, 2, mensajeDisplay);
		}
	}

}
