/*
 * globales.h
 *
 *  Created on: Jun 15, 2023
 *      Author: edgardog
 */

#ifndef MAIN_GLOBALES_H_
#define MAIN_GLOBALES_H_
//Includes de freeRTOS
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
//Includes de esp-idf
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "esp_timer.h"
#include "rom/ets_sys.h"
#include "driver/spi_master.h"

//Includes del Stack TCP/IP
#include "lwip/err.h"
#include "lwip/sys.h"
#include <string.h>
//Cliente MQTT incluido en ESP-IDF
#include "mqtt_client.h"




//DEfines globales
#define LED_PIN 2
#define BUZZER_PIN 3
#define SENSOR_PIN 1
#define WIFI_SSID      "SSID"
#define WIFI_PASS      "clave"
#define REINTENTOS_MAX  6

//Flags para determinar si se conecto o no al wifi
#define WIFI_CONECTADO BIT0
#define WIFI_FALLA      BIT1

//Variable para llevar el estado de los flags
extern EventGroupHandle_t flagsWifi;
extern uint8_t estado;


#endif /* MAIN_GLOBALES_H_ */
