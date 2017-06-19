/*
 * Final_Ecotester.c
 *
 * Created: 6/14/2017 2:42:11 PM
 * Author : Chandan
 */ 
#define F_CPU			16000000UL

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <math.h>
#include <util/delay.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <compat/deprecated.h>
#include <avr/eeprom.h>

#include "DB18S20.h"
#include "LCD.h"
#include "i2c_master.h"
#include "onewire_i2c.h"
#include "UART0.h"
#include "UART1.h"

#define CHECKBIT(x,b)	x&(1<<b)
#define NumberOfAttempts	6
#define MASTER_NUMBER	"+918149597213\0"

#define SW1				CHECKBIT(PINC,5)
#define SW2				CHECKBIT(PINC,4)
#define SW3				CHECKBIT(PINC,3)
#define SW4				CHECKBIT(PINC,2)
#define SW5				CHECKBIT(PINC,1)
#define SW6				CHECKBIT(PINC,0)
#define SW7				CHECKBIT(PIND,7)
#define SW8				CHECKBIT(PIND,6)
#define SW9				CHECKBIT(PIND,5)
#define SW10			CHECKBIT(PIND,4)

#define DS2482_1_ADDR	0x30
#define DS2482_2_ADDR	0x32
#define DS2482_3_ADDR	0x34

#define BB1				PB6
#define BB2				PB7
#define BB3				PG3
#define BB4				PG4

void allsensor_datalog();
void set_rtc_time();
void check_error_sim800();

//  Global variables for Timer
uint8_t skip=0,overflowcount=0;

//Global Variable for DataFreeze Reset
uint8_t counter_datafreeze=0,flag_datafreeze=0,counter_main=0;
uint8_t no_loc_flag=0;
char str_datafreeze_main[100],str_datafreeze[100];

//  Function Definition
uint8_t get_apn(char* apn,char* user);
void sim900_cmd(char* str,char* response);
uint8_t get_sms(uint8_t* system_on,char* customer_no,char* Serial_num);
uint8_t dataLog(uint8_t system_on,char* apn,char* Serial_num,uint8_t signal_strength,char* user,char* loc_lat,char* loc_long,uint8_t flag_cooldown);
uint8_t fetch_gprs();
void get_data(char* responseimp);
uint8_t set_apn(char* apn);
uint8_t allocate_dynaIP();
uint8_t Log_data(char* responseimp,char* Serial_num,char* loc_lat,char* loc_long);
void sim900_response(char* response);
uint8_t get_signalstrength();
void start_timer();
uint8_t wait_for_data();
void get_loc(char* apn,char* loc_lat,char* loc_long);
void reset_SIM900();

int main(void)
{
    _delay_ms(100);
	
	LCDInit();
	UART0Init();
	UART1Init();
	//i2c_init();
	//ds2482_init(DS2482_1_ADDR);
	
	DDRC = 0b11000000;
	DDRD = 0b00001111;
	
	PORTC = 0xFF;
	PORTD = 0xFF;
	
	LCDclr();
	LCDhome();
	LCDString("   Eco-tester   ");
	LCDGotoXY(0,1);
	LCDString("----------------");
	
	_delay_ms(20000);
	
	
	
	_delay_ms(1);
	sbi(TIMSK,TOIE1);
	_delay_ms(1);
	
	sei();
	
	sbi(UCSR0B,RXEN0);
	
	start_timer();

	cbi(TCCR1B,CS12);

	
    while (1) 
    {
		if (SW10)
		{
			LCDclr();
			LCDhome();
			LCDString("Hello");
			LCDGotoXY(0,1);
			LCDString("Chandan");
		} 
		
		else if (SW9)
		{
			LCDclr();
			LCDhome();
			LCDString("Hello");
			LCDGotoXY(0,1);
			LCDString("World");
		}
    }
}

void allsensor_datalog()
{
	uint8_t flag_get_apn=0,flag_get_sms=0,system_on,signal_strength=0,flag_datalog=0,flag_cooldown=0x89;
	uint8_t dummy,data_counter=0;
	uint16_t e=0,l=0,i=0;
	char Serial_num[20],customer_no[20],apn[30],user[30],responseimp[512],loc_lat[15],loc_long[15];
	
	e=32;l=0;i=0;
	while (l<11){
		Serial_num[i]=eeprom_read_byte((uint8_t*)e);
		i++;e++;l++;
	}
	Serial_num[i]='\0';
	
	while (flag_get_apn!=1){
		flag_get_apn = get_apn(apn,user);
		data_counter++;
		if(data_counter>10){
			// Acquiring data for resetting
			get_data(responseimp);
			data_counter=0;
			
		}
	}
	
	//  Get Location
	get_loc(apn,loc_lat,loc_long);
	
	//  To get signal strength
	signal_strength=get_signalstrength();
	
	// To fetch SMS
	data_counter=NumberOfAttempts; // Number of attempts to be made if SMS fetch fails
	while (data_counter!=0){
		flag_get_sms = get_sms(&system_on,customer_no,Serial_num);
		data_counter--;
		if(flag_get_sms==1){  // SMS Fetch successful
			break;
		}
	}
	
	// Logging Data
	flag_datalog = dataLog(system_on,apn,Serial_num,signal_strength,user,loc_lat,loc_long,flag_cooldown);
	if(flag_datalog==0){
		reset_SIM900();  //  Reset SIM900 to get CGATT=1 sooner
		flag_get_apn=0;
	}
}

