//#######################################################################################################
//#################################### Plugin 114: DMS501A #############################################
//#################################### by serpa #############################################
//#######################################################################################################

#define PLUGIN_114
#define PLUGIN_ID_114 114
#define PLUGIN_NAME_114 "Dust sensor - DSM501a"
#define PLUGIN_VALUENAME1_114 "PM1.0" // from the datasheet the detection is from PM1 and up. You could have from PM1 to PM2.5, on subtracting PM2.5 value on PM1 value. This value come from the pin #4
#define PLUGIN_VALUENAME2_114 "PM2.5" // from the datasheet the detection is from PM2.5 and up. This value come from the pin #2. With different resistor topn the pin #1, you could adjust the size threshold detection 

#define DELTA_MIN 10000  //10ms
#define DELTA_MAX 90000  //90ms

unsigned long Plugin_114_pulseCounter[TASKS_MAX];
unsigned long Plugin_114_pulseTotalCounter[TASKS_MAX];
unsigned long Plugin_114_pulseTime[TASKS_MAX];
unsigned long Plugin_114_pulseTimePrevious[TASKS_MAX];
unsigned long tstart1, tstart2;
unsigned long tduration = 30000; // duration of measurement in ms
//unsigned long triggerOn; // start of pulse time in us
//unsigned long triggerOff; // end of pulse time in us
//unsigned long lowpulseoccupancy; // duration of pulse in us
volatile unsigned long thigh1, thigh2;
volatile unsigned long tlow1, tlow2;
volatile unsigned long startlow1, startlow2;
volatile unsigned long starthigh1, starthigh2;
volatile boolean done1, done2;
volatile boolean value1, value2;
//boolean trigger = false;
volatile float ratio1, ratio2;
volatile int DMSpin1, DMSpin2;
float dens1, dens2;

typedef struct{
	volatile unsigned long start;
	volatile unsigned int cnt;
	volatile unsigned long low;
  volatile unsigned long high;
} pm_114;

pm_114 data_114[2];

boolean Plugin_114(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_114;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_TEMP_HUM;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 2;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_114);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_114));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_114));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        char tmpString[128];
        sprintf_P(tmpString, PSTR("<TR><TD>Averaging Time (mSec):<TD><input type='text' name='plugin_114' value='%u'>"), Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
        string += tmpString;
        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        String plugin1 = WebServer.arg("plugin_114");
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = plugin1.toInt();
        // tduration= Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        success = true;
        break;
      }


    case PLUGIN_INIT:
      {
        String log = F("INIT : DSM501A ");
        log += Settings.TaskDevicePin1[event->TaskIndex];
        addLog(LOG_LEVEL_INFO, log);
        // tduration= Settings.TaskDevicePluginConfig[event->TaskIndex][0];
//        tstart1 = millis();
//        startlow1 = micros();
//        starthigh1 = startlow1;
        DMSpin1 = Settings.TaskDevicePin1[event->TaskIndex];
        pinMode(DMSpin1, INPUT);
				data_114[0].start=micros();
				data_114[0].cnt=0;
        attachInterrupt(digitalPinToInterrupt(DMSpin1), Plugin_114_ISR1, CHANGE);
//        tstart2 = millis();
//        startlow2 = micros();
//        starthigh2 = startlow2;
        DMSpin2 = Settings.TaskDevicePin2[event->TaskIndex];
        pinMode(DMSpin2, INPUT);
				data_114[1].start=micros();
				data_114[0].cnt=0;
        attachInterrupt(digitalPinToInterrupt(DMSpin2), Plugin_114_ISR2, CHANGE);
        success = true;
        break;
      }

    case PLUGIN_READ:
		{
      String log = F("DSM501A: PM1.0=");
			if(data_144[0].cnt>0){
			}
      UserVar[event->BaseVarIndex] = (float) dens1;
      log += F(" PM2.5=");
      log += dens2;
      addLog(LOG_LEVEL_INFO, log);
      UserVar[event->BaseVarIndex + 1] = (float) dens2;

/*        if (done1 && done2) {
          done1 = FALSE;
          dens1 = ratio1 * 110; // ug/m^3
          done2 = FALSE;
          dens2 = ratio2 * 110; // ug/m^3

          String log = F("DSM501A: PM1.0=");
          log += dens1;
          log += F(" PM2.5=");
          log += dens2;
          addLog(LOG_LEVEL_INFO, log);
          UserVar[event->BaseVarIndex] = (float) dens1;
          UserVar[event->BaseVarIndex + 1] = (float) dens2;
        }
*/        success = true;
        break;
      }
  }
  return success;
}


