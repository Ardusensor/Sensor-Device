/*
 * ArduSensor.c
 *
 * Created: 22.02.2014 17:54:27
 *  Author: Cmc
 */ 

#include <stdio.h>

#define F_CPU 16000000UL

#define _DH_ "0013A200" 		// XBee address of the destination Supernode
#define _DL_ "40BD5555"

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <avr/wdt.h>
#include <stdlib.h>
#include <avr/interrupt.h>
#include <avr/eeprom.h>
#include <util/delay.h>
#include <avr/sleep.h>
#include "Common.h"
#include "uart.h"


uint16_t sleepCounter = 60000;
uint16_t timeforsleep = 6400;	//64000 around 24 minutes sleep time
uint16_t TempValueSens= 0;		//the value of the meaasured temperature from ferrite
uint16_t TempValueI = 0;		//the value of the measured temperature from cpu
uint16_t VoltageValue = 0;		//the voltage of the supply
uint16_t CapMeas = 0;
uint16_t PostID = 14;
char buffer[64];
uint8_t SC = 0;

uint16_t CapMeasA=0;
uint16_t CapMeasB=0;
uint16_t CapMeasC=0;
uint16_t CapMeasD=0;
uint16_t CapMeasE=0;

uint16_t Cap_Measure(void);


int main(void)
{
	SETBIT(TCCR0B,CS00);			//Set timer prescale to clk
	SETBIT(DDRC,PC5);				//Set Xbee sleep ctrl pin as output
	CLEARBIT(ACSR,ACD);				//Make sure that comparator works
	DIDR1=0xFF;						//Analog comparator pins are not digital inputs
	cli();							//no interrupts whlie dealing with wdt
	MCUSR = 0;						//null all previous reset info
	wdt_disable();					//kill WDT
	SETBIT(DDRD,PD2);				//Ready pullup mosfet
	SETBIT(PORTD,PD2);				//pullup mosfet off
	sei();							//good place as any to enable interrupts
	
	uart_init(UART_BAUD_SELECT(9600,F_CPU));	//Enable xbee uart at 9600
	
	// Initialization sequence, sets the ID of Supernode that will connect to
	uart_puts("+++"); 			//Enter command mode
	_delay_ms(2000);

	uart_puts("ATID 1342\r"); 	// Set PAN ID
	_delay_ms(500);
	
 	uart_puts("ATDH"); 			// Set address
	_delay_ms(10);
	uart_puts(_DH_); 			// Written in #Define
	uart_puts("\r");
	_delay_ms(100);
	
	uart_puts("ATDL");
	_delay_ms(10);
	uart_puts(_DL_);			// Written in #Define
	uart_puts("\r");
	_delay_ms(100);
	
	
	uart_puts("ATNH 1E\r");		// Required XBee constants
	_delay_ms(100);
	uart_puts("ATNO 3\r");
	_delay_ms(100);
	uart_puts("ATSP 7D0\r");
	_delay_ms(100);
	uart_puts("ATSN 21C\r");
	_delay_ms(100);
	uart_puts("ATSM 1\r");
	_delay_ms(100);
	
	uart_puts("ATWR\r");		// Save configuration
	_delay_ms(2000);
	
	uart_puts("ATCN\r");		// Exit Command mode
	_delay_ms(1000);


    while(1)
    {
		if(sleepCounter >= timeforsleep )
		{
			wdt_disable();				//no need for wdt here
			
			CLEARBIT(PORTC,PC5);		//Wake up xbee (also needs some time) 
			CLEARBIT(PORTD,PD2);		//Start peripherals under mosfet
		
			_delay_ms(4);				//Stabilize time
	
			//Lets measure ADC values
			//First measurement. Disregard Int reference not stabilized
			ADMUX = 0b11000000;
			ADCSRA = 0b11000111;
			_delay_ms(3);
			TempValueSens = ADC;
		
			ADMUX = 0b11000000;
			ADCSRA = 0b11000111;
			_delay_ms(3);
			TempValueSens = ADC;
		
			ADMUX = 0b11001000;
			ADCSRA = 0b11000111;
			_delay_ms(3);
			TempValueI = ADC;
		
			ADMUX = 0b11000001;
			ADCSRA = 0b11000111;
			_delay_ms(3);
			VoltageValue = ADC;
			CLEARBIT(ADCSRA,ADEN);
		
			//Take some cap measures
			CapMeasA = Cap_Measure();
			CapMeasB = Cap_Measure();
			CapMeasC = Cap_Measure();
			CapMeasD = Cap_Measure();
			CapMeasE = Cap_Measure();
			
			//Calculate average
			CapMeas =  (CapMeasA + CapMeasB + CapMeasC + CapMeasD + CapMeasE)/5;
		
			SETBIT(PORTD,PD2);			//Un initalize peripheral mosfet
		
			//Some formating
			uint16_t tvse = TempValueSens;
			uint16_t tvi = TempValueI;
			uint16_t vv = VoltageValue;
			uint16_t cm = CapMeas;

			//more formating and uart transmit
			sprintf(buffer,"<%d;%d;%d;%d;%d>", tvi, tvse, vv, cm, SC );	//temperatuuriv‰‰rtus prose, temp v sensor, Pingev‰‰rtus, V‰rske mahtuvuse mııtmine, sendcounter.
			_delay_us(2);
			//Enter delay as needed for xbee wake up completion or check if xbee is awaken
			_delay_ms(4000);
			uart_puts(buffer);			//Sending Dataaaa
			++SC;						//We are counting times we have sent with xbee

			_delay_ms(1000);			//wait for xbee transmit
			sleepCounter = 0;			//since we had our beauty sleep
			SETBIT(PORTC,PC5);			//Xbee time to sleep


		}
		
	

		
		//Enable wdt and go to sleep
		++sleepCounter;
		cli();
		SETBIT(WDTCSR,WDCE);		//writing to watchdog.
		WDTCSR= 0b01110001;			//Watchdog conf for 8s timeout, interrupt mode
		WDTCSR= 0b01110001;	
		
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		sleep_bod_disable();
		sei();
		sleep_cpu();
		
				 
    }
}


uint16_t Cap_Measure(void)
{
	TCNT1=0;						//Reset counter
	CLEARBIT(DDRD,PD6);				//stop charge draining
	

	SETBIT(DDRD,PD5);				//Make Charge pin output
	TCCR1B=0b00000001;				//Start counter
	SETBIT(PORTD,PD5);				//Start loading the cap
	
	CLEARBIT(TIFR1,TOV1);			//Reset overflow flag
	
	//wait until comparator registers change or counter overflows
	while((BITVAL(ACSR,ACO)!=1)|| (BITVAL(TIFR1,TOV1)))	
	{
		//then we wait
	}
	
	TCCR1B = 0x00;					//stop counter
	CLEARBIT(PORTD,PD5);			//stop charging
	SETBIT(DDRD,PD6);				//Start charge draining
	CLEARBIT(PORTD,PD6);			//drain charge in cap	
	_delay_us(2);					//some time to get charge out before next measurement

	if(BITVAL(TIFR1,TOV1) == 0)
	{
		return(TCNT1);				//return time to charge
	}
	
	else
	{
		return(0xFFFF);				//Way too big number for working cap measure
	}
	
}

ISR(WDT_vect)
{
	//_delay_us(1);	//this does not do else than wakes up CPU
}





