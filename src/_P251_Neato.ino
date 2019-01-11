#ifdef USES_P251

//#include <cdcacm.h>
//#include <usbhub.h>

//#include <SPI.h>

USB     *Usb;

#define PLUGIN_251
#define PLUGIN_ID_251    251               //plugin id
#define PLUGIN_NAME_251   "Robot - Neato USB"
#define PLUGIN_251_DEBUG  false             //set to true for extra log info in the debug

#define PLUGIN_VALUENAME1_251 "p1"
#define PLUGIN_VALUENAME2_251 "p2"
#define PLUGIN_VALUENAME3_251 "p3"

boolean Plugin_251(byte function, struct EventStruct *event, String& string){
	boolean success = false;
  switch (function){
    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_251;
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
        string = F(PLUGIN_NAME_251);
        break;
      }
/*
    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_251));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[1], PSTR(PLUGIN_VALUENAME2_251));
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[2], PSTR(PLUGIN_VALUENAME3_251));
        break;
      }
*/    case PLUGIN_INIT:
			success=true;
			break;
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
	}
  return success;
}


#endif // USES_P251