/*
 * ArduSensor.c
 *
 * Created: 22.02.2014 17:54:27
 *  Author: Cmc
 */ 


#define F_CPU 16000000UL
#define TWI_CMD_MASTER_READ 0



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




uint16_t sleepCounter = 117;
uint16_t timeforsleep = 120;	//120 peaks olema 30 min ringis
uint16_t TempValueF= 0;		//the value of the meaasured temperature from ferrite
uint16_t TempValueI = 0;		//the value of the measured temperature from cpu
uint16_t VoltageValue = 0;		//the voltage of the supply
uint16_t CapMeas = 0;
uint16_t PostID = 11;
uint16_t SensCalVal = 200; 
uint8_t buffer[64];
uint8_t SC;


uint16_t Cap_Measure(void);
void CommsTimeLoop(void);




int main(void)
{
	
	SETBIT(TCCR0B,CS00);
	SETBIT(TIMSK0,TOIE0);

	
	
	//SETBIT(ADCSRB,ACME); Wtf see jama on ? 
	//BITSET(ADCSRA,ADEN);
	CLEARBIT(ACSR,ACD);
	DIDR1=0xFF;
	cli();
	MCUSR = 0;
	wdt_disable();
	//initalization for pullup mosfet
	SETBIT(DDRD,PD2);
	//Enable interrupts and enter main loop
	MCUSR &= ~(1<<WDRF); //disable watchdog
	
	//Initalize xbee sleep pin (using i2c outp for it

	DDRC=0b00110000;
	
	
	//TWI init

	
	sei();
	uart_init(UART_BAUD_SELECT(9600,F_CPU));
	
    while(1)
    {
		
		if(sleepCounter >= timeforsleep )
		{
			
			//Bring xbee out from sleep
			CLEARBIT(PORTC,PC4);
			CLEARBIT(PORTC,PC5);
			_delay_ms(5000);
					
			//initalize pullup mosfet
			SETBIT(DDRD,PD2);
			CLEARBIT(PORTD,PD2);

			_delay_ms(1);
			//measure ADC values

			ADMUX = 0b11000000;
			ADCSRA = 0b11000111;
			_delay_ms(3);
			TempValueF = ADC;
		
			ADMUX = 0b11001000;
			ADCSRA = 0b11000111;
			_delay_ms(3);
			TempValueI = ADC;
		
			ADMUX = 0b11000001;
			ADCSRA = 0b11000111;
			_delay_ms(3);
			VoltageValue = ADC;
			CLEARBIT(ADCSRA,ADEN);
		
			CapMeas = Cap_Measure();

			//measure cap value
		
			//un-initalize pullup mosfet
			SETBIT(PORTD,PD2);
			//PostID = PostID+1;			
			uint16_t tvf = TempValueF;
			uint16_t tvi = TempValueI;
			uint16_t vv = VoltageValue;
			uint16_t cm = CapMeas;
			uint16_t pid = PostID;
			uint16_t scv = SensCalVal;
			SC = SC + 1;

			sprintf(buffer,"<%d;%d;%d;%d;%d>",pid, tvi, vv, cm, SC );	//Posti ID, temperatuuriv‰‰rtus, Pingev‰‰rtus, V‰rske mahtuvuse mııtmine, sendcounter. 
			uart_puts(buffer);
			_delay_ms(1000);
			
			sleepCounter = 0;
			SETBIT(PORTC,PC4);
			SETBIT(PORTC,PC5);			

			
		}

		
		//Enable wdt and go to sleep
		++sleepCounter;
		cli();
		SETBIT(WDTCSR,WDCE);
		WDTCSR= 0b01111001;
		WDTCSR= 0b01111001;
		
		set_sleep_mode(SLEEP_MODE_PWR_DOWN);
		sleep_enable();
		sleep_bod_disable();
		sei();
		sleep_cpu();
		
				 
    }
}


uint16_t Cap_Measure(void)
{
	TCNT1=0;

	SETBIT(DDRD,PD5);	
	TCCR1B=0b00000001;
	SETBIT(PORTD,PD5);	
	
	while(BITVAL(ACSR,ACO)!=1)
	{
		
	}
	TCCR1B = 0x00;
	CLEARBIT(PORTD,PD5);

	
	return(TCNT1);
	
}

ISR(WDT_vect)
{
	_delay_us(1);
}

ISR(TIMER0_OVF_vect)
{
		
}



void CommsTimeLoop(void)
{
	//comms handle
}