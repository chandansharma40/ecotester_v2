/*
 * UART1.h
 *
 * Created: 1/18/2017 12:01:13 PM
 *  Author: Chandan
 */ 


#ifndef UART1_H_
#define UART1_H_

#define BAUD	9600

void UART1Init();
void UART1Char(uint8_t data);
void UART1String(char *str);

#endif /* UART1_H_ */