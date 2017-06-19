/*
 * UART0.h
 *
 * Created: 2/4/2017 1:32:14 PM
 *  Author: Chandan
 */ 


#ifndef UART0_H_
#define UART0_H_

#define BAUD	9600

void UART0Init();
void UART0Char(uint8_t data);
void UART0String(char *str);
#endif /* UART0_H_ */