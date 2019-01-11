#ifdef USES_P249
/*
  Plugin is based upon SenseAir plugin by Daniel Tedenljung info__AT__tedenljungconsulting.com
   and Dmitry (rel22 ___ inbox.ru)
  Additional features based on https://geektimes.ru/post/285572/ by Gerben (infernix__AT__gmail.com)

  MHZ14:  Connection:
  VCC     5 V
  GND     GND
  Tx   -> RX ESP8266 1st GPIO specified in Device-settings
  Rx   <- TX ESP8266 2nd GPIO specified in Device-settings
*/

#define PLUGIN_249
#define PLUGIN_ID_249         249
#define PLUGIN_NAME_249       "Gases - CO2 MH-Z14A"
#define PLUGIN_VALUENAME1_249 "PPM"
#define PLUGIN_READ_TIMEOUT   3000

#define PLUGIN_249_FILTER_OFF        1
#define PLUGIN_249_FILTER_OFF_ALLSAMPLES 2
#define PLUGIN_249_FILTER_FAST       3
#define PLUGIN_249_FILTER_MEDIUM     4
#define PLUGIN_249_FILTER_SLOW       5

boolean Plugin_249_init = false;

#include <ESPeasySoftwareSerial.h>
ESPeasySoftwareSerial *Plugin_249_SoftSerial;

enum mhz14Commands : byte { mhz14CmdReadPPM,
                        };
// 9 byte commands:
// mhzCmdReadPPM[]              = {0xFF,0x01,0x86,0x00,0x00,0x00,0x00,0x00,0x79};
// mhzCmdCalibrateZero[]        = {0xFF,0x01,0x87,0x00,0x00,0x00,0x00,0x00,0x78};
// mhzCmdABCEnable[]            = {0xFF,0x01,0x79,0xA0,0x00,0x00,0x00,0x00,0xE6};
// mhzCmdABCDisable[]           = {0xFF,0x01,0x79,0x00,0x00,0x00,0x00,0x00,0x86};
// mhzCmdReset[]                = {0xFF,0x01,0x8d,0x00,0x00,0x00,0x00,0x00,0x72};
/* It seems the offsets [3]..[4] for the detection range setting (command byte 0x99) are wrong in the latest
 * online data sheet: http://www.winsen-sensor.com/d/files/infrared-gas-sensor/mh-z19b-co2-ver1_0.pdf
 * According to the MH-Z19B datasheet version 1.2, valid from: 2017.03.22 (received 2018-03-07)
 * the offset should be [6]..[7] instead.
 * 0x99 - Detection range setting, send command:
 * /---------+---------+---------+---------+---------+---------+---------+---------+---------\
 * | Byte 0  | Byte 1  | Byte 2  | Byte 3  | Byte 4  | Byte 5  | Byte 6  | Byte 7  | Byte 8  |
 * |---------+---------+---------+---------+---------+---------+---------+---------+---------|
 * | Start   | Reserved| Command | Reserved|Detection|Detection|Detection|Detection| Checksum|
 * | Byte    |         |         |         |range    |range    |range    |range    |         |
 * |         |         |         |         |24~32 bit|16~23 bit|8~15 bit |0~7 bit  |         |
 * |---------+---------+---------+---------+---------+---------+---------+---------+---------|
 * | 0xFF    | 0x01    | 0x99    | 0x00    | Data 1  | Data 2  | Data 3  | Data 4  | Checksum|
 * \---------+---------+---------+---------+---------+---------+---------+---------+---------/
 * Note: Detection range should be 0~2000, 0~5000, 0~10000 ppm.
 * For example: set 0~2000 ppm  detection range, send command: FF 01 99 00 00 00 07 D0 8F
 *              set 0~10000 ppm detection range, send command: FF 01 99 00 00 00 27 10 8F
 * The latter, updated version above is implemented here.
 */
// mhzCmdMeasurementRange1000[] = {0xFF,0x01,0x99,0x00,0x00,0x00,0x03,0xE8,0x7B};
// mhzCmdMeasurementRange2000[] = {0xFF,0x01,0x99,0x00,0x00,0x00,0x07,0xD0,0x8F};
// mhzCmdMeasurementRange3000[] = {0xFF,0x01,0x99,0x00,0x00,0x00,0x0B,0xB8,0xA3};
// mhzCmdMeasurementRange5000[] = {0xFF,0x01,0x99,0x00,0x00,0x00,0x13,0x88,0xCB};
// Removing redundant data, just keeping offsets [2], [6]..[7]:
const PROGMEM byte mhz14CmdData[][3] = {
  {0x86,0x00,0x00},
  {0x87,0x00,0x00},
  {0x79,0xA0,0x00},
  {0x79,0x00,0x00},
  {0x8d,0x00,0x00},
#ifdef ENABLE_DETECTION_RANGE_COMMANDS
  {0x99,0x03,0xE8},
  {0x99,0x07,0xD0},
  {0x99,0x0B,0xB8},
  {0x99,0x13,0x88}
#endif
  };

