#ifdef USES_P252
/*
Pb - P boiler steam 0-2.5 Bar
Pw - P water pump   0-19  Bar
Te - T in E61       0-120 *C
Tb - T boiler       0-110 *C
t  - timer          0-99  sec
L  - flow           0-200 ml
-----------
давление в бойлере - 1/4"
давление в магистрали - 1/8" тройник
расход воды - трубка d4x7 Q>100

Input: 0-60 psi(Gauge Pressure);
Output: 0.5-4.5V linear voltage output. 0 psi outputs 0.5V, 30 psi outputs 2.5V, 60 psi outputs 4.5V.

SmallFont - 6x8
MediumNumbers - 12x16
BigNumbers - 14x24
*/
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include "max6675.h"


Adafruit_SSD1306 *Plugin_252_oled;
MAX6675 *Plugin_252_thermocouple;

#define thermoDO  12
#define thermoCS  14
#define thermoCLK 16
#define FLOW_PIN  2
#define PUMP_PIN  13
#define SSR_PIN   0
#define PRESS_PIN 3 //rx
#define BTN_PIN   1 //tx
#define BUZ_PIN   15

#define SSR_ON  HIGH
#define SSR_OFF LOW
#define R1      220.0
#define R2      51.0
#define PSI     60.0 //max P
#define VOUT(v) ( (((R1+R2)/R2)/1024)*v )

#define PLUGIN_252
#define PLUGIN_ID_252    252               //plugin id
#define PLUGIN_NAME_252   "Coffee Maker"
#define PLUGIN_252_DEBUG  false             //set to true for extra log info in the debug

#define PLUGIN_VALUENAME1_252 "p1"
#define PLUGIN_VALUENAME2_252 "p2"
#define PLUGIN_VALUENAME3_252 "p3"

struct Plugin_252_defvar{
	int flow;
	int pump;
	int press;
	bool btn_on;
  float bar;
};
struct Plugin_252_defvar *Plugin_252_var;

void  Plugin_252_del(){
	if(Plugin_252_oled!=NULL){
    delete Plugin_252_oled;
    Plugin_252_oled=NULL;
	}
	if(Plugin_252_thermocouple!=NULL){
    delete Plugin_252_thermocouple;
    Plugin_252_thermocouple=NULL;
	}
/*	if(Plugin_252_var!=NULL){
		delete Plugin_252_var;
    Plugin_252_var=NULL;
	}*/
}

void flowISR(){
  Plugin_252_var->flow++;
}

void pumpISR(){
  Plugin_252_var->pump++;
}

void pressISR(){
  Plugin_252_var->press++;
}

int flow2ml(int cnt){//counter to mL
	return (int)((float)cnt/2.7); //* Q;
}

float psi2bar(float psi){
	return (psi/14.5037738);//1 Bar = 14,5037738 Psi
}

float pressure(int adcval,int max){//adc->bar
	float p;   
  p = VOUT(adcval); 
  if (p < 0.1){//short
  	p = -98;//
  }else if(p> 4.8){//no sensor
  	p=-99;
  }else{
  	p-=0.5;
  	p*=(max/4.0);//psi
  	p=psi2bar(p);//bar
  }
  return p;
}

boolean Plugin_252(byte function, struct EventStruct *event, String& string){
	boolean success = false;
  switch (function){
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_252;
        Device[deviceCount].Type = DEVICE_TYPE_DUMMY;
        Device[deviceCount].VType = SENSOR_TYPE_TRIPLE;// Type of value the plugin will return, used only for Domoticz
        Device[deviceCount].ValueCount = 3;// The number of output values of a plugin. The value should match the number of keys PLUGIN_VALUENAME1_xxx
//        Device[deviceCount].Ports = 0;  // Port to use when device has multiple I/O pins  (N.B. not used much)
//        Device[deviceCount].PullUpOption = false; // Allow to set internal pull-up resistors
//        Device[deviceCount].InverseLogicOption = false;// Allow to invert the boolean state
//        Device[deviceCount].FormulaOption = false;// Allow to enter a formula to convert values during read. (not possible with Custom enabled)
        Device[deviceCount].SendDataOption = true; // Allow to send data to a controller.
        Device[deviceCount].TimerOption = true; // Allow to set the "Interval" timer for the plugin.
        Device[deviceCount].TimerOptional = true;      // When taskdevice timer is not set and not optional, use default "Interval" delay (Settings.Delay)
//        Device[deviceCount].GlobalSyncOption = false; // No longer used.
//        Device[deviceCount].Custom = false; //?
        Device[deviceCount].DecimalsOnly = false;// Allow to set the number of decimals (otherwise treated a 0 decimals)
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_252);
        break;
      }
/*
    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_252));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_252));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_252));
        break;
      }
*/  case PLUGIN_INIT:{
      String log;
			log += F("COFFEE_MAKER INIT: ");
      Plugin_252_del();
      Plugin_252_oled=new Adafruit_SSD1306();
      Plugin_252_thermocouple=new MAX6675(thermoCLK, thermoCS, thermoDO);
      Plugin_252_var=new Plugin_252_defvar;
			if(!Plugin_252_oled || !Plugin_252_thermocouple){
				log+=F("error");
        addLog(LOG_LEVEL_ERROR, log);
				break;
			}
      Plugin_252_oled->begin(SSD1306_SWITCHCAPVCC, 0x3D);
      pinMode(FLOW_PIN, INPUT_PULLUP);
      pinMode(PUMP_PIN, INPUT_PULLUP);
      pinMode(BUZ_PIN, OUTPUT);
      digitalWrite(BUZ_PIN, LOW);
			if(Settings.UseSerial){
				log+=F(" button&pressure disabled, serial is on");
			}else{
      	pinMode(BTN_PIN, INPUT_PULLUP);
        pinMode(PRESS_PIN, INPUT_PULLUP);
        attachInterrupt(digitalPinToInterrupt(PRESS_PIN), pressISR, FALLING);
			}

      attachInterrupt(digitalPinToInterrupt(FLOW_PIN), flowISR, FALLING);
      attachInterrupt(digitalPinToInterrupt(PUMP_PIN), pumpISR, FALLING);

      pinMode(SSR_PIN, OUTPUT);
      digitalWrite(SSR_PIN, SSR_ON);
      
#if FEATURE_ADC_VCC      
//      ADC_MODE(ADC_VCC);
#endif

			log+=F(" OK");
        addLog(LOG_LEVEL_INFO, log);
			success=true;
			break;
		}
		case PLUGIN_EXIT:{
      Plugin_252_del();
			break;
		}
    case PLUGIN_WEBFORM_LOAD:
			success=true;
			break;
    case PLUGIN_WEBFORM_SAVE:
			success=true;
			break;
    case PLUGIN_READ:
			success=true;
			break;
    case PLUGIN_WRITE:
			success=true;
			break;
    case PLUGIN_TEN_PER_SECOND:
			break;
    case PLUGIN_ONCE_A_SECOND:
			break;
	}
  return success;
}
/*
//MAX6675_Library max6675.cpp
#if defined(ESP8266)
#include <pgmspace.h>
#define _delay_ms(ms) delayMicroseconds((ms) * 1000)
#endif

//#include <util/delay.h>
#ifdef avr
#include <avr/pgmspace.h>
#include <util/delay.h>
#endif
#include <stdlib.h>
*/

#endif // USES_P252
