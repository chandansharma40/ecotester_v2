/*
 * LCD.h
 *
 * Created: 1/18/2017 12:12:17 PM
 *  Author: Chandan
 */ 


#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>

void LCDInit();
void LCDclr();
void LCDhome();
void LCDChar(uint8_t data);
void LCDString(char *str);
void LCDCmd(uint8_t cmd);
void LCDGotoXY(uint8_t x, uint8_t y);

#endif /* LCD_H_ */