byte mhz14Resp[9];    // 9 byte response buffer


boolean Plugin_249_Check_and_ApplyFilter(unsigned int prevVal, unsigned int &newVal, const int filterValue, String& log) {
  if (prevVal < 400 || prevVal > 5000) {
    return true;
  }
  boolean filterApplied = filterValue > PLUGIN_249_FILTER_OFF_ALLSAMPLES;
  int32_t difference = newVal - prevVal;
  switch (filterValue) {
    case PLUGIN_249_FILTER_OFF:
      filterApplied = false;
      break;
    case PLUGIN_249_FILTER_OFF_ALLSAMPLES: filterApplied = false; break; // No Delay
    case PLUGIN_249_FILTER_FAST:    difference /= 2; break; // Delay: 2 samples
    case PLUGIN_249_FILTER_MEDIUM:  difference /= 4; break; // Delay: 5 samples
    case PLUGIN_249_FILTER_SLOW:    difference /= 8; break; // Delay: 11 samples
  }
  if (filterApplied) {
    log += F("Raw PPM: ");
    log += newVal;
    log += F(" Filtered ");
  }
  newVal = static_cast<unsigned int>(prevVal + difference);
  return true;
}

boolean Plugin_249(byte function, struct EventStruct *event, String& string)
{
  bool success = false;

  switch (function)
  {

    case PLUGIN_DEVICE_ADD:
      {
        Device[++deviceCount].Number = PLUGIN_ID_249;
        Device[deviceCount].Type = DEVICE_TYPE_DUAL;
        Device[deviceCount].VType = SENSOR_TYPE_SINGLE;
        Device[deviceCount].Ports = 0;
        Device[deviceCount].PullUpOption = false;
        Device[deviceCount].InverseLogicOption = false;
        Device[deviceCount].FormulaOption = true;
        Device[deviceCount].ValueCount = 1;
        Device[deviceCount].SendDataOption = true;
        Device[deviceCount].TimerOption = true;
        Device[deviceCount].GlobalSyncOption = true;
        break;
      }

    case PLUGIN_GET_DEVICENAME:
      {
        string = F(PLUGIN_NAME_249);
        break;
      }

    case PLUGIN_GET_DEVICEVALUENAMES:
      {
        strcpy_P(ExtraTaskSettings.TaskDeviceValueNames[0], PSTR(PLUGIN_VALUENAME1_249));
        break;
      }

    case PLUGIN_WEBFORM_LOAD:
      {
        addFormNote(F("<b>GPIO:</b> pin1=RX pin2=TX"));
        byte choiceFilter = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
        String filteroptions[5] = { F("Skip Unstable"), F("Use Unstable"), F("Fast Response"), F("Medium Response"), F("Slow Response") };
        int filteroptionValues[5] = {
          PLUGIN_249_FILTER_OFF,
          PLUGIN_249_FILTER_OFF_ALLSAMPLES,
          PLUGIN_249_FILTER_FAST,
          PLUGIN_249_FILTER_MEDIUM,
          PLUGIN_249_FILTER_SLOW };
        addFormSelector(F("Filter"), F("plugin_249_filter"), 5, filteroptions, filteroptionValues, choiceFilter);

        success = true;
        break;
      }

    case PLUGIN_WEBFORM_SAVE:
      {
        const int filterValue = getFormItemInt(F("plugin_249_filter"));
        Settings.TaskDevicePluginConfig[event->TaskIndex][0] = filterValue;
        success = true;
        break;
      }

    case PLUGIN_INIT:
      {
        Plugin_249_SoftSerial = new ESPeasySoftwareSerial(Settings.TaskDevicePin1[event->TaskIndex], Settings.TaskDevicePin2[event->TaskIndex]);
        Plugin_249_SoftSerial->begin(9600);
        addLog(LOG_LEVEL_INFO, F("MHZ14: Init OK "));

        //delay first read, because hardware needs to initialize on cold boot
        //otherwise we get a weird value or read error
        schedule_task_device_timer(event->TaskIndex, millis() + 15000);

        Plugin_249_init = true;
        success = true;
        break;
      }

    case PLUGIN_READ:
      {

        if (Plugin_249_init)
        {
          //send read PPM command
          byte nbBytesSent = _P249_send_mhzCmd(mhz14CmdReadPPM);
          if (nbBytesSent != 9) {
            String log = F("MHZ14: Error, nb bytes sent != 9 : ");
              log += nbBytesSent;
              addLog(LOG_LEVEL_INFO, log);
          }

          // get response
          memset(mhz14Resp, 0, sizeof(mhz14Resp));

          long timer = millis() + PLUGIN_READ_TIMEOUT;
          int counter = 0;
          while (!timeOutReached(timer) && (counter < 9)) {
            if (Plugin_249_SoftSerial->available() > 0) {
              mhz14Resp[counter++] = Plugin_249_SoftSerial->read();
            } else {
              delay(10);
            }
          }
          if (counter < 9){
              addLog(LOG_LEVEL_INFO, F("MHZ14: Error, timeout while trying to read"));
          }

          unsigned int ppm = 0;
          byte checksum = _P249_calculateChecksum(mhz14Resp);

          if ( !(mhz14Resp[8] == checksum) ) {
             String log = F("MHZ14: Read error: checksum = ");
             log += String(checksum); log += " / "; log += String(mhz14Resp[8]);
             log += " bytes read  => ";for (byte i = 0; i < 9; i++) {log += mhz14Resp[i];log += "/" ;}
             addLog(LOG_LEVEL_ERROR, log);

             // Sometimes there is a misalignment in the serial read
             // and the starting byte 0xFF isn't the first read byte.
             // This goes on forever.
             // There must be a better way to handle this, but here
             // we're trying to shift it so that 0xFF is the next byte
             byte checksum_shift;
             for (byte i = 1; i < 8; i++) {
                checksum_shift = Plugin_249_SoftSerial->peek();
                if (checksum_shift == 0xFF) {
                  String log = F("MHZ14: Shifted ");
                  log += i;
                  log += F(" bytes to attempt to fix buffer alignment");
                  addLog(LOG_LEVEL_ERROR, log);
                  break;
                } else {
                 checksum_shift = Plugin_249_SoftSerial->read();
                }
             }
             success = false;
             break;

          // Process responses to 0x86
          } else if (mhz14Resp[0] == 0xFF && mhz14Resp[1] == 0x86 && mhz14Resp[8] == checksum)  {

              //calculate CO2 PPM
              unsigned int mhz14RespHigh = (unsigned int) mhz14Resp[2];
              unsigned int mhz14RespLow = (unsigned int) mhz14Resp[3];
              ppm = (256*mhz14RespHigh) + mhz14RespLow;
              String log = F("MHZ14: ");

              const int filterValue = Settings.TaskDevicePluginConfig[event->TaskIndex][0];
              if (Plugin_249_Check_and_ApplyFilter(UserVar[event->BaseVarIndex], ppm, filterValue, log)) {
                UserVar[event->BaseVarIndex] = (float)ppm;
                success = true;
              } else {
                success = false;
              }

              // Log values in all cases
              log += F("PPM value: ");
              log += ppm;
              addLog(LOG_LEVEL_INFO, log);
              break;

          // log verbosely anything else that the sensor reports
          } else {

              String log = F("MHZ14: Unknown response:");
              for (int i = 0; i < 9; ++i) {
                log += F(" ");
                log += String(mhz14Resp[i], HEX);
              }
              addLog(LOG_LEVEL_INFO, log);
              success = false;
              break;

          }

        }
        break;
      }
  }
  return success;
}

byte _P249_calculateChecksum(byte *array)
{
  byte checksum = 0;
  for (byte i = 1; i < 8; i++)
    checksum+=array[i];
  checksum = 0xFF - checksum;
  return (checksum+1);
}

size_t _P249_send_mhzCmd(byte CommandId)
{
  // The receive buffer "mhz14Resp" is re-used to send a command here:
  mhz14Resp[0] = 0xFF; // Start byte, fixed
  mhz14Resp[1] = 0x01; // Sensor number, 0x01 by default
  memcpy_P(&mhz14Resp[2], mhz14CmdData[CommandId], sizeof(mhz14CmdData[0]));
  mhz14Resp[6] = mhz14Resp[3]; mhz14Resp[7] = mhz14Resp[4];
  mhz14Resp[3] = mhz14Resp[4] = mhz14Resp[5] = 0x00;
  mhz14Resp[8] = _P249_calculateChecksum(mhz14Resp);

  return Plugin_249_SoftSerial->write(mhz14Resp, sizeof(mhz14Resp));
}
#endif // USES_P249