uint8_t get_apn(char* apn,char* user){
	 
	uint8_t i=0,p=0;
	char simresponse[200];
	
	UART0String("Checking SIM...\r\n");
	// SIM900 Echo disabled in response
	UART1String("ATE0\r\n\0");  
	_delay_ms(500);
	
	// SIM900 module check
	sim900_cmd("AT\r\n\0",simresponse);
	if (strcmp(simresponse,"OK\0")!=0){
		UART0String("No Response...\r\n");
		UART0String("Rechecking...\r\n");
		return 0;
	}
	UART0String("SIM Checked OK...\r\n");
	UART0String("Checking SimCard...\r\n");
	
	// To check if SIM card is present	 
	sim900_cmd("AT+CSMINS?\r\n\0",simresponse); 
	if (strcmp(simresponse,"+CSMINS: 0\,1\0")!=0){
		UART0String("No Response...\r\n");
		UART0String("Rechecking...\r\n");
		return 0;
	}
	UART0String("SimCard Present...\r\n");
	UART0String("Checking SIMCARD Registration...\r\n");
	
	// To check if SIM card is registered
	sim900_cmd("AT+CREG?\r\n\0",simresponse);  
	if (!((strcmp(simresponse,"+CREG: 0\,1\0")==0) || (strcmp(simresponse,"+CREG: 0\,5\0")==0))){
		UART0String("No Response...\r\n");
		UART0String("Rechecking...\r\n");
		return 0;
	}
	UART0String("SimCard Registered OK...\r\n");
	// Returns name of Network Provider
	sim900_cmd("AT+CSPN?\r\n\0",simresponse);  
	i=0;p=0;
	while (simresponse[i]!='\"' && i<200){
		i++;
	}
	i++;
	while (simresponse[i]!='\"' && p<30){
		user[p]=simresponse[i];
		p++;i++;
	}
	user[p]='\0';
	UART0String("SIM network provider...");
	UART0String(user);
	UART0String("\r\n");
	
	//APN Compare and Assignment
	if (strcmp(user,"Hutch\0")==0){
		strcpy(apn,"www");
		UART0String("APN...");
		UART0String(apn);
		UART0String("\r\n");
		return 1;
	}
	else if ((strcmp(user,"Vodafone\0")==0) || (strcasecmp(user,"Vodafone IN\0")==0)){
		strcpy(apn,"www");
		UART0String("APN...");
		UART0String(apn);
		UART0String("\r\n");
		return 1;
	}
	else if ((strcmp(user,"airtel\0")==0) || (strcmp(user,"Airtel\0")==0)){
		strcpy(apn,"airtelgprs.com");
		UART0String("APN...");
		UART0String(apn);
		UART0String("\r\n");
		return 1;
	}
	else if((strcmp(user,"CellOne\0")==0)||(strcmp(user,"BSNL\0")==0)||(strcmp(user,"BSNL 3G\0")==0)){
		strcpy(apn,"bsnlenet");
		UART0String("APN...");
		UART0String(apn);
		UART0String("\r\n");
		return 1;;
	}
	else if(strcmp(user,"Reliance\0")==0){
		strcpy(apn,"rcomwap");
		UART0String("APN...");
		UART0String(apn);
		UART0String("\r\n");
		return 1;
	}
	else if(strcmp(user,"TATA\0")==0){
		strcpy(apn,"tata.docomo.internet");
		UART0String("APN...");
		UART0String(apn);
		UART0String("\r\n");
		return 1;
	}
	else if(strcmp(user,"Uninor\0")==0){
		strcpy(apn,"uninor");
		UART0String("APN...");
		UART0String(apn);
		UART0String("\r\n");
		return 1;
	}
	else if((strcmp(user,"!dea\0")==0) || (strcasecmp(user,"Idea\0") == 0)){
		strcpy(apn,"isafe");
		UART0String("APN...");
		UART0String(apn);
		UART0String("\r\n");
		return 1; 
	}
	else{
		UART0String("APN not found...\r\n");
		return 0;
	}
}

