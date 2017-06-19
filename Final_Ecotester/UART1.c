/*
 * UART1.c
 *
 * Created: 1/18/2017 12:03:12 PM
 *  Author: Chandan
 */ 

#define F_CPU	16000000UL

#include <avr/io.h>
#include <util/delay.h>
#include "UART1.h"

void UART1Init()
{
	UCSR1A = 0x00;
	UCSR1B = 0x00;
	
	UBRR1H = (unsigned char) (((F_CPU/(BAUD*16UL))-1)>>8) ;
	UBRR1L = (unsigned char) (F_CPU/(BAUD*16UL))-1;
	
	UCSR1C = 0b00000110;
	UCSR1B = 0b10011000;
}

void UART1Char(uint8_t data)
{
	while ( !( UCSR1A & (1<<UDRE)));
	UDR1 = data;
}

void UART1String(char *str)
{
	while( *str != '\0' )
	{
		UART1Char(*str++);
	}
}