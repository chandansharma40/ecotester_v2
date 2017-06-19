/*
 * LCD.c
 *
 * Created: 1/18/2017 12:16:40 PM
 *  Author: Chandan
 */ 
#define F_CPU	16000000UL

#define DATAPORT	PORTA		//Data Port
#define CMDPORT		PORTC		//RS and EN Port
#define DATADDR		DDRA		//DDR for Data Port
#define CMDDDR		DDRC		//DDR for Command Port
#define RS			PC7			//RS Pin
#define EN			PC6			//EN Pin

#define LCD_LINE0_DDRAMADDR		0x00
#define LCD_LINE1_DDRAMADDR		0x40
#define LCD_LINE2_DDRAMADDR		0x14
#define LCD_LINE3_DDRAMADDR		0x54

#define LCD_DDRAM				7

#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include <compat/deprecated.h>

void LCDCmd(uint8_t cmd)
{
	cbi(CMDPORT, RS);
	DATAPORT = cmd;
	
	sbi(CMDPORT,EN);
	_delay_ms(5);
	cbi(CMDPORT,EN);
	
	_delay_ms(10);
}

void LCDChar(uint8_t data)
{
	sbi(CMDPORT, RS);
	DATAPORT = data;
	
	sbi(CMDPORT,EN);
	_delay_ms(5);
	cbi(CMDPORT,EN);
	
	_delay_ms(10);
}

void LCDString(char *str)
{
	while( *str != '\0')
	{
		LCDChar(*str++);
	}
}

void LCDInit()
{
	DATADDR = 0xFF;
	CMDDDR = (1<<RS) | (1<<EN);
	
	LCDCmd(0x38);
	_delay_ms(10);
	LCDCmd(0x01);
	_delay_ms(10);
	LCDCmd(0x06);
	_delay_ms(10);
	LCDCmd(0x0C);
	_delay_ms(10);
	LCDCmd(0x80);
	_delay_ms(10);
}

void LCDclr()
{
	LCDCmd(0x01);
}

void LCDhome()
{
	LCDCmd(0x02);
}

void LCDGotoXY(uint8_t x, uint8_t y)
{
	register uint8_t DDRAMAddr;
	// remap lines into proper order
	switch(y)
	{
		case 0	: DDRAMAddr = LCD_LINE0_DDRAMADDR+x; break;
		case 1	: DDRAMAddr = LCD_LINE1_DDRAMADDR+x; break;
		case 2	: DDRAMAddr = LCD_LINE2_DDRAMADDR+x; break;
		case 3	: DDRAMAddr = LCD_LINE3_DDRAMADDR+x; break;
		default	: DDRAMAddr = LCD_LINE0_DDRAMADDR+x;
	}
	
	LCDCmd(1<<LCD_DDRAM | DDRAMAddr);
}