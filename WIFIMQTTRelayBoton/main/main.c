#include <stdio.h>

#include "globales.h"

#include "wifihardcoded.h"


static const char *TAG = "main";

//Flags
uint8_t mqtt_conectado = 0;
uint8_t estado=0; //Indica si el wifi esta conectado o no...
uint8_t contador=0; //Contador


/* Flags para comunicarse llevar el estado del wifi*/
EventGroupHandle_t flagsWifi;

/* Variable para manejar el cliente mqtt */
esp_mqtt_client_handle_t clienteMQTT = NULL;

static void TIMER_BUZZER(void *args)
{
	gpio_set_level(RELAY_PIN,0);
	gpio_set_level(RELAY_PIN,0);

}


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
        if (strstr(bufferMQTT, "motor_pap") != NULL){
        	//Mensaje para buzzer
        	uint8_t tiempo = bufferMQTT[29]-0x30;
        	const esp_timer_create_args_t variableTimer = {
        				.callback = &TIMER_BUZZER,
        			    .name = "Rutina del Buzzer"
        		};
        		esp_timer_handle_t timer_handler;
        		esp_timer_create(&variableTimer, &timer_handler);
        		esp_timer_start_once(timer_handler, 1000000*tiempo);
        		gpio_set_level(RELAY_PIN,1);

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
    esp_mqtt_client_config_t mqttConfig = { .broker.address.uri= "mqtt://IP:8741",.credentials.client_id="Relay"};
    clienteMQTT = esp_mqtt_client_init(&mqttConfig);
    esp_mqtt_client_register_event(clienteMQTT, ESP_EVENT_ANY_ID, mqtt_event_handler, clienteMQTT);
    esp_mqtt_client_start(clienteMQTT);
}

char bufferSalida[]="{\"tipo\":\"sensor_2\",\"valor\":\"BotonON\"}";
uint8_t detectado=0;
static void IRAM_ATTR rutinaISR_IRQ(void *args)
{


	if(mqtt_conectado) {
		detectado=1;

	                }
}



void app_main(void)
{
	gpio_reset_pin(LED_PIN);
	gpio_set_direction(LED_PIN,GPIO_MODE_OUTPUT);
	gpio_reset_pin(RELAY_PIN);
	gpio_set_direction(RELAY_PIN,GPIO_MODE_OUTPUT);
	gpio_set_level(RELAY_PIN,0);

	gpio_set_direction(SENSOR_PIN, GPIO_MODE_INPUT);
		gpio_pulldown_dis(SENSOR_PIN);
		//gpio_pullup_dis(SENSOR_PIN);
		gpio_set_intr_type(SENSOR_PIN, GPIO_INTR_POSEDGE);
		gpio_install_isr_service(0);
		//Defino rutina ISR
		gpio_isr_handler_add(SENSOR_PIN, rutinaISR_IRQ, NULL);


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
	wifi_init_sta();
	mqtt_app_start();

	while(1){
		if (detectado){
			detectado=0;
			 esp_mqtt_client_publish(clienteMQTT, "grupo6/topic_sub", bufferSalida, strlen(bufferSalida), 0, 0);
		}

	}

}
