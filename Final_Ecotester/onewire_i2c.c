/*
 * onewire_i2c.c
 *
 * Created: 11/19/2016 7:01:45 PM
 *  Author: Chandan
 */ 
#define F_CPU	16000000L

#include <avr/io.h>
#include <stdio.h>
#include <stdbool.h>
#include <util/delay.h>
#include "i2c_master.h"
#include "onewire_i2c.h"
#include "LCD.h"
#include <util/crc16.h>

uint8_t ds2482_reset(uint8_t add)
{
	uint8_t rec;
	
	i2c_start(add|WRITE);
	i2c_write(0xF0);
	i2c_start(add|READ);
	rec = i2c_read_nack();
	i2c_stop();
	
	if((rec & 0xF7) != 0x10){
		return 0;
	}
	else{
	return 1;}
}

uint8_t ds2482_channel_select(uint8_t add,int channel)
{
	uint8_t status, ch, ch_read;
	
	i2c_start(add|WRITE);
	i2c_write(0xC3);
	
	switch (channel)
	{
		default:case 0: ch = 0xF0; ch_read = 0xB8; break;
				case 1: ch = 0xE1; ch_read = 0xB1; break;
				case 2: ch = 0xD2; ch_read = 0xAA; break;
				case 3: ch = 0xC3; ch_read = 0xA3; break;
				case 4: ch = 0xB4; ch_read = 0x9C; break;
				case 5: ch = 0xA5; ch_read = 0x95; break;
				case 6: ch = 0x96; ch_read = 0x8E; break;
				case 7: ch = 0x87; ch_read = 0x87; break;
	};
	
	i2c_write(ch);
	i2c_start(add|READ);
	status = i2c_read_nack();
	i2c_stop();
	
	if (status != ch_read)
	{
		return 0;
	}
	else{
		return 1;
	}
}

uint8_t ds2482_write_cfg(uint8_t add,uint8_t config)
{
	uint8_t rec;
	
	i2c_start(add|WRITE);
	i2c_write(0xD2);
	i2c_write(config|(~(config)<<4));
	i2c_start(add|READ);
	rec = i2c_read_nack();
	i2c_stop();
	
	if(rec != config){
		return 0;
	}
	else{
		return 1;
	}
}

uint8_t onewire_reset(uint8_t add)
{
	uint8_t status, status1;
	int poll_count = 0, poll_limit = 20;
	
	i2c_start(add|WRITE);
	i2c_write(0xB4);
	i2c_start(add|READ);
	status = i2c_read_ack();
	do
	{
		status = i2c_read_ack();
	} while ((status & 0x01) && (poll_count++ < poll_limit));
	
	status1 = i2c_read_nack();
	i2c_stop();
	
	if (poll_count >= poll_limit)
	{
		ds2482_reset(add);
		return 0;
	}
	
	if (status1 & PPD)
	return 1;
	else
	return 0;
}

uint8_t onewire_write(uint8_t add, uint8_t sendbyte)
{
	uint8_t status, status1;
	int poll_count=0, poll_limit=16;
	
	i2c_start(add|WRITE);
	i2c_write(0xA5);
	i2c_write(sendbyte);
	i2c_start(add|READ);
	status = i2c_read_ack();
	do
	{
		status = i2c_read_ack();
	} while ((status & 0x01) && (poll_count++ < poll_limit));
	
	status1 = i2c_read_nack();
	i2c_stop();
	
	if (poll_count >= poll_limit)
	{
		ds2482_reset(add);
		return 0;
	}
	else{
		return 1;
	}
}

uint8_t onewire_read(uint8_t add)
{
	uint8_t rec, status, status1;
	int poll_count=0, poll_limit=16;
	
	i2c_start(add|WRITE);
	i2c_write(0x96);
	i2c_start(add|READ);
	status = i2c_read_ack();
	do
	{
		status = i2c_read_ack();
	} while ((status & STATUS_1WB) && (poll_count++ < poll_limit));
	
	rec = i2c_read_nack();
	
	if (poll_count >= poll_limit)
	{
		ds2482_reset(add);
		return 0;
	}
	
	i2c_start(add|WRITE);
	i2c_write(0xE1);
	i2c_write(0xE1);
	i2c_start(add|READ);
	status1 = i2c_read_nack();
	i2c_stop();
	
	return status1;
}

uint8_t ds2482_init(uint8_t add)
{
	uint8_t rec;
	
	rec = ds2482_reset(add);
	if (!rec)	return 0;
	
	rec = ds2482_write_cfg(add,0x01);
	if (!rec)	return 0;
	
	return 1;
}

uint8_t get_temp(uint8_t add,int channel,int16_t* temp)
{
	uint8_t rec, tmpr[9];
	
	rec = ds2482_channel_select(add,channel);
	if (!rec)
	{
		return 0;
	}
	
	rec = onewire_reset(add);
	if (!rec)
	{
		return 0;
	}
	
	rec = onewire_write(add,0xCC);
	if (!rec)
	{
		return 0;
	}
	
	rec = onewire_write(add,0x44);
	if (!rec)
	{
		return 0;
	}
	
	_delay_ms(1000);
	
	rec = onewire_reset(add);
	if (!rec)
	{
		return 0;
	}
	
	rec = onewire_write(add,0xCC);
	if (!rec)
	{
		return 0;
	}
	
	rec = onewire_write(add,0xBE);
	if (!rec)
	{
		return 0;
	}
	
	tmpr[0] = onewire_read(add);
	tmpr[1] = onewire_read(add);
	tmpr[2] = onewire_read(add);
	tmpr[3] = onewire_read(add);
	tmpr[4] = onewire_read(add);
	tmpr[5] = onewire_read(add);
	tmpr[6] = onewire_read(add);
	tmpr[7] = onewire_read(add);
	tmpr[8] = onewire_read(add);
	
	rec = onewire_reset(add);
	if (!rec)	return 0;
	
	uint8_t crc = 0x00;
	uint8_t b=0;
	
	for (b=0; b<8; b++)
	{
		crc = _crc_ibutton_update(crc, tmpr[b]);
	}
	
	if ((tmpr[8] == crc) && (crc != 0xFF))
	{

		float f_temp = ( (tmpr[1] << 8) + tmpr[0] )*0.0625;
		*temp = ceil(f_temp*10);
		
		return 1;
	}
	else
	{
		return 0;
	}
		
}