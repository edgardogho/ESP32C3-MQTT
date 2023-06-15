/*
 * wifihardcoded.c
 *
 *  Created on: Jun 15, 2023
 *      Author: edgardog
 */

#include "wifihardcoded.h"


static const char *TAG = "Wifi_hardcoded";
static int reintentos = 0;

void event_handler(void* arg, esp_event_base_t event_base,int32_t event_id, void* event_data)
{
	//Primer evento se produce cuando se inicializa el wifi en modo Station
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect(); //Conectar
    }
    //Verificamos si hay evento wifi y el ID es desconectado
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (reintentos < REINTENTOS_MAX) {
            esp_wifi_connect();
            reintentos++;
            ESP_LOGI(TAG, "Intendando conexion...%d",reintentos);
        } else {
        	ESP_LOGI(TAG, "Desconexion de wifi!!!");
            xEventGroupSetBits(flagsWifi, WIFI_FALLA);
        }
    }
    //Si es un evento de IP y el ID es GOT IP (obtuvo IP), podemos decir que estamos
    //conectados correctamente con DHCP.
    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "IP Asignada:" IPSTR, IP2STR(&event->ip_info.ip));
        reintentos = 0;
        xEventGroupSetBits(flagsWifi, WIFI_CONECTADO);
    }
}

void wifi_init_sta(void)
{
	//Creamos los flags para indicar el estado
	flagsWifi = xEventGroupCreate();

	//Inicializamos el modulo WIFI
    esp_netif_init();

    //Inicializamos la maquina de estados de conexion para STATION
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    //Creamos variable de configuracion
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    //Indicamos a la maquina de estado que eventos queremos recibir
    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    //Eventos WIFI cualesquiera
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    //Eventos DHCP (IP) cualesquiera
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    //Esto de abajo es una configuracion de SSID y Clave, pero deberia
    //ser reemplazado por wifi provisioning. Siendo que este codigo es
    //simple para alguien que esta aprendiendo como usar ESP32, usamos
    //valores hardcodeados por ahora.
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASS,
	     .threshold.authmode = WIFI_AUTH_WPA2_PSK,
        },
    };

    //Modo wifi STATION
    esp_wifi_set_mode(WIFI_MODE_STA);
    esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    esp_wifi_start();

    ESP_LOGI(TAG, "Modulo WIFI inicializado... esperando conexion...");

    //A partir de ahora, puede pasar que se conecte o que falle, el codigo
    //debe quedarse esperando verificando los flags WIFI_CONECTADO o WIFI_FALLA.
    //El valor portMAX_DELAY define un tiempo maximo de espera.
    EventBits_t bits = xEventGroupWaitBits(flagsWifi,WIFI_CONECTADO | WIFI_FALLA,pdFALSE,pdFALSE,portMAX_DELAY);

    //Se actualizaron los flags, asi que tenemos que identificar que fue lo que paso.
    if (bits & WIFI_CONECTADO) {
        ESP_LOGI(TAG, "Conectado a SSID:%s",WIFI_SSID);
        //Actualizo el estado para indicar con el LED de conexion.
        estado=3;
    } else if (bits & WIFI_FALLA) {
        ESP_LOGI(TAG, "Error conectando a SSID:%s",WIFI_SSID);
        estado=2;
    } else {
        ESP_LOGE(TAG, "Error verificando el estado de la conexion");
    }

    //Quitamos el monitoreo de eventos de wifi
    esp_event_handler_instance_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, instance_got_ip);
    esp_event_handler_instance_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, instance_any_id);
    vEventGroupDelete(flagsWifi);
}




