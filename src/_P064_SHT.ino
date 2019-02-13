//#######################################################################################################
//######################## Plugin 064: Temperature and Humidity sensor Sensirion SHT 10/20/30 ###########
//#######################################################################################################

#define PLUGIN_064
#define PLUGIN_ID_064     64
#define PLUGIN_NAME_064       "Environment - SHT10/20/30"
#define PLUGIN_VALUENAME1_064 "Temperature"
#define PLUGIN_VALUENAME2_064 "Humidity"
#define PLUGIN_VALUENAME3_064 "Humidity20C"
#include <Sensirion.h>

uint8_t Plugin_064_SHT_Pin_SDA;
uint8_t Plugin_064_SHT_Pin_SCL;

Sensirion sht1 = Sensirion(12,0,0x40,true);

boolean Plugin_064(byte function, struct EventStruct *event, String& string)
{
  boolean success = false;

  switch (function)
  {
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_064;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_TRIPLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 3;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_064);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_064));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_064));
          strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME3_064));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        const String options[] = { F("SHT 1X"), F("SHT 2X"), F("SHT 3X") };
        int indices[] = { 10, 20, 30 };

        addFormSelector(string, F("SHT Type"), F("plugin_064_shttype"), 3, options, indices, Settings.TaskDevicePluginConfig[event->TaskIndex][0] );

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = getFormItemInt(F("plugin_064_shttype"));
         success = true;
        break;
      }

case PLUGIN_INIT:
      {
        Plugin_064_SHT_Pin_SDA = Settings.TaskDevicePin1[event->TaskIndex];
        Plugin_064_SHT_Pin_SCL = Settings.TaskDevicePin2[event->TaskIndex];

     byte Par3 = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        uint8_t address = 0x00; //SHT1X
        if(Par3 == 20){//SHT2x
          address = 0x40;
        }
        else if(Par3 == 30){//SHT 3x
          address = 0x44;
        }
         String log = F("Sensirion SHT INIT address: ");
         log += String(address, HEX); 
         log += F(" SDA:");
         log += String( Plugin_064_SHT_Pin_SDA);
         log += F(" SCL:");
         log += String(Plugin_064_SHT_Pin_SCL);
        addLog(LOG_LEVEL_DEBUG,log);
        sht1 = Sensirion ( Plugin_064_SHT_Pin_SDA , Plugin_064_SHT_Pin_SCL , address , true );
     success = true;
      break;
      }
      
    case PLUGIN_READ:
      {
        
      float temperature;
      float humidity;
      float dewpoint;
      float humidity20C;
       
       int8_t ret = 0;

       ret = sht1.measure(&temperature,&humidity, &dewpoint,20,&humidity20C);
      
      while (ret>0 && ret <6)
      {
        ret = sht1.measure(&temperature,&humidity, &dewpoint,20,&humidity20C);        
        
      }
      
      
       if (ret == S_Meas_Rdy) // A new measurement is available
        {
        UserVar[event->BaseVarIndex] = temperature;
        UserVar[event->BaseVarIndex+1] = humidity;
        UserVar[event->BaseVarIndex+2] = humidity20C;
        
            String log = F("Sensirion SHT : Reading: "); 
            log += F("Temperature: ");
            log += String(UserVar[event->BaseVarIndex]);
            log += F(" Humidity: ");
            log += String(UserVar[event->BaseVarIndex+1]);
            log += F(" Humidity 20C: ");
            log += String(humidity20C);
            addLog(LOG_LEVEL_DEBUG, log);

            //sendData(event);
         }else
          {
            logError(ret);  
          }

      }

  }
  return success;
}


void logError(int error) 
{
  if (error>=0) // No error
    return;
    
  String log = "Sensirion SHT Error: ";
  log += String(error);

  switch (error) 
  {
    case S_Err_Param:
      log += F(" - Parameter error in function call! ");
      break;
    case S_Err_NoACK:
      log += F(" - No response (ACK) from sensor! ");
      break;
    case S_Err_CRC:
      log += F(" - CRC mismatch! ");
      break;
    case S_Err_TO:
      log += F(" - Measurement timeout! ");
      break;
    default:
      log += F(" - Unknown error received! ");
      break;
  }
  addLog(LOG_LEVEL_ERROR, log);
}


