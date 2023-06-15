/*
 * i2clcddisplay.c
 *
 *  Created on: Jun 15, 2023
 *      Author: edgardog
 */

#include "i2clcddisplay.h"

const uint8_t initArray[] = {  0x03, 0x03, 0x03 , 0x02, 0x02 , 0x08 , 0x00 , 0x08 , 0x00, 0x01 , 0x00, 0x06, 0x00, 0x0C };

#define WRITE_BIT      I2C_MASTER_WRITE
#define READ_BIT       I2C_MASTER_READ
#define ACK_CHECK      true

void sendNibbleCmd(I2CLCDDisplay display, uint8_t lower_nibble){
	uint8_t shifted[2];
	shifted[0] = (lower_nibble <<4);
	shifted[1] = (lower_nibble <<4);
	if (display.Backlight){
		shifted[0] |= 0x08;
		shifted[1] |= 0x08;
	}
	shifted[0] |= 0x04;
	shifted[1] &= 0xFC;

	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
			i2c_master_start(cmd);
			i2c_master_write_byte(cmd, display.Address,false );
			i2c_master_write_byte(cmd,shifted[0],false);
			i2c_master_write_byte(cmd,shifted[1],false);
			i2c_master_stop(cmd);
			i2c_master_cmd_begin(0, cmd, 1000 / portTICK_PERIOD_MS);
			i2c_cmd_link_delete(cmd);
	//HAL_I2C_Master_Transmit(&display.Bus,display.Address,shifted,2,10);
}

void sendNibbleData(I2CLCDDisplay display, uint8_t lower_nibble){
	uint8_t shifted[2];
	shifted[0] = (lower_nibble <<4);
	shifted[1] = (lower_nibble <<4);
	if (display.Backlight){
		shifted[0] |= 0x08;
		shifted[1] |= 0x08;
	}
	shifted[0] |= 0x05;
	shifted[1] &= 0xFC;
	i2c_cmd_handle_t cmd = i2c_cmd_link_create();
		i2c_master_start(cmd);
		i2c_master_write_byte(cmd, display.Address,false );
		i2c_master_write_byte(cmd,shifted[0],false);
		i2c_master_write_byte(cmd,shifted[1],false);
		i2c_master_stop(cmd);
		i2c_master_cmd_begin(0, cmd, 1000 / portTICK_PERIOD_MS);
		i2c_cmd_link_delete(cmd);
	//HAL_I2C_Master_Transmit(&display.Bus,display.Address,shifted,2,10);
}

void initI2C(){

	int i2c_master_port = 0;

	i2c_config_t conf = {
	    .mode = I2C_MODE_MASTER,
	    .sda_io_num = 1,         // select SDA GPIO specific to your project
	    .sda_pullup_en = GPIO_PULLUP_ENABLE,
	    .scl_io_num = 0,         // select SCL GPIO specific to your project
	    .scl_pullup_en = GPIO_PULLUP_ENABLE,
	    .master.clk_speed = 100000,  // select frequency specific to your project
	    .clk_flags = 0,                          // optional; you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here
	};
	i2c_param_config(i2c_master_port, &conf);
	i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);

}


void I2CLCD_WriteLine(I2CLCDDisplay display, uint8_t lineNumber,char *data){
	uint8_t offset=0;
	switch (lineNumber){
	case 0:
		offset = 0;
		break;
	case 1:
		offset = 40;
		break;
	case 2:
		offset = 20;
		break;
	case 3:
		offset = 84;
		break;
	}
	offset |=0x80;
	sendNibbleCmd(display,(offset>>4));
	sendNibbleCmd(display,offset);
	uint8_t *pointer = (uint8_t*)data;
	while (*pointer != '\0'){
		sendDataByte(display,*pointer);
		pointer++;
	}
}


void sendDataByte(I2CLCDDisplay display, uint8_t byte ){
	sendNibbleData(display,(byte>>4));
	sendNibbleData(display,(byte));
}

void I2CLCD_Init(I2CLCDDisplay display ){

		for(int i=0;i<sizeof(initArray);i++){
			sendNibbleCmd(display,initArray[i]);
			ets_delay_us(5000);
		}
}

void I2CLCD_InitPort(){

	int i2c_master_port = 0;

	i2c_config_t conf = {
	    .mode = I2C_MODE_MASTER,
	    .sda_io_num = 1,         // select SDA GPIO specific to your project
	    .sda_pullup_en = GPIO_PULLUP_ENABLE,
	    .scl_io_num = 0,         // select SCL GPIO specific to your project
	    .scl_pullup_en = GPIO_PULLUP_ENABLE,
	    .master.clk_speed = 100000,  // select frequency specific to your project
	    .clk_flags = 0,                          // optional; you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here
	};
	i2c_param_config(i2c_master_port, &conf);
	i2c_driver_install(i2c_master_port, conf.mode, 0, 0, 0);

}
