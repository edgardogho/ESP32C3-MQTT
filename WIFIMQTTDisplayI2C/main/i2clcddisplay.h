/*
 * i2clcddisplay.h
 *
 *  Created on: Jun 15, 2023
 *      Author: edgardog
 */

#ifndef MAIN_I2CLCDDISPLAY_H_
#define MAIN_I2CLCDDISPLAY_H_

#include "globales.h"
typedef struct I2CLCDDisplay {
	uint8_t Address;
	uint8_t Backlight;
}I2CLCDDisplay;

void sendNibbleCmd(I2CLCDDisplay display, uint8_t lower_nibble);
void sendNibbleData(I2CLCDDisplay display, uint8_t lower_nibble);
void sendDataByte(I2CLCDDisplay display, uint8_t byte );
void I2CLCD_Init(I2CLCDDisplay);
void I2CLCD_InitPort();
void I2CLCD_WriteLine(I2CLCDDisplay display, uint8_t lineNumber,char *data);


#endif /* MAIN_I2CLCDDISPLAY_H_ */
