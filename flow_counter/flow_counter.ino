#include <Arduino.h>
#include <EEPROM.h>
#include <avr/sleep.h>
#include <avr/interrupt.h>
#include <avr/power.h>
#include <avr/wdt.h>

#define RTC_PIN   0
#define SLEEP_PIN 1
#define FLOW_PIN  2
#define UART_TX   3
#define RDY_PIN   4

#define	UART_TX_ENABLED		(1) // Enable UART TX
#define UART_BAUDRATE   57600
#include "uart.h"


struct eeprom_data{//impulse
	uint32_t all;//от начала(всего)
	uint32_t sum;//после обнуления(ресурс фильтра)
} ;
uint16_t volatile cnt;//тек.значение
/*
Model: YF-S201
Flow rate pulse characteristics: Frequency (Hz) = 7.5 * Flow rate (L/min)
Pulses per Liter: 450
*/
#define Q_WATER	450 	//Pulse/liter
#define Q (float)(Q_WATER/1000) //Pulse/ml

int flow2ml(int cnt){//counter to mL
	return (int)((float)cnt/2.7); //* Q;
}

void flow_cnt(){
	cnt++;
}

void tx_data(){
	eeprom_data data;
	EEPROM.get(0, data);
	char *c=(char*)&data;
	for(uint8_t i=0;i<sizeof(data);i++){
  	uart_putc(c[i]);
	}
	EEPROM.put(0,data);
}

void ack(){
	tx_data();
}

void sleep() {
  ADCSRA = 0;                    //disable the ADC
  power_all_disable ();
	wdt_reset();
  sleep_bod_disable();
	sleep_mode();
  sleep_disable();               //wake up here
//  power_timer0_enable();
}

void setup(){
	cnt=0;
	pinMode(SLEEP_PIN, INPUT_PULLUP);
	pinMode(FLOW_PIN, INPUT_PULLUP);
	pinMode(RDY_PIN,INPUT_PULLUP);
	pinMode(UART_TX,OUTPUT);
	pinMode(RTC_PIN,OUTPUT);

  attachInterrupt(FLOW_PIN, flow_cnt, FALLING );//flow
  wdt_reset();
	wdt_enable(WDTO_8S);
	WDTCSR |= _BV(WDIE);
	MCUSR=0;
	sleep();
}

void loop(){
}

ISR (WDT_vect) {
	WDTCSR |= _BV(WDIE);
}
