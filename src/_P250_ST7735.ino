#ifdef USES_P250
#include <Adafruit_ST7735.h>
#include <Adafruit_GFX.h>
#include <SPI.h>
#include <Fonts/FreeMono9pt7b.h>
//from fonts
#define yAdvance  15    //
#define xAdvance  11    //max from glyph
#define baseline  10

//#define LED_CONTROL //led control on
// Sample templates
//  Temp: [DHT11#Temperature]   Hum:[DHT11#humidity]
//  DS Temp:[Dallas1#Temperature#R]
//  Lux:[Lux#Lux#R]
//  Baro:[Baro#Pressure#R]

#define PLUGIN_250
#define PLUGIN_ID_250    250               //plugin id
#define PLUGIN_NAME_250   "Display - 128x160 ST7735"
#define PLUGIN_VALUENAME1_250 "ST7735"
#define PLUGIN_250_DEBUG  false             //set to true for extra log info in the debug

// WEMOS
// SCK     GPIO14    D5
// MOSI    GPIO13    D7
// MISO    GPIO12    D6
// CS      GPIO15    D8

#define TFT_CS     -1 //gnd
#define TFT_RST    -1 //rst
#define TFT_DC     15 //15 D8
#define TFT_LED    16 //D0
Adafruit_ST7735 *Plugin_250_tft=NULL;  //128x160

void  Plugin_250_del(){
	if(Plugin_250_tft!=NULL){
    delete Plugin_250_tft;
    Plugin_250_tft=NULL;
	}
}

boolean Plugin_250(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;
#ifdef LED_CONTROL
  static byte displayTimer=0;
#endif
	static uint8_t Xnum=0;
	static uint8_t Ynum=0;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_250;
#ifdef LED_CONTROL
        Device[deviceCount].Type = DEVICE_TYPE_TRIPLE;//DC,LED,DISPLAY
#else
        Device[deviceCount].Type = DEVICE_TYPE_SINGLE;//DC,LED,DISPLAY
#endif
        Device[deviceCount].VType = SENSOR_TYPE_NONE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = false;
        Device[deviceCount].ValueCount = 0;
        Device[deviceCount].SendDataOption = false;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = false;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_250);
        break;
      }
/*
    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_250));
        break;
      }
*/
    case PLUGIN_EXIT:
			{
#ifdef LED_CONTROL
      	digitalWrite(Settings.TaskDevicePin2[event->TaskIndex],LOW);//led off
#endif
      	Plugin_250_del();
	  		break;
			}

    case PLUGIN_INIT:
      {
        String log;
      log = F("ST7735 INIT: ");
/*				log +=F("pin1: ");
        log+= String(Settings.TaskDevicePin1[event->TaskIndex]);
        log +=F(" pin2: ");
        log+= String(Settings.TaskDevicePin2[event->TaskIndex]);
        log +=F(" pin3: ");
        log+= String(Settings.TaskDevicePin3[event->TaskIndex]);*/
        log+=ESP.getResetInfo();
        addLog(LOG_LEVEL_INFO, log);
        
        Plugin_250_del();
#ifdef LED_CONTROL
				if(Settings.TaskDevicePin1[event->TaskIndex]!=-1 && Settings.TaskDevicePin2[event->TaskIndex]!=-1){
	        pinMode(/*TFT_LED*/Settings.TaskDevicePin2[event->TaskIndex], OUTPUT);
          digitalWrite(Settings.TaskDevicePin2[event->TaskIndex],HIGH);//led on
	        if (Settings.TaskDevicePin3[event->TaskIndex] != -1 && Settings.TaskDevicePluginConfig[event->TaskIndex][1]>0){//button set
	          pinMode(/*DISPLAY*/Settings.TaskDevicePin3[event->TaskIndex], INPUT_PULLUP);
	          displayTimer = 2*Settings.TaskDevicePluginConfig[event->TaskIndex][1];
		        log=F("P250 : Display timer");
	        	addLog(LOG_LEVEL_INFO, log);
					}
#else
				if(Settings.TaskDevicePin1[event->TaskIndex]!=-1){
#endif
	        pinMode(/*TFT_DC*/Settings.TaskDevicePin1[event->TaskIndex], OUTPUT);

					 // initialize SPI:
	        SPI.setHwCs(false);
	        SPI.end();
	        SPI.begin();
	        addLog(LOG_LEVEL_INFO, F("P250 : SPI Init"));
          Plugin_250_tft = new Adafruit_ST7735(TFT_CS, /*TFT_DC*/Settings.TaskDevicePin1[event->TaskIndex], TFT_RST);
          Plugin_250_tft->initR(INITR_BLACKTAB);
				  Plugin_250_tft->fillScreen(ST7735_BLACK);
          Plugin_250_tft->setRotation(Settings.TaskDevicePluginConfig[event->TaskIndex][0]);
          SPI.setFrequency(40000000);
          const GFXfont *f=&FreeMono9pt7b;
          Plugin_250_tft->setFont(f);
//        yAdvance=(uint8_t)pgm_read_byte(&f->yAdvance);
//          xAmax=(uint8_t)pgm_read_byte(&g->xAdvance)
          Plugin_250_tft->setTextWrap(false);
          Ynum=(Plugin_250_tft->height()/yAdvance);//row
          Xnum=(Plugin_250_tft->width()/xAdvance);//col
          //Plugin_250_tft->setCursor(10, 10);
  				//Plugin_250_tft->print(F("TFT INIT"));
	        log=F("P250 : TFT Init ");
					log+=String(Xnum);
					log+=F("x");
					log+=String(Ynum);
        	addLog(LOG_LEVEL_INFO, log);
				}else{
	        addLog(LOG_LEVEL_INFO, F("P250 : pins not configured"));
				}
        success = true;

        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        byte choice2 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
#ifdef LED_CONTROL
        addFormNumericBox(F("Display Timeout"), F("plugin_250_timer"), Settings.TaskDevicePluginConfig[event->TaskIndex][1]);
        addFormNote(F("<b>GPIO</b> = DC=pin1, LED=pin2, DISPLAY=pin3"));
#else
        addFormNote(F("<b>GPIO</b> = DC=pin1"));
#endif
        addFormNote(F("<b>GPIO</b> = MISO=12, MOSI=13, SCK=14, CS=gnd, RST=rst)"));

        String options2[4] = { F("0"), F("90"), F("180"), F("270") };
        int optionValues2[4] = { 0, 1, 2, 3 };
        addFormSelector(F("Rotation"), F("plugin_250_rotate"), 4, options2, optionValues2, choice2);

        char deviceTemplate[10][64];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
        for (byte varNr = 0; varNr < 10; varNr++)
        {
          addFormTextBox(String(F("Line ")) + (varNr + 1), String(F("Plugin_250_template")) + (varNr + 1), deviceTemplate[varNr], 64);
        }

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_250_rotate"));
#ifdef LED_CONTROL
        Settings.TaskDevicePluginConfig[event->TaskIndex][1] = getFormItemInt(F("plugin_250_timer"));
#endif

        char deviceTemplate[10][64];
        for (byte varNr = 0; varNr < 10; varNr++)
        {
          String arg = F("Plugin_250_template");
          arg += varNr + 1;
          String tmpString = WebServer.arg(arg);
          strncpy(deviceTemplate[varNr], tmpString.c_str(), sizeof(deviceTemplate[varNr])-1);
          deviceTemplate[varNr][63]=0;
        }

        SaveCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
        success = true;
        break;
      }

#ifdef LED_CONTROL
    case PLUGIN_TEN_PER_SECOND:
      {
        if (Settings.TaskDevicePin3[event->TaskIndex] != -1)//button set
        {
          if (digitalRead(Settings.TaskDevicePin3[event->TaskIndex])==LOW)//button press low
          {
            digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], HIGH);      //led on
            displayTimer = Settings.TaskDevicePluginConfig[event->TaskIndex][1];
//      		addLog(LOG_LEVEL_INFO, F("P250: led on"));
          }
        }
        break;
      }

    case PLUGIN_ONCE_A_SECOND:
      {
/*        String log = F("P250: timer=");
        log +=(String(displayTimer));
				log +=F(" pin3=");
				log +=(String(digitalRead(Settings.TaskDevicePin3[event->TaskIndex])));
				log +=F(" led=");
				log +=String(digitalRead(Settings.TaskDevicePin2[event->TaskIndex]));
        addLog(LOG_LEVEL_INFO, log);*/
        if (displayTimer > 0)
        {
          displayTimer--;
          if (displayTimer == 0)
            digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], LOW);//led off
        }
        break;
      }