/*********************************************************************************************\
  Check Pulse (called from irq handler)
  \*********************************************************************************************/
void Plugin_114_ISR1()
{
   if(digitalRead(DMSpin1) ==0){//read input pin just changed
			data_114[0].start=micros();
	 }else{
			data_114[0].start=micros();
			int delta=micros()-data_114[0].start;
			if(delta>DELTA_MIN && delta<DELTA_MAX){
				data_114[0].cnt++;
				data_114[0].low+=delta;
			}
	 }
/*  if (value1 == 0) { // gone low
    startlow1 = micros(); // record starting of low period
    thigh1 += startlow1 - starthigh1; // record duration of past high state
  } else { // gone high
    starthigh1 = micros(); // record starting of high period
    tlow1 += starthigh1 - startlow1; // record duration of past low state
  }
  if (millis() > tstart1 + tduration) { // check if average time has past
    tstart1 = millis(); // reset time period
    ratio1 = float(tlow1) / float(thigh1 + tlow1) * 100; // compute ratio low to total
    tlow1 = 0; // reset low time counter
    thigh1 = 0; // reset high time counter
    done1 = TRUE; // set reading complete flag
  }
 */
}
/*********************************************************************************************\
  Check Pulse (called from irq handler)
  \*********************************************************************************************/
