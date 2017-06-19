/*
 * UART0.c
 *
 * Created: 2/4/2017 1:32:29 PM
 *  Author: Chandan
 */ 

#define F_CPU	16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "UART0.h"

void UART0Init()
{
	UCSR0A = 0x00;
	UCSR0B = 0x00;
	
	UBRR0H = (unsigned char) (((F_CPU/(BAUD*16UL))-1)>>8) ;
	UBRR0L = (unsigned char) (F_CPU/(BAUD*16UL))-1;
	
	UCSR0C = 0b00000110;
	UCSR0B = 0b10011000;
}

void UART0Char(uint8_t data)
{
	while ( !( UCSR0A & (1<<UDRE)));
	UDR0 = data;
}

void UART0String(char *str)
{
	while( *str != '\0' )
	{
		UART0Char(*str++);
	}
}