#endif

    case PLUGIN_READ:
      {
        char deviceTemplate[10][64];
        LoadCustomTaskSettings(event->TaskIndex, (byte*)&deviceTemplate, sizeof(deviceTemplate));
        for (byte x = 0; x < 10; x++)
        {
          String tmpString = deviceTemplate[x];
          if (tmpString.length())
          {
            String newString = P250_parseTemplate(tmpString, Xnum);
          	Plugin_250_print(newString.c_str(), 0, x*yAdvance);//col,row
          }
        }
        success = true;
        break;
      }

    case PLUGIN_WRITE:
      {      
        String arguments = String(string);

        String log = F("ST7735 WRITE: ");
        log += F(", TaskDeviceName: ");
        log += getTaskDeviceName(event->TaskIndex);
        log += F(", event->TaskIndex: ");
        log += String(event->TaskIndex);
        log += F(", cmd str: ");
        log += string;
        addLog(LOG_LEVEL_INFO, log);

        int argIndex = arguments.indexOf(',');
        if (argIndex)
          arguments = arguments.substring(0, argIndex);

        if (arguments.equalsIgnoreCase(F("TFTCMD")))
        {
          success = true;
          argIndex = string.lastIndexOf(',');
          arguments = string.substring(argIndex + 1);
          if (arguments.equalsIgnoreCase(F("Off")))
            digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], LOW);      //led
          else if (arguments.equalsIgnoreCase(F("On")))
            digitalWrite(Settings.TaskDevicePin2[event->TaskIndex], HIGH);      //led
          else if (arguments.equalsIgnoreCase(F("Clear")))
            Plugin_250_tft->fillScreen(ST77XX_BLACK);
        }
        else if (arguments.equalsIgnoreCase(F("TFT")))
        {
          success = true;
          argIndex = string.lastIndexOf(',');
          arguments = string.substring(argIndex + 1);
          String newString = P250_parseTemplate(arguments, Xnum);
          Plugin_250_print(newString.c_str(), (event->Par2 - 1), (event->Par1 - 1));
        }
        break;
      }
  }
  return success;
}

String P250_parseTemplate(String &tmpString, byte lineSize) {
  String result = parseTemplate(tmpString, lineSize);
  const char degree[3] = {0xc2, 0xb0, 0};  // Unicode degree symbol
  const char degree_oled[2] = {0x7F, 0};  // P250_TFT degree symbol
  result.replace(degree, degree_oled);
  return result;
}

void Plugin_250_print(const char *string, int X, int Y){//col row
  Plugin_250_tft->fillRect(X, Y, Plugin_250_tft->width()-X, yAdvance, ST7735_BLACK);//x,y,w,h
	Plugin_250_tft->setCursor(X, Y+baseline);//baseline
	Plugin_250_tft->print(string);
}

#endif // USES_P250