void Plugin_114_ISR2()
{
  value2 = digitalRead(DMSpin2); //read input pin just changed
  if (value2 == 0) { // gone low
    startlow2 = micros(); // record starting of low period
    thigh2 += startlow2 - starthigh2; // record duration of past high state
  } else { // gone high
    starthigh2 = micros(); // record starting of high period
    tlow2 += starthigh2 - startlow2; // record duration of past low state
  }
  if (millis() > tstart2 + tduration) { // check if average time has past
    tstart2 = millis(); // reset time period
    ratio2 = float(tlow2) / float(thigh2 + tlow2) * 100; // compute ratio low to high
    tlow2 = 0; // reset low time counter
    thigh2 = 0; // reset high time counter
    done2 = TRUE; // set reading complete flag
  }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////
 /*#include "DSM501.h"
#include <avr/interrupt.h>


#define _mS_By_S(x)	((x) * 1000ul)
#define _S_By_S(x) 	(x)

#define DSM501_MIN_SIG_SPAN	(10ul)			// 10mS
#define DSM501_MAX_SIG_SPAN	(90ul)			// 90mS
#define DSM501_MIN_WIN_SPAN	_mS_By_S(60ul)	// 60S

#define PM10_PIN	A3
#define PM25_PIN	A2

#define PM10_IDX	0
#define PM25_IDX	1

#define SAF_WIN_MAX 10	// mins

uint8_t dsm501_coeff = 1;

DSM501::DSM501(int pin10, int pin25) {
	_pin[PM10_IDX] = pin10;
	_pin[PM25_IDX] = pin25;

	for (int i = 0; i < 2; i++) {
		_win_start[i] = 0;
		_low_total[i] = 0;
		_state[i] = S_Idle;
		_sig_start[i] = 0;
		_lastLowRatio[i] = NAN;

		_saf_sum[i] = 0;
		memset(_saf_ent[i], 0, SAF_WIN_MAX * sizeof(uint32_t));
		_saf_idx[i] = 0;
	}
}


void DSM501::begin() {
	_coeff = dsm501_coeff;

	_win_start[PM10_IDX] = millis();
	pinMode(_pin[PM10_IDX], INPUT);

	_win_start[PM25_IDX] = millis();
	pinMode(_pin[PM25_IDX], INPUT);
}


void DSM501::reset() {
	_win_start[PM10_IDX] = millis();
	_win_start[PM25_IDX] = millis();
}

uint8_t DSM501::setCoeff(uint8_t coeff) {
	_coeff = coeff;
	return _coeff;
}


void DSM501::update() {
	if (_state[PM10_IDX] == S_Idle && digitalRead(_pin[PM10_IDX]) == LOW) {
		signal_begin(PM10_IDX);
	} else if (_state[PM10_IDX] == S_Start && digitalRead(_pin[PM10_IDX]) == HIGH) {
		signal_end(PM10_IDX);
	}

	if (_state[PM25_IDX] == S_Idle && digitalRead(_pin[PM25_IDX]) == LOW) {
		signal_begin(PM25_IDX);
	} else if (_state[PM25_IDX] == S_Start && digitalRead(_pin[PM25_IDX]) == HIGH) {
		signal_end(PM25_IDX);
	}
}


void DSM501::signal_begin(int i) {
	if (millis() >= _win_start[i]) {
		_sig_start[i] = millis();
		_state[i] = S_Start;
	}
}


void DSM501::signal_end(int i) {
	uint32_t now = millis();
	if (_sig_start[i]) { // we had a signal, and
		if ((now - _sig_start[i]) <= DSM501_MAX_SIG_SPAN &&
			(now - _sig_start[i]) >= DSM501_MIN_SIG_SPAN) {	// this signal is not bouncing.
			_low_total[i] += now - _sig_start[i];
		}
		_sig_start[i] = 0;
	}
	_state[i] = S_Idle;
}


 * Only return the stabilized ratio

double DSM501::getLowRatio(int i) {
	uint32_t now = millis();
	uint32_t span = now - _win_start[i];

	// special case if the device run too long, the millis() counter wrap back.
	if (now < _win_start[i]) {
		span = DSM501_MIN_WIN_SPAN;
	}

	if (span >= DSM501_MIN_WIN_SPAN) {
		int idx = (++_saf_idx[i]) % SAF_WIN_MAX;
		_saf_sum[i] -= _saf_ent[i][idx];
		_saf_ent[i][idx] = _low_total[i];
		_saf_sum[i] += _low_total[i];

		if (_saf_idx[i] < SAF_WIN_MAX) {
			_lastLowRatio[i] = (double)_saf_sum[i] * 100.0 / (double)(DSM501_MIN_WIN_SPAN * _saf_idx[i]);
		} else {
			_lastLowRatio[i] = (double)_saf_sum[i] * 100.0 / (double)(DSM501_MIN_WIN_SPAN * SAF_WIN_MAX);
		}

		_win_start[i] = now;
		_low_total[i] = 0;
	}

	return _lastLowRatio[i] / (double)_coeff;
}


double DSM501::getParticalWeight(int i) {
	 * with data sheet...regression function is
	 * 	y=0.30473*x^3-2.63943*x^2+102.60291*x-3.49616
	double r = getLowRatio(i);
	double weight = 0.30473 * pow(r, 3) - 2.63943 * pow(r, 2) + 102.60291 * r - 3.49616;
	return weight < 0.0 ? 0.0 : weight;
}


int DSM501::getAQI() {
	// this works only under both pin configure
	int aqi = -1;

	double P25Weight = getParticalWeight(0) - getParticalWeight(1);
	if (P25Weight>= 0 && P25Weight <= 15.4) {
		aqi = 0   +(int)(50.0 / 15.5 * P25Weight);
	} else if (P25Weight > 15.5 && P25Weight <= 40.5) {
		aqi = 50  + (int)(50.0 / 25.0 * (P25Weight - 15.5));
	} else if (P25Weight > 40.5 && P25Weight <= 65.5) {
		aqi = 100 + (int)(50.0 / 25.0 * (P25Weight - 40.5));
	} else if (P25Weight > 65.5 && P25Weight <= 150.5) {
		aqi = 150 + (int)(50.0 / 85.0 * (P25Weight - 65.5));
	} else if (P25Weight > 150.5 && P25Weight <= 250.5) {
		aqi = 200 + (int)(100.0 / 100.0 * (P25Weight - 150.5));
	} else if (P25Weight > 250.5 && P25Weight <= 350.5) {
		aqi = 300 + (int)(100.0 / 100.0 * (P25Weight - 250.5));
	} else if (P25Weight > 350.5 && P25Weight <= 500.0) {
		aqi = 400 + (int)(100.0 / 150.0 * (P25Weight - 350.5));
	} else if (P25Weight > 500.0) {
		aqi = 500 + (int)(500.0 / 500.0 * (P25Weight - 500.0)); // Extension
	} else {
		aqi = -1; // Initializing
	}

	return aqi;
}

*/