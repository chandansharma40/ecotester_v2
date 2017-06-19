/*
 * onewire_i2c.h
 *
 * Created: 11/19/2016 7:01:20 PM
 *  Author: Chandan
 */ 


#ifndef ONEWIRE_I2C_H_
#define ONEWIRE_I2C_H_

#define SHORTDETECT 0x04
#define PPD			0x02
#define STATUS_1WB	0x01

#define WRITE		0x00
#define READ		0x01

uint8_t ds2482_reset(uint8_t add);
uint8_t ds2482_channel_select(uint8_t add,int channel); 
uint8_t ds2482_write_cfg(uint8_t add,uint8_t config);
uint8_t onewire_reset(uint8_t add);
uint8_t onewire_write(uint8_t add,uint8_t sendbyte);
uint8_t onewire_read(uint8_t add);

uint8_t ds2482_init(uint8_t add);
uint8_t get_temp(uint8_t add,int channel,int16_t* temp);

#endif /* ONEWIRE_I2C_H_ */