void sim900_cmd(char* str,char* response){	
	uint8_t i=0;
	char initial[]={'0','0'};

	//  Send Command
	while( *str != '\0' ){
		UART1Char( *str++ );
	}
	
	//  Reception Enable
	sbi(UCSR1B,RXEN1);
	
	sim900_response(response);
}

uint8_t get_sms(uint8_t* system_on,char* customer_no,char* Serial_num){
	
	uint8_t i=0,p=0,dataarrived=0;
	uint8_t update_customer_no=0,update_serial_no=0,update_remote_control=0;
	int no_of_msg,message_number;
	char no_msg[6],response_number[150],contact_no[20],simresponse[30],msg[160],dummy[5];
	char initial[]={'0','0'};
	
	UART0String("Checking no. of unread messages...\r\n");
	// Check no. of messages
	sim900_cmd("AT+CPMS=\"SM\"\r\n\0",simresponse);
	
	//  For re-enabling reception for OK
	sbi(UCSR1B,RXEN1);
	
	sim900_response(dummy);
	
	i=0;p=0;
	while (simresponse[i]!='\:' && i<30){
		i++;
	}
	i++;i++;
	while (simresponse[i]!='\,' && p<6){
		no_msg[p]=simresponse[i];
		p++;i++;
	}
	no_msg[p]='\0';
	no_of_msg = atoi(no_msg);  
	UART0String("no. of unread messages are..");
	UART0String(no_msg);
	UART0String("\r\n");
	
	// Exiting the function if no new messages have arrived
	if (no_of_msg==0){
		return 1;
	}
	
	// Changing the message to readable format
	sim900_cmd("AT+CMGF=1\r\n",simresponse);
	if (strcmp(simresponse,"OK\0")!=0){
		return 0;
	}
	UART0String("Messages converted to Readable Format...\r\n");
	
	// To fetch SMS messages
	message_number=1;
	while (message_number<=no_of_msg){
		initial[0]='0';initial[1]='0';  // Re-initializing initial variable
		
		// Send command for reading the particular message
		itoa(message_number,no_msg,10);
		UART1String("AT+CMGR=");
		UART1String(no_msg);
		UART1String("\r\n");
		
		//  For enabling reception
		sbi(UCSR1B,RXEN1);		
		
		//  For receiving sender number
		while(!(initial[0]==0x0D && initial[1]==0x0A)){
			initial[0]=initial[1];
			dataarrived=wait_for_data();
			if (dataarrived==0){
				break;
			}
			initial[1]=UDR1;
		}

		i=0;
		
		while(1){
			dataarrived=wait_for_data();
			if (dataarrived==0){
				break;
			}
			response_number[i]=UDR1;
			if(response_number[i]==0x0D || i>=150){
				break;
			}
			i++;
		}
		
		//  Terminating strings
		response_number[i]="\0";
		
		//  For those odd unexplainable cases where this response comes empty with OK
		if (strcmp(response_number,"OK\0")==0){
			message_number++;
			continue;
		}
		
		//  Receiving message
		p=0;
		
		dataarrived=wait_for_data();
		if (dataarrived==1){
			initial[1]=UDR1;  // To eliminate Lf that comes after Cr but before message starts;
		}
		
		while(1){
			dataarrived=wait_for_data();
			if (dataarrived==0){
				break;
			}
			msg[p]=UDR1;
			if(msg[p]==0x0D || p>=160){
				break;
			}
			p++;
		}
		
		//  For receiving final OK
		sim900_response(dummy);

		//  Terminating strings
		msg[p]="\0";	
		
		// Figuring out Phone number of SMS sender
		i=0;p=0;
		while (response_number[i]!='\"' && i<150){
			i++;
		}
		i++;
		while (response_number[i]!='\"' && i<150){
			i++;
		}
		i++;
		while (response_number[i]!='\"' && i<150){
			i++;
		}
		i++;
		while (response_number[i]!='\"' && p<20 && i<150){
			contact_no[p]=response_number[i];
			p++;i++;
		}
		contact_no[p]='\0';
		
		// If message doesn't match authenticated source, move to next message
		if (!((strcmp(contact_no,customer_no)==0) || (strcmp(contact_no,MASTER_NUMBER)==0))){
			message_number++;
			continue;
		}
		
		if ((msg[0]=='+') && (strcmp(contact_no,MASTER_NUMBER)==0)){  //  Changing customer number by Master
			strcpy(customer_no,msg);
			UART0String("Customer No. changed to ");
			UART0String(customer_no);
			UART0String("\r\n");
			update_customer_no=1;
		}
		else if ((strcasecmp(msg,"on")==0) && ((strcmp(contact_no,MASTER_NUMBER)==0) || (strcmp(contact_no,customer_no)==0))){  //  Commanding system to Switch on by Customer/Master
			*system_on=0x31;
			UART0String("System Turned ON....\r\n");
			sbi(PORTB,7);
			update_remote_control=1;
		}
		else if((strcasecmp(msg,"off")==0) && ((strcmp(contact_no,MASTER_NUMBER)==0) || (strcmp(contact_no,customer_no)==0))){  //  Commanding system to Switch off by Customer/Master
			*system_on=0x30;
			UART0String("System Turned OFF....\r\n");
			cbi(PORTB,7);
			update_remote_control=1;
		}
		else if ((strncmp(msg,"SNO+E",5)==0) && (strcmp(contact_no,MASTER_NUMBER)==0)){  //  To change serial number by Master
			strncpy(Serial_num,msg+4,11);
			
			UART0String("Serial No. changed to ");
			UART0String(Serial_num);
			UART0String("\r\n");
			update_serial_no=1;
		}
	
		message_number++;
	}
	
	// To update customer number in EEPROM
	if (update_customer_no){
		i=0;
		while (i<=12){
			eeprom_update_byte((uint8_t*)(i+16),customer_no[i]);
			_delay_ms(5);  //  EEPROM write time is 3.3ms in data sheet
			i++;
		}
	}
	
	// To update serial number in EEPROM
	if (update_serial_no){
		i=0;
		while (i<=10){
			eeprom_update_byte((uint8_t*)(i+32),Serial_num[i]);
			_delay_ms(5);  //  EEPROM write time is 3.3ms in data sheet
			i++;
		}
	}
	
	// To update system on/off status in EEPROM
	if (update_remote_control){
		eeprom_update_byte((uint8_t*)30,*system_on);
		_delay_ms(5);  //  EEPROM write time is 3.3ms in data sheet
	}
		
	// To delete all messages
	sim900_cmd("AT+CMGDA=\"DEL READ\"\r\n",simresponse);
	UART0String("Deleting Read messages...\r\n");
	
	return 1;
}

uint8_t dataLog(uint8_t system_on,char* apn,char* Serial_num,uint8_t signal_strength,char* user,char* loc_lat,char* loc_long,uint8_t flag_cooldown){
	uint8_t flag_fetch_gprs=0,flag_set_apn=0,flag_allocate_dynaIP=0,flag_Log_data=0,flag_get_apn=0;
	uint8_t data_counter=0;
	char responseimp[512];
	
//	no_loc_flag++;
	
	// To acquire GPRS settings
	data_counter=NumberOfAttempts;  // Number of attempts to be made if APN fetch fails
	while (data_counter!=0){
		flag_fetch_gprs = fetch_gprs();
		data_counter--;
		if(flag_fetch_gprs==1){  // GPRS settings Fetch successful
			break;
		}
		else if(data_counter==0){  // Number of attempts exhausted
			return 0;
		}	
	}
	
	//For Testing
	//strcpy(responseimp,"A01=1&A02=2");
	
	// Acquiring data for logging
	get_data(responseimp);
	// Acquiring data for logging and setting APN
	data_counter=NumberOfAttempts;  // Number of attempts to be made if APN set fails
	while (data_counter!=0){
		//Setting APN
		flag_set_apn = set_apn(apn);
		data_counter--;
		if(flag_set_apn==1){  // APN set successful
			break;
		}
		else if(data_counter==0){  // Number of attempts exhausted
			return 0;
		}		
	}
	
	// Data Logging
	data_counter=NumberOfAttempts;  // Number of attempts to be made if Dynamic IP allocation fails
	while (data_counter!=0){
		flag_Log_data = Log_data(responseimp,Serial_num,loc_lat,loc_long);
		data_counter--;
		if(flag_Log_data==1){  // Data Logging successful
			break;
		}
		else if(data_counter==0){  // Number of attempts exhausted
			return 1;
		}		
	}
	
	return 1;
}

uint8_t fetch_gprs(){
	char simresponse[20];
	UART0String("Searching GPRS...\r\n");
	
	sim900_cmd("AT+CGATT?\r\n",simresponse);
	if(strcmp(simresponse,"+CGATT: 0\0")==0){  // If GPRS is not attached
		UART0String("GPRS returns ERROR...\r\n");
		_delay_ms(5000);
	}else if(strcmp(simresponse,"+CGATT: 1\0")==0){  // GPRS is attached properly
		//  Reception Enable again for OK
		sbi(UCSR1B,RXEN1);
		UART0String("GPRS OK...\r\n");
		sim900_response(simresponse);
		return 1;
	}
	return 0;
}

void get_data(char* responseimp){
	
	LCDclr();
	LCDhome();
	LCDString("Fetching DATA...");
	
	uint8_t tes_count=0;
	int16_t TES1=0, TES2=0, TES3=0, TES4=0, TES5=0, TES6=0, TES7=0, TES8=0, TES9=0, TES10=0, TES11=0, TES12=0, TES13=0, TES14=0, TES15=0, TES16=0, TES17=0, TES18=0;
	int16_t TES19=0, TES20=0, TES21=0, TES22=0, TES23=0, TES24=0, TES25=0, TES26=0, TES27=0, TES28=0;
	char t1[6]="0.0", t2[6]="0.0", t3[6]="", t4[6]="0.0", t5[6]="0.0", t6[6]="0.0", t7[6]="0.0", t8[6]="", t9[6]="0.0", t10[6]="0.0", t11[6]="0.0", t12[6]="0.0", t13[6]="0.0";
	char t14[6]="0.0", t15[6]="0.0", t16[6]="0.0", t17[6]="0.0", t18[6]="0.0", t19[6]="0.0", t20[6]="0.0", t21[6]="0.0", t22[6]="0.0", t23[6]="0.0", t24[6]="0.0", t25[6]="0.0";
	char t26[6]="0.0", t27[6]="0.0", t28[6]="0.0";
	uint16_t p1=0, p2=0, p3=0, p4=0, p5=0, p6=0, p7=0, p8=0;
	char pres1[6]="0.0", pres2[6]="0.0", pres3[6]="0.0", pres4[6]="0.0", pres5[6]="0.0" , pres6[6]="0.0" , pres7[6]="0.0" , pres8="0.0"  ;
	
	ADMUX  = 0b01000000;
	_delay_ms(2);
	ADCSRA |= (1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	p1=ADC;
	double pressure1 = ((((double)p1-181.0)*0.0556)+0.8563);
	
	if(pressure1<0)
	{
		pressure1=0;
	}
	dtostrf(pressure1,2,2,pres1);
	
	
	ADMUX  = 0b01000001;
	_delay_ms(2);
	ADCSRA |= (1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	p2=ADC;
	double pressure2 = ((((double)p2-181.0)*0.0556)+0.8563);
	
	if(pressure2<0)
	{
		pressure2=0;
	}
	dtostrf(pressure2,2,2,pres2);
	
	
	ADMUX  = 0b01000010;
	_delay_ms(2);
	ADCSRA |= (1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	p3=ADC;
	double pressure3 = ((((double)p3-181.0)*0.0556)+0.8563);
	
	if(pressure3<0)
	{
		pressure3=0;
	}
	dtostrf(pressure3,2,2,pres3);
	
	
	ADMUX  = 0b01000011;
	_delay_ms(2);
	ADCSRA |= (1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	p4=ADC;
	double pressure4 = ((((double)p4-181.0)*0.0556)+0.8563);
	
	if(pressure4<0)
	{
		pressure4=0;
	}
	dtostrf(pressure4,2,2,pres4);
	
	
	ADMUX  = 0b01000100;
	_delay_ms(2);
	ADCSRA |= (1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	p5=ADC;
	double pressure5 = ((((double)p5-181.0)*0.0556)+0.8563);
	
	if(pressure5<0)
	{
		pressure5=0;
	}
	dtostrf(pressure5,2,2,pres5);
	
	
	ADMUX  = 0b01000101;
	_delay_ms(2);
	ADCSRA |= (1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	p6=ADC;
	double pressure6 = ((((double)p6-181.0)*0.0556)+0.8563);
	
	if(pressure6<0)
	{
		pressure6=0;
	}
	dtostrf(pressure6,2,2,pres6);
	
	
	ADMUX  = 0b01000110;
	_delay_ms(2);
	ADCSRA |= (1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	p5=ADC;
	double pressure7 = ((((double)p7-181.0)*0.0556)+0.8563);
	
	if(pressure7<0)
	{
		pressure7=0;
	}
	dtostrf(pressure7,2,2,pres7);
	
	
	ADMUX  = 0b01000111;
	_delay_ms(2);
	ADCSRA |= (1<<ADSC);
	while(!(ADCSRA & (1<<ADIF)));
	ADCSRA|=(1<<ADIF);
	p5=ADC;
	double pressure8 = ((((double)p8-181.0)*0.0556)+0.8563);
	
	if(pressure8<0)
	{
		pressure8=0;
	}
	
	dtostrf(pressure8,2,2,pres8);
	
	
	for (tes_count=0;tes_count<20;tes_count++)
	{
		if(therm_read_temperature(PG4, &TES28) != -1)
		{
			break;
			}else{
			TES28=0;
		}
	}
	
	float tes28 = (double)TES28/10.0;
	dtostrf(tes28,3,1,t28);
	
	strcpy(responseimp,"A09=");
	strcat(responseimp,t1);
	strcat(responseimp,"&A14=");
	strcat(responseimp,t2);
	strcat(responseimp,"&A01=");
	strcat(responseimp,t28);
	strcat(responseimp,"&A12=");
	strcat(responseimp,t4);
	strcat(responseimp,"&A13=");
	strcat(responseimp,t5);
	strcat(responseimp,"&A04=");
	strcat(responseimp,t6);
	strcat(responseimp,"&A05=");
	strcat(responseimp,t7);
	strcat(responseimp,"&A06=");
	strcat(responseimp,t8);
	strcat(responseimp,"&A07=");
	strcat(responseimp,t9);
	strcat(responseimp,"&A08=");
	strcat(responseimp,t10);
	strcat(responseimp,"&A02=");
	strcat(responseimp,t11);
	strcat(responseimp,"&A10=");
	strcat(responseimp,t12);
	strcat(responseimp,"&A11=");
	strcat(responseimp,t13);
	strcat(responseimp,"&A03=");
	strcat(responseimp,t14);
	strcat(responseimp,"&A15=");
	strcat(responseimp,t15);
	strcat(responseimp,"&A16=");
	strcat(responseimp,t16);
	strcat(responseimp,"&A17=");
	strcat(responseimp,t17);
	strcat(responseimp,"&A18=");
	strcat(responseimp,t18);
	strcat(responseimp,"&A19=");
	strcat(responseimp,t19);
	strcat(responseimp,"&A20=");
	strcat(responseimp,t20);
	strcat(responseimp,"&A21=");
	strcat(responseimp,t21);
	strcat(responseimp,"&A22=");
	strcat(responseimp,t22);
	strcat(responseimp,"&A23=");
	strcat(responseimp,t23);
	strcat(responseimp,"&A24=");
	strcat(responseimp,t24);
	strcat(responseimp,"&A25=");
	strcat(responseimp,t25);
	strcat(responseimp,"&A26=");
	strcat(responseimp,t26);
	strcat(responseimp,"&A27=");
	strcat(responseimp,t27);
	strcat(responseimp,"&A28=");
	strcat(responseimp,t28);
	strcat(responseimp,"&A29=");
	strcat(responseimp,pres1);
	strcat(responseimp,"&A30=");
	strcat(responseimp,pres2);
	strcat(responseimp,"&A31=");
	strcat(responseimp,pres3);
	strcat(responseimp,"&A32=");
	strcat(responseimp,pres4);
	strcat(responseimp,"&A33=");
	strcat(responseimp,pres5);
	strcat(responseimp,"&A34=");
	strcat(responseimp,pres6);
	strcat(responseimp,"&A35=");
	strcat(responseimp,pres7);
	strcat(responseimp,"&A36=");
	strcat(responseimp,pres8);
}

uint8_t set_apn(char* apn){
	char simresponse[10];
	uint8_t flag_allocate_dynaIP=0;
	
	UART0String("Closing PDP Settings...\r\n");
	//  Closing PDP connection
	sim900_cmd("AT+CIPSHUT\r\n\0",simresponse);	
	if (strcmp(simresponse,"SHUT OK\0")!=0){  //  Previous PDP connection not closed properly
		UART0String("PDP connection not closed properly...\r\n");
		UART0String("Re-shutting...\r\n");
		return 0;
	}
	UART0String("PDP Settings closed Properly...\r\n");
	UART0String("Setting apn to ");
	UART0String(apn);
	UART0String("\r\n");
	
	//  Reception Enable
	sbi(UCSR1B,RXEN1);
	
	//  Setting APN
	UART1String("AT+CSTT=\"");  //  To assign the APN username and password
	UART1String(apn);
	UART1String("\"\,\"\"\,\"\"\r\n");
		
	// Getting response
	sim900_response(simresponse);
	
	if (strcmp(simresponse,"OK\0")!=0){  //  Not good
		UART0String("APN not set properly...\r\n");
		UART0String("Re-setting APN...\r\n");
		return 0;
	}
	UART0String("APN set properly...\r\n");
	UART0String("Allocating Dynamic IP...\r\n");
	// Allocating dynamic IP
	flag_allocate_dynaIP = allocate_dynaIP();
	if(flag_allocate_dynaIP==1){  // Dynamic IP allocation successful
		UART0String("Dynamic IP Allocated successfully...\r\n");
		return 1;
	}
	
		
	return 0;
}

uint8_t allocate_dynaIP(){
	char simresponse[20];
	
	//  To attach to GPRS connection
	sim900_cmd("AT+CIICR\r\n\0",simresponse);
	if (strcmp(simresponse,"OK\0")==0){
		//  To allocate dynamic IP
		sim900_cmd("AT+CIFSR\r\n\0",simresponse);
		if (strcmp(simresponse,"ERROR\0")!=0){
			return 1;
		}
	}
	return 0;
} 

uint8_t Log_data(char* responseimp,char* Serial_num,char* loc_lat,char* loc_long){
	char simresponse[512],var_remote,dummy;
	uint8_t tabname_length=0,dataarrived=0,e=32,l=0,i=0;
	
	//  If Serial_num gets erased mysteriously
	tabname_length=strlen(Serial_num);
	if (tabname_length!=11){
		while (l<11){
			Serial_num[i]=eeprom_read_byte((uint8_t*)e);
			i++;e++;l++;
		}
		Serial_num[i]='\0';
	}
	UART0String("Starting CHIP...\r\n");
	UART0String(Serial_num);
	UART0String("\r\n");
	
	//  GPRS services connected and getting ready to Log
	sim900_cmd("AT+CIPSTART=\"TCP\"\,\"52.74.151.81\"\,\"80\"\r\n\0",simresponse);
	if (strcmp(simresponse,"OK\0")==0){
		
		//  Enabling data reception again to verify connection
		sbi(UCSR1B,RXEN1);
		
		//  Verifying connection
		sim900_response(simresponse);
		UART0String("Sending Data...\r\n");
		
		//  Start sending the data							
		UART1String("AT+CIPSEND\r\n\0");
		_delay_ms(500);

		// Sending data
		UART1String("PUT /datalogging/write.php?tabname=\0");
 		UART1String(Serial_num);
 		UART1Char('&');
		UART1String(responseimp);
		
// 		UART1String("&A46=");
//  		UART1String(loc_lat);
//  		UART1String("&A47=");
//  		UART1String(loc_long);
		UART1String(" HTTP/1.1\r\n\0");
		UART1String("Host:52.74.151.81\r\nAccept: */*\r\nAccept-Language: en-us\r\nConnection: Keep-Alive\r\n\r\n\x1A\0");
		
		//  Enabling data reception
		sbi(UCSR1B,RXEN1);
		
		//  Disabling data reception UART0
		cbi(UCSR0B,RXEN0);
		
		while (dummy!='$'){
			dataarrived=wait_for_data();
			if (dataarrived==0){
				break;
			}
			dummy=UDR1;
		}
		dataarrived=wait_for_data();
		var_remote=UDR1;
		
		//  Enabling data reception UART0
		sbi(UCSR0B,RXEN0);
		if (dummy=='$'){
			UART0String("Data Sent Successfully to ");
			UART0String(Serial_num);
			UART0String("\r\n");
		}
		else{
			UART0String("Data Sent returns ERROR...\r\n");
			UART0String("Check String or SIMCARD balance...\r\n");
		}
		if (var_remote=='1'){
			cli();
			flag_datafreeze=1;
			sei();
		}
		else if(var_remote=='2'){
			cli();
			flag_datafreeze=0;
			sei();
		}
				
		return 1;
	}
	
	return 0;
}

void sim900_response(char* response){
	uint8_t i=0,dataarrived=0;
	char initial[]={'0','0'};
		
	while(!(initial[0]==0x0D && initial[1]==0x0A)){
		initial[0]=initial[1];
		dataarrived=wait_for_data();
		if (dataarrived==0){
			UART0String("No Response...\r\n");
			UART0String("Re-Checking...\r\n");
			break;
		}
		initial[1]=UDR1;
	}

	while(1){		
		dataarrived=wait_for_data();
		if (dataarrived==0){
			break;
		}
		response[i]=UDR1;
		if(response[i]==0x0D){
			break;
		}
		i++;
	}

	//  Terminating string
	response[i]="\0";

	//  Reception Disable
	cbi(UCSR1B,RXEN1);
} 

uint8_t get_signalstrength(){
	char dummy[5],strength[3],simresponse[200];
	uint8_t signalstrength;
	
	//  To get signal strength
	sim900_cmd("AT+CSQ\r\n\0",simresponse); 
	
	//  Re-enabling reception for OK
	sbi(UCSR1B,RXEN1);
	
	sim900_response(dummy);
	
	strength[0]=simresponse[6];
	if (simresponse[7]!=','){
		strength[1]=simresponse[7];
		strength[2]='\0';
	} else {
		strength[1]='\0';
	}	
	
	signalstrength=atoi(strength);
	
	if (signalstrength<10 || signalstrength==99){
		return 0;
	} else if (signalstrength>=10 && signalstrength<20){
		return 1;
	} else if (signalstrength>=20 && signalstrength<30){
		return 2;
	} else if (signalstrength>30){
		return 3;
	}
}

ISR(TIMER1_OVF_vect)
{
	overflowcount++;
	
	if (overflowcount>=10){  //  To account for 20 secs
		overflowcount = 0;
		skip = 1;
		cbi(TCCR1B,CS12);
	}
}

void start_timer(){
	skip = 0;
	TCNT1 = 0;
	sbi(TCCR1B,CS12);
}

void reset_SIM900(){
	
	char simresponse[20];
	
	sim900_cmd("AT+CPOWD=1\r\n\0",simresponse);	
	_delay_ms(20000);
}

uint8_t wait_for_data(){
	// Start Timer
	start_timer();
	
	//  Waiting for data
	while (!(UCSR1A & (1<<RXC1))){
		_delay_us(10);
		if (skip==1){
			return 0;  //  No data; timer expired
		}
	}
	
	//  Stop Timer
	cbi(TCCR1B,CS12);
	
	return 1;  //  Data arrived
}

void get_loc(char* apn,char* loc_lat,char* loc_long){
	  char set_apn_str[60], sim_Response[70];
	  
	  uint8_t number_attempts=0,allokay=1;
	  
	  //  No. of attempts
	  number_attempts=NumberOfAttempts-2;		// Number of attempts reduced to 4
// 	  
// 	  if ((no_loc_flag%40) != 0)
// 	  {
// 		  loc_lat=0;loc_long=0;
// 		  return;
// 	  }
// 	  else
// 	  {
// 		  
// 	  no_loc_flag = 0;
	  
	  while (number_attempts>0){
		  number_attempts--;
		  
		  if (!allokay){
			  allokay=1;
			  //  Deactivate Bearer
			  sim900_cmd("AT+SAPBR=0,1\r\n",sim_Response);
		  }
		  
		  //  Setting connection type as GPRS
		  sim900_cmd("AT+SAPBR=3,1,\"CONTYPE\",\"GPRS\"\r\n",sim_Response);
		  if (strstr(sim_Response,"OK")==0){
			  continue;
		  }
		  
		  // Empty APN string
		  memset(set_apn_str, 0, sizeof set_apn_str);
		  strcat(set_apn_str,"AT+SAPBR=3,1,\"APN\",\"");
		  strcat(set_apn_str,apn);
		  strcat(set_apn_str,"\"\r\n");
		  sim900_cmd(set_apn_str,sim_Response);
		  if (strstr(sim_Response,"OK")==0){
			  continue;
		  }
		  
		  //  Open Bearer
		  sim900_cmd("AT+SAPBR=1,1\r\n",sim_Response);
		  if (strstr(sim_Response,"OK")==0){
			  allokay=0;
			  continue;
		  }
		  
		  //  Query Bearer
		  sim900_cmd("AT+SAPBR=2,1\r\n",sim_Response);
		  if(strstr(sim_Response,"ERROR")!=0){
			  continue;
		  }
		  
		  if (strstr(sim_Response,"OK")==0){
			  allokay=0;
			  continue;
		  }
		  
		  //  Get Latitude and Longitude
		  sim900_cmd("AT+CIPGSMLOC=1,1\r\n",sim_Response);
		  
		  if (strstr(sim_Response,"OK")!=0)
		  {
			  if (strstr(sim_Response,"+CIPGSMLOC: 0")!=0)
			  {
				  char *s, *m, *e;
				  
				  memset(loc_lat, 0, sizeof loc_lat);
				  memset(loc_long, 0, sizeof loc_long);
				  
				  s = strstr(sim_Response,"+CIPGSMLOC:");
				  if (s != 0)
				  {
					  m = strchr(s+14,',');
					  if ( m != 0)
					  {
						  e = strchr(m+1,',');
						  if (e-s-14 < 20)
						  {
							  memcpy(loc_long, s+14, m-s-14);
							  memcpy(loc_lat,  m+1, e-m-1);
						  }
					  }else
					  {
						  allokay=0;
						  continue;
					  }
					  }else{
					  allokay=0;
					  continue;
				  }
			  }
			  else{
				  //  Deactivate Bearer
				  sim900_cmd("AT+SAPBR=0,1\r\n",sim_Response);
				  break;
			  }
		  }
		  else{
			  allokay=0;
			  continue;
		  }
		  
		  //  Deactivate Bearer
		  sim900_cmd("AT+SAPBR=0,1\r\n",sim_Response);
		  if (strstr(sim_Response,"OK")!=0){
			  break;
		  }
	  }
		  }
	  
	  
