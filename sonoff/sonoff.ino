/*
 * Sonoff, ElectroDragon and Wkaku by Theo Arends
 * 
 * ==================================================
 * Prerequisites:
 *   Change libraries/PubSubClient/src/PubSubClient.h
 *     #define MQTT_MAX_PACKET_SIZE 256
 *     #define MQTT_KEEPALIVE 120
 *   Select IDE Tools - Flash size: "1M (64K SPIFFS)"
 * ==================================================
 *
 * ESP-12F connections (Wkaku)
 * 3V3                                                     5V
 *                   |-------------------|       |---------|
 *  |                |   -------------   |    |1N4001|  |Relay|
 *  |                | -|          Tx |- |       |---------|
 *  |                | -|          Rx |- |                /
 *  |-------------------| En          |- |---| 1k|------|<  BC547B
 *  |                | -|             |-                  \
 *  |                | -|        IO00 |------|Switch|------|
 *  |                ---| IO12   IO02 |--- LED (ESP-12E/F) |
 *  |---| 1k|---|LED|---| IO13   IO15 |------|10k|---------|
 *  |-------------------| Vcc     Gnd |--------------------|
 *                       -------------                     |
 *                        | | | | | |                     Gnd
*/

#define VERSION                0x02000600   // 2.0.6

#define SONOFF                 1            // Sonoff, Sonoff SV, Sonoff Dual, Sonoff TH 10A/16A, S20 Smart Socket, 4 Channel
#define SONOFF_POW             9            // Sonoff Pow
#define ELECTRO_DRAGON         10           // Electro Dragon Wifi IoT Relay Board Based on ESP8266

#define DHT11                  11
#define DHT21                  21
#define DHT22                  22
#define AM2301                 21
#define AM2302                 22
#define AM2321                 22

enum log_t   {LOG_LEVEL_NONE, LOG_LEVEL_ERROR, LOG_LEVEL_INFO, LOG_LEVEL_DEBUG, LOG_LEVEL_DEBUG_MORE, LOG_LEVEL_ALL};
enum week_t  {Last, First, Second, Third, Fourth}; 
enum dow_t   {Sun=1, Mon, Tue, Wed, Thu, Fri, Sat};
enum month_t {Jan=1, Feb, Mar, Apr, May, Jun, Jul, Aug, Sep, Oct, Nov, Dec};
enum wifi_t  {WIFI_STATUS, WIFI_SMARTCONFIG, WIFI_MANAGER, WIFI_WPSCONFIG};

#include "user_config.h"

/*********************************************************************************************\
 * Enable feature by removing leading // or disable feature by adding leading //
\*********************************************************************************************/

#define USE_TICKER                          // Enable interrupts to keep RTC synced during subscription flooding
//#define USE_SPIFFS                          // Switch persistent configuration from flash to spiffs (+24k code, +0.6k mem)
#define USE_WEBSERVER                       // Enable web server and wifi manager (+37k code, +2k mem)

#if MODULE == SONOFF_POW
  #define USE_POWERMONITOR                  // Enable Power Monitoring
#endif

/*********************************************************************************************\
 * No user configurable items below
\*********************************************************************************************/

#define SONOFF_DUAL            2            // (iTEAD PSB)
#define CHANNEL_3              3            // iTEAD PSB
#define CHANNEL_4              4            // iTEAD PSB
#define CHANNEL_5              5
#define CHANNEL_6              6
#define CHANNEL_7              7
#define CHANNEL_8              8

#define DEF_WIFI_HOSTNAME      "%s-%04d"    // Expands to <MQTT_TOPIC>-<last 4 decimal chars of MAC address>
#define DEF_MQTT_CLIENT_ID     "DVES_%06X"  // Also fall back topic using Chip Id = last 6 characters of MAC address

#define MQTT_UNITS             0            // Default do not show value units (Hr, Sec, V, A, W etc.)
#define MQTT_SUBTOPIC          "POWER"      // Default MQTT subtopic (POWER or LIGHT)
#define APP_POWER              0            // Default saved power state Off
#define MAX_DEVICE             1            // Max number of devices

#define STATES                 10           // loops per second
#define MQTT_RETRY_SECS        10           // Seconds to retry MQTT connection

#define INPUT_BUFFER_SIZE      128          // Max number of characters in serial buffer
#define TOPSZ                  40           // Max number of characters in topic string
#define MESSZ                  200          // Max number of characters in message string (Syntax string)
#define LOGSZ                  128          // Max number of characters in log string

#define MAX_LOG_LINES          80           // Max number of lines in weblog

#define APP_BAUDRATE           115200       // Default serial baudrate

#ifdef USE_POWERMONITOR
  #define MAX_STATUS           9
#else
  #define MAX_STATUS           7
#endif

enum butt_t {PRESSED, NOT_PRESSED};

#include <ESP8266WiFi.h>                    // MQTT, Ota, WifiManager
#include <ESP8266HTTPClient.h>              // MQTT, Ota
#include <ESP8266httpUpdate.h>              // Ota
#include <PubSubClient.h>                   // MQTT
#ifdef USE_WEBSERVER
  #include <ESP8266WebServer.h>             // WifiManager, Webserver
  #include <DNSServer.h>                    // WifiManager
#endif  // USE_WEBSERVER
#ifdef USE_TICKER
  #include <Ticker.h>                       // RTC
#endif  // USE_TICKER
#ifdef USE_SPIFFS
  #include <FS.h>                           // Config
#endif

extern "C" uint32_t _SPIFFS_start;
extern "C" uint32_t _SPIFFS_end;

#define MAX_BUTTON_COMMANDS    8

const char commands[MAX_BUTTON_COMMANDS][14] PROGMEM = {
  {"reset 1"},        // Hold button for more than 4 seconds
  {"1/light 2"},      // Press button once
  {"2/light 2"},      // Press button twice - will default to "1/light 2" if Maxdevice = 1
  {"wificonfig 1"},   // Press button three times
  {"wificonfig 2"},   // Press button four times
  {"wificonfig 3"},   // Press button five times
  {"restart 1"},      // Press button six times
  {"upgrade 1"}};     // Press button seven times

struct SYSCFG {
  unsigned long cfg_holder;
  unsigned long saveFlag;
  unsigned long version;
  byte          seriallog_level;
  byte          syslog_level;
  char          syslog_host[32];
  char          sta_ssid[32];
  char          sta_pwd[64];
  char          otaUrl[80];
  char          mqtt_host[32];
  char          mqtt_grptopic[32];
  char          mqtt_topic[32];
  char          mqtt_topic2[32];
  char          mqtt_subtopic[32];
  int8_t        timezone;
  uint8_t       power;
  uint8_t       ledstate;
  uint16_t      mqtt_port;
  char          mqtt_client[33];
  char          mqtt_user[33];
  char          mqtt_pwd[33];
  uint8_t       webserver;
  unsigned long bootcount;
  char          hostname[33];
  uint16_t      syslog_port;
  byte          weblog_level;
  uint16_t      tele_period;
  uint8_t       sta_config;
  int16_t       savedata;
  byte          model;
  byte          mqtt_retain;
  byte          savestate;
  unsigned long hlw_pcal;
  unsigned long hlw_ucal;
  unsigned long hlw_ical;
  unsigned long hlw_esave;
  byte          mqtt_units;
  uint16_t      hlw_pmin;
  uint16_t      hlw_pmax;
  uint16_t      hlw_umin;
  uint16_t      hlw_umax;
  uint16_t      hlw_imin;
  uint16_t      hlw_imax;
} sysCfg;

struct TIME_T {
  uint8_t       Second;
  uint8_t       Minute;
  uint8_t       Hour;
  uint8_t       Wday;      // day of week, sunday is day 1
  uint8_t       Day;
  uint8_t       Month;
  char          MonthName[4];
  uint16_t      Year;
  unsigned long Valid;
} rtcTime;

struct TimeChangeRule
{
  uint8_t       week;      // 1=First, 2=Second, 3=Third, 4=Fourth, or 0=Last week of the month
  uint8_t       dow;       // day of week, 1=Sun, 2=Mon, ... 7=Sat
  uint8_t       month;     // 1=Jan, 2=Feb, ... 12=Dec
  uint8_t       hour;      // 0-23
  int           offset;    // offset from UTC in minutes
};

TimeChangeRule myDST = { TIME_DST };  // Daylight Saving Time
TimeChangeRule mySTD = { TIME_STD };  // Standard Time

int Baudrate = APP_BAUDRATE;          // Serial interface baud rate
byte SerialInByte;                    // Received byte
int SerialInByteCounter = 0;          // Index in receive buffer
char serialInBuf[INPUT_BUFFER_SIZE + 2];  // Receive buffer
byte Hexcode = 0;                     // Sonoff dual input flag
uint16_t ButtonCode = 0;              // Sonoff dual received code
int16_t savedatacounter;              // Counter and flag for config save to Flash or Spiffs
char Version[16];                     // Version string from VERSION define
char Hostname[33];                    // Composed Wifi hostname
char MQTTClient[33];                  // Composed MQTT Clientname
uint8_t mqttcounter = 0;              // MQTT connection retry counter
unsigned long timerxs = 0;            // State loop timer
#ifndef USE_TICKER
  unsigned long timersec = 0;         // Second counter if interrupt is not used
#endif  // USE_TICKER
int state = 0;                        // State per second flag
int mqttflag = 1;                     // MQTT connection messages flag
int otaflag = 0;                      // OTA state flag
int otaok;                            // OTA result
int restartflag = 0;                  // Sonoff restart flag
int wificheckflag = WIFI_STATUS;      // Wifi state flag
int uptime = 0;                       // Current uptime in hours
int tele_period = 0;                  // Tele period timer
String Log[MAX_LOG_LINES];            // Web log buffer
byte logidx = 0;                      // Index in Web log buffer
byte Maxdevice = MAX_DEVICE;          // Max number of devices supported

WiFiClient espClient;                 // Wifi Client
PubSubClient mqttClient(espClient);   // MQTT Client
WiFiUDP portUDP;                      // UDP Syslog

uint8_t power;                        // Current copy of sysCfg.power

int blinks = 1;                       // Number of LED blinks
uint8_t blinkstate = 0;               // LED state

uint8_t lastbutton = NOT_PRESSED;     // Last button state
uint8_t holdcount = 0;                // Timer recording button hold
uint8_t multiwindow = 0;              // Max time between button presses to record press count
uint8_t multipress = 0;               // Number of button presses within multiwindow

/********************************************************************************************/

void CFG_Default()
{
  addLog_P(LOG_LEVEL_INFO, PSTR("Config: Use default configuration"));
  memset(&sysCfg, 0x00, sizeof(SYSCFG));
  sysCfg.cfg_holder = CFG_HOLDER;
  sysCfg.saveFlag = 0;
  sysCfg.version = VERSION;
  sysCfg.bootcount = 0;
  sysCfg.savedata = SAVE_DATA;
  sysCfg.savestate = SAVE_STATE;
  sysCfg.weblog_level = WEB_LOG_LEVEL;
  sysCfg.seriallog_level = SERIAL_LOG_LEVEL;
  sysCfg.syslog_level = SYS_LOG_LEVEL;
  strlcpy(sysCfg.syslog_host, SYS_LOG_HOST, sizeof(sysCfg.syslog_host));
  sysCfg.syslog_port = SYS_LOG_PORT;
  strlcpy(sysCfg.sta_ssid, STA_SSID, sizeof(sysCfg.sta_ssid));
  strlcpy(sysCfg.sta_pwd, STA_PASS, sizeof(sysCfg.sta_pwd));
  sysCfg.sta_config = WIFI_CONFIG_TOOL;
  strlcpy(sysCfg.hostname, WIFI_HOSTNAME, sizeof(sysCfg.hostname));
  strlcpy(sysCfg.otaUrl, OTA_URL, sizeof(sysCfg.otaUrl));
  strlcpy(sysCfg.mqtt_host, MQTT_HOST, sizeof(sysCfg.mqtt_host));
  sysCfg.mqtt_port = MQTT_PORT;
  strlcpy(sysCfg.mqtt_client, MQTT_CLIENT_ID, sizeof(sysCfg.mqtt_client));
  strlcpy(sysCfg.mqtt_user, MQTT_USER, sizeof(sysCfg.mqtt_user));
  strlcpy(sysCfg.mqtt_pwd, MQTT_PASS, sizeof(sysCfg.mqtt_pwd));
  strlcpy(sysCfg.mqtt_grptopic, MQTT_GRPTOPIC, sizeof(sysCfg.mqtt_grptopic));
  strlcpy(sysCfg.mqtt_topic, MQTT_TOPIC, sizeof(sysCfg.mqtt_topic));
  strlcpy(sysCfg.mqtt_topic2, "0", sizeof(sysCfg.mqtt_topic2));
  strlcpy(sysCfg.mqtt_subtopic, MQTT_SUBTOPIC, sizeof(sysCfg.mqtt_subtopic));
  sysCfg.mqtt_retain = MQTT_BUTTON_RETAIN;
  sysCfg.mqtt_units = MQTT_UNITS;
  sysCfg.tele_period = TELE_PERIOD;
  sysCfg.timezone = APP_TIMEZONE;
  sysCfg.power = APP_POWER;
  sysCfg.ledstate = APP_LEDSTATE;
  sysCfg.webserver = WEB_SERVER;
  sysCfg.model = 0;
  sysCfg.hlw_pcal = 0;
  sysCfg.hlw_ucal = 0;
  sysCfg.hlw_ical = 0;
  sysCfg.hlw_esave = 0;
  sysCfg.hlw_pmin = 0;
  sysCfg.hlw_pmax = 0;
  sysCfg.hlw_umin = 0;
  sysCfg.hlw_umax = 0;
  sysCfg.hlw_imin = 0;
  sysCfg.hlw_imax = 0;
  CFG_Save();
}

void CFG_Delta()
{
  if (sysCfg.version != VERSION) {      // Fix version dependent changes
    if (sysCfg.version < 0x01000D00) {  // 1.0.13 - Add ledstate
      sysCfg.ledstate = APP_LEDSTATE;
    }
    if (sysCfg.version < 0x01001600) {  // 1.0.22 - Add MQTT parameters
      sysCfg.mqtt_port = MQTT_PORT;
      strlcpy(sysCfg.mqtt_client, MQTT_CLIENT_ID, sizeof(sysCfg.mqtt_client));
      strlcpy(sysCfg.mqtt_user, MQTT_USER, sizeof(sysCfg.mqtt_user));
      strlcpy(sysCfg.mqtt_pwd, MQTT_PASS, sizeof(sysCfg.mqtt_pwd));
    }
    if (sysCfg.version < 0x01001700) {  // 1.0.23 - Add webserver parameters
      sysCfg.webserver = WEB_SERVER;
    }
    if (sysCfg.version < 0x01001A00) {  // 1.0.26 - Add bootcount
      sysCfg.bootcount = 0;
      strlcpy(sysCfg.hostname, WIFI_HOSTNAME, sizeof(sysCfg.hostname));
      sysCfg.syslog_port = SYS_LOG_PORT;
    }
    if (sysCfg.version < 0x01001B00) {  // 1.0.27 - Add web log level
      sysCfg.weblog_level = WEB_LOG_LEVEL;
    }
    if (sysCfg.version < 0x01001C00) {  // 1.0.28 - Add telemetry parameter
      sysCfg.tele_period = TELE_PERIOD;
    }
    if (sysCfg.version < 0x01002000) {  // 1.0.32 - Add default Wifi config
      sysCfg.sta_config = WIFI_CONFIG_TOOL;
    }
    if (sysCfg.version < 0x01002300) {  // 1.0.35 - Add default savedata flag
      sysCfg.savedata = SAVE_DATA;
    }
    if (sysCfg.version < 0x02000000) {  // 2.0.0 - Add default model
      sysCfg.model = 0;
    }
    if (sysCfg.version < 0x02000300) {  // 2.0.3 - Add button retain flag
      sysCfg.mqtt_retain = MQTT_BUTTON_RETAIN;
      sysCfg.savestate = SAVE_STATE;
    }
    if (sysCfg.version < 0x02000500) {  // 2.0.5 - Add pow calibration
      sysCfg.hlw_pcal = 0;
      sysCfg.hlw_ucal = 0;
      sysCfg.hlw_ical = 0;
      sysCfg.hlw_esave = 0;
      sysCfg.mqtt_units = MQTT_UNITS;
    }
    if (sysCfg.version < 0x02000600) {  // 2.0.6 - Add pow thresholds
      sysCfg.hlw_pmin = 0;
      sysCfg.hlw_pmax = 0;
      sysCfg.hlw_umin = 0;
      sysCfg.hlw_umax = 0;
      sysCfg.hlw_imin = 0;
      sysCfg.hlw_imax = 0;
    }
    
    sysCfg.version = VERSION;
  }
}

/********************************************************************************************/

void setRelay(uint8_t power)
{
  if ((sysCfg.model >= SONOFF_DUAL) && (sysCfg.model <= CHANNEL_4)) {
    Serial.write(0xA0);
    Serial.write(0x04);
    Serial.write(power);
    Serial.write(0xA1);
    Serial.write('\n');
/*
    Serial.print("\xA0\x04");
    Serial.write(power);
    Serial.println("\xA1");
*/
    Serial.flush();
  } else {
    digitalWrite(REL_PIN, power & 0x1);
  }
}

/********************************************************************************************/

void mqtt_publish(const char* topic, const char* data, boolean retained)
{
  char log[TOPSZ+MESSZ];
  
  mqttClient.publish(topic, data, retained);
  snprintf_P(log, sizeof(log), PSTR("MQTT: %s = %s%s"), strchr(topic,'/')+1, data, (retained) ? " (retained)" : ""); // Skip topic prefix
  addLog(LOG_LEVEL_INFO, log);
//  mqttClient.loop();  // Do not use here! Will block previous publishes
  blinks++;
}

void mqtt_publish(const char* topic, const char* data)
{
  mqtt_publish(topic, data, false);
}

void mqtt_connected()
{
  char stopic[TOPSZ], svalue[MESSZ];

  snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/#"), SUB_PREFIX, sysCfg.mqtt_topic);
  mqttClient.subscribe(stopic);
  mqttClient.loop();  // Solve LmacRxBlk:1 messages
  snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/#"), SUB_PREFIX, sysCfg.mqtt_grptopic);
  mqttClient.subscribe(stopic);
  mqttClient.loop();  // Solve LmacRxBlk:1 messages
  snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/#"), SUB_PREFIX, MQTTClient); // Fall back topic
  mqttClient.subscribe(stopic);
  mqttClient.loop();  // Solve LmacRxBlk:1 messages

  if (mqttflag) {
    snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/INFO"), PUB_PREFIX, sysCfg.mqtt_topic);
    snprintf_P(svalue, sizeof(svalue), PSTR(APP_NAME " version %s, Fallback %s, GroupTopic %s"), Version, MQTTClient, sysCfg.mqtt_grptopic);
    mqtt_publish(stopic, svalue);
#ifdef USE_WEBSERVER
    if (sysCfg.webserver) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/INFO"), PUB_PREFIX, sysCfg.mqtt_topic);
      snprintf_P(svalue, sizeof(svalue), PSTR("Webserver active for %s on %s with IP address %s"),
        (sysCfg.webserver == 2) ? "Admin" : "User", Hostname, WiFi.localIP().toString().c_str());
      mqtt_publish(stopic, svalue);
    }
#endif  // USE_WEBSERVER
    if (MQTT_MAX_PACKET_SIZE < (TOPSZ+MESSZ)) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/WARNING"), PUB_PREFIX, sysCfg.mqtt_topic);
      snprintf_P(svalue, sizeof(svalue), PSTR("Change MQTT_MAX_PACKET_SIZE in libraries/PubSubClient.h to at least %d"), TOPSZ+MESSZ);
      mqtt_publish(stopic, svalue);
    }
    if (!spiffsPresent()) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/WARNING"), PUB_PREFIX, sysCfg.mqtt_topic);
      snprintf_P(svalue, sizeof(svalue), PSTR("No persistent config. Please reflash with at least 16K SPIFFS"));
      mqtt_publish(stopic, svalue);
    }
  }
  mqttflag = 0;
}

void mqtt_reconnect()
{
  char stopic[TOPSZ], log[LOGSZ];

  mqttcounter = MQTT_RETRY_SECS;
  addLog_P(LOG_LEVEL_INFO, PSTR("MQTT: Attempting connection..."));
  snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/lwt"), PUB_PREFIX, sysCfg.mqtt_topic);
  if (mqttClient.connect(MQTTClient, sysCfg.mqtt_user, sysCfg.mqtt_pwd, stopic, 0, 0, "offline")) {
    addLog_P(LOG_LEVEL_INFO, PSTR("MQTT: Connected"));
    mqttcounter = 0;
    mqtt_connected();
  } else {
    snprintf_P(log, sizeof(log), PSTR("MQTT: CONNECT FAILED, rc %d. Retry in %d seconds"), mqttClient.state(), mqttcounter);
    addLog(LOG_LEVEL_DEBUG, log);
  }
}

void mqttDataCb(char* topic, byte* data, unsigned int data_len)
{
  int i, grpflg = 0, device;
  float ped, pi, pc;
  uint16_t pe, pw, pu;
  char *str, *p, *mtopic = NULL, *type = NULL, *devc = NULL;
  char stopic[TOPSZ], svalue[MESSZ], stemp1[TOPSZ], stemp2[10], stemp3[10];

  int topic_len = strlen(topic);
  char topicBuf[topic_len +1]; 
  char dataBuf[data_len +1]; 
  char dataBufUc[data_len +1]; 
  
  memcpy(topicBuf, topic, topic_len);
  topicBuf[topic_len] = 0;

  memcpy(dataBuf, data, data_len);
  dataBuf[data_len] = 0;

  snprintf_P(svalue, sizeof(svalue), PSTR("MQTT: Receive topic %s, data %s"), topicBuf, dataBuf);
  addLog(LOG_LEVEL_DEBUG, svalue);

  i = 0;
  for (str = strtok_r(topicBuf, "/", &p); str && i < 4; str = strtok_r(NULL, "/", &p)) {
    switch (i++) {
    case 0:  // cmnd
      break;
    case 1:  // Topic / GroupTopic / DVES_123456
      mtopic = str;
      break;
    case 2:  // TopicIndex / Text
      type = str;
      break;
    case 3:  // Text
      devc = str;
    }
  }
  if (!strcmp(mtopic, sysCfg.mqtt_grptopic)) grpflg = 1;

  device = 1;
  if (devc != NULL) {
    if (Maxdevice) {
      device = atoi(type);
      if ((device < 1) || (device > Maxdevice)) device = 1;
    }
    type = devc;
  }
  if (!device) type = NULL;
  
  if (type != NULL) for(i = 0; i < strlen(type); i++) type[i] = toupper(type[i]);

  for(i = 0; i <= data_len; i++) dataBufUc[i] = toupper(dataBuf[i]);

  snprintf_P(svalue, sizeof(svalue), PSTR("MQTT: DataCb Topic %s, Group %d, Device %d, Type %s, Data %s (%s)"),
    mtopic, grpflg, device, type, dataBuf, dataBufUc);
  addLog(LOG_LEVEL_DEBUG, svalue);

  if (type != NULL) {
    if (devc == NULL) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/%s"), PUB_PREFIX, sysCfg.mqtt_topic, type);
    } else {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/%d/%s"), PUB_PREFIX, sysCfg.mqtt_topic, device, type);
    }
    strlcpy(svalue, "Error", sizeof(svalue));

    if (!strcmp(dataBufUc,"?")) data_len = 0;
    int16_t payload = atoi(dataBuf);
    if (!strcmp(dataBufUc,"OFF") || !strcmp(dataBufUc,"STOP")) payload = 0;
    if (!strcmp(dataBufUc,"ON") || !strcmp(dataBufUc,"START") || !strcmp(dataBufUc,"USER")) payload = 1;
    if (!strcmp(dataBufUc,"TOGGLE") || !strcmp(dataBufUc,"ADMIN")) payload = 2;
    
    if (!strcmp(type,"STATUS")) {
      if ((data_len == 0) || (payload < 0) || (payload > MAX_STATUS)) payload = 99;
      if ((payload == 0) || (payload == 99)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("%s, %d, %s, %s, %s, %d, %d, %d, %d, %d, %d"),
          Version, sysCfg.model, sysCfg.mqtt_topic, sysCfg.mqtt_topic2, sysCfg.mqtt_subtopic, power, sysCfg.timezone, sysCfg.ledstate, sysCfg.savedata, sysCfg.savestate, sysCfg.mqtt_retain);
        if (payload == 0) mqtt_publish(stopic, svalue);
      }
#ifdef USE_POWERMONITOR
      if ((payload == 0) || (payload == 8)) {
        hlw_readEnergy(0, ped, pe, pw, pu, pi, pc);
        dtostrf(pi, 1, 3, stemp1);
        dtostrf(ped, 1, 3, stemp2);
        dtostrf(pc, 1, 2, stemp3);
        snprintf_P(svalue, sizeof(svalue), PSTR("PWR: Voltage %d V, Current %s A, Current Power %d W, Total Power Today %s kWh, Power Factor %s"),
          pu, stemp1, pw, stemp2, stemp3); 
        if (payload == 0) mqtt_publish(stopic, svalue);
      }
      if ((payload == 0) || (payload == 9)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("PWR: Threshold PowerLow %d W, PowerHigh %d W, VoltageLow %d V, VoltageHigh %d V, CurrentLow %d mA, CurrentHigh %d mA"),
          sysCfg.hlw_pmin, sysCfg.hlw_pmax, sysCfg.hlw_umin, sysCfg.hlw_umax, sysCfg.hlw_imin, sysCfg.hlw_imax); 
        if (payload == 0) mqtt_publish(stopic, svalue);
      }
#endif  // USE_POWERMONITOR      
      if ((payload == 0) || (payload == 1)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("PRM: Baudrate %d, GroupTopic %s, OtaUrl %s, Uptime %d Hr, Bootcount %d, SaveCount %d"),
          Baudrate, sysCfg.mqtt_grptopic, sysCfg.otaUrl, uptime, sysCfg.bootcount, sysCfg.saveFlag);
        if (payload == 0) mqtt_publish(stopic, svalue);
      }          
      if ((payload == 0) || (payload == 2)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("FWR: Version %s, Boot %d, SDK %s"),
          Version, ESP.getBootVersion(), ESP.getSdkVersion());
        if (payload == 0) mqtt_publish(stopic, svalue);
      }          
      if ((payload == 0) || (payload == 3)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("LOG: Seriallog %d, Weblog %d, Syslog %d, LogHost %s, SSId %s, Password %s, TelePeriod %d"),
          sysCfg.seriallog_level, sysCfg.weblog_level, sysCfg.syslog_level, sysCfg.syslog_host, sysCfg.sta_ssid, sysCfg.sta_pwd, sysCfg.tele_period);
        if (payload == 0) mqtt_publish(stopic, svalue);
      }          
      if ((payload == 0) || (payload == 4)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("MEM: Sketch size %dkB, Free %dkB (Heap %dkB), Spiffs start %dkB (%dkB), Flash size %dkB (%dkB)"),
          ESP.getSketchSize()/1024, ESP.getFreeSketchSpace()/1024, ESP.getFreeHeap()/1024, ((uint32_t)&_SPIFFS_start - 0x40200000)/1024,
          (((uint32_t)&_SPIFFS_end - 0x40200000) - ((uint32_t)&_SPIFFS_start - 0x40200000))/1024, ESP.getFlashChipRealSize()/1024, ESP.getFlashChipSize()/1024);
        if (payload == 0) mqtt_publish(stopic, svalue);
      }          
      if ((payload == 0) || (payload == 5)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("NET: Host %s, IP %s, Gateway %s, Subnetmask %s, Mac %s, Webserver %d, WifiConfig %d"),
          Hostname, WiFi.localIP().toString().c_str(), WiFi.gatewayIP().toString().c_str(), WiFi.subnetMask().toString().c_str(),
          WiFi.macAddress().c_str(), sysCfg.webserver, sysCfg.sta_config);
        if (payload == 0) mqtt_publish(stopic, svalue);
      }          
      if ((payload == 0) || (payload == 6)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("MQT: Host %s, Port %d, Client %s (%s), User %s, Password %s, MAX_PACKET_SIZE %d, KEEPALIVE %d"),
          sysCfg.mqtt_host, sysCfg.mqtt_port, sysCfg.mqtt_client, MQTTClient, sysCfg.mqtt_user, sysCfg.mqtt_pwd, MQTT_MAX_PACKET_SIZE, MQTT_KEEPALIVE); 
        if (payload == 0) mqtt_publish(stopic, svalue);
      }      
      if ((payload == 0) || (payload == 7)) {
        snprintf_P(svalue, sizeof(svalue), PSTR("TIM: UTC %s, Local %s, Start DST %s, End DST %s"),
          rtc_time(0).c_str(), rtc_time(1).c_str(), rtc_time(2).c_str(), rtc_time(3).c_str()); 
      }      
    }
    else if (!strcmp(type,"SAVEDATA")) {
      if ((data_len > 0) && (payload >= 0) && (payload <= 3600)) {
        sysCfg.savedata = payload;
        savedatacounter = sysCfg.savedata;
      }
      if (sysCfg.savestate) sysCfg.power = power;
      CFG_Save();
      if (sysCfg.savedata > 1) {
        snprintf_P(svalue, sizeof(svalue), PSTR("Every %d seconds"), sysCfg.savedata);
      } else {
        strlcpy(svalue, (sysCfg.savedata) ? "On" : "Manual", sizeof(svalue));
      }
    }
    else if (!strcmp(type,"SAVESTATE")) {
      if ((data_len > 0) && (payload >= 0) && (payload <= 1)) {
        sysCfg.savestate = payload;
      }
      strlcpy(svalue, (sysCfg.savestate) ? "ON" : "OFF", sizeof(svalue));
    }
    else if (!strcmp(type,"MODEL")) {
      if ((data_len > 0) && (payload >= 0) && (payload <= CHANNEL_4)) {
        sysCfg.model = payload;
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d"), sysCfg.model);
    }
    else if (!strcmp(type,"UPGRADE") || !strcmp(type,"UPLOAD")) {
      if ((data_len > 0) && (payload == 1)) {
        otaflag = 3;
        snprintf_P(svalue, sizeof(svalue), PSTR("Upgrade %s from %s"), Version, sysCfg.otaUrl);
      } else {
        snprintf_P(svalue, sizeof(svalue), PSTR("1 to upgrade"));
      }
    }
    else if (!strcmp(type,"OTAURL")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.otaUrl)))
        strlcpy(sysCfg.otaUrl, (payload == 1) ? OTA_URL : dataBuf, sizeof(sysCfg.otaUrl));
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.otaUrl);
    }
    else if (!strcmp(type,"SERIALLOG")) {
      if ((data_len > 0) && (payload >= LOG_LEVEL_NONE) && (payload <= LOG_LEVEL_ALL)) {
        sysCfg.seriallog_level = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d"), sysCfg.seriallog_level);
    }
    else if (!strcmp(type,"SYSLOG")) {
      if ((data_len > 0) && (payload >= LOG_LEVEL_NONE) && (payload <= LOG_LEVEL_ALL)) {
        sysCfg.syslog_level = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d"), sysCfg.syslog_level);
    }
    else if (!strcmp(type,"LOGHOST")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.syslog_host))) {
        strlcpy(sysCfg.syslog_host, (payload == 1) ? SYS_LOG_HOST : dataBuf, sizeof(sysCfg.syslog_host));
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.syslog_host);
    }
    else if (!strcmp(type,"LOGPORT")) {
      if ((data_len > 0) && (payload > 0) && (payload < 32766)) {
        sysCfg.syslog_port = (payload == 1) ? SYS_LOG_PORT : payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d"), sysCfg.syslog_port);
    }
    else if (!strcmp(type,"SSID")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.sta_ssid))) {
        strlcpy(sysCfg.sta_ssid, (payload == 1) ? STA_SSID : dataBuf, sizeof(sysCfg.sta_ssid));
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.sta_ssid);
    }
    else if (!strcmp(type,"PASSWORD")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.sta_pwd))) {
        strlcpy(sysCfg.sta_pwd, (payload == 1) ? STA_PASS : dataBuf, sizeof(sysCfg.sta_pwd));
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.sta_pwd);
    }
    else if (!grpflg && !strcmp(type,"HOSTNAME")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.hostname))) {
        strlcpy(sysCfg.hostname, (payload == 1) ? WIFI_HOSTNAME : dataBuf, sizeof(sysCfg.hostname));
        if (strstr(sysCfg.hostname,"%"))
          strlcpy(sysCfg.hostname, DEF_WIFI_HOSTNAME, sizeof(sysCfg.hostname));
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.hostname);
    }
    else if (!strcmp(type,"WIFICONFIG") || !strcmp(type,"SMARTCONFIG")) {
      if (data_len > 0) {
        if (payload == 0) payload = sysCfg.sta_config;
        if ((payload > 0) && (payload <= 3)) {
          wificheckflag = payload;
          sysCfg.sta_config = wificheckflag;
          snprintf_P(svalue, sizeof(svalue), PSTR("%s selected"), (payload == WIFI_SMARTCONFIG) ? "Smartconfig" : (payload == WIFI_MANAGER) ? "Wifimanager" : "WPSconfig");
          if (WIFI_State() != WIFI_STATUS) {
            snprintf_P(svalue, sizeof(svalue), PSTR("%s on restart"), svalue);
            restartflag = 2;
          }
        }
      } else {
        snprintf_P(svalue, sizeof(svalue), PSTR("1 to start smartconfig, 2 to start wifimanager, 3 to start wpsconfig. Default is %d"), sysCfg.sta_config);
      }
    }
#ifdef USE_WEBSERVER
    else if (!strcmp(type,"WEBSERVER")) {
      if ((data_len > 0) && (payload >= 0) && (payload <= 2)) {
        sysCfg.webserver = payload;
      }
      if (sysCfg.webserver) {
        snprintf_P(svalue, sizeof(svalue), PSTR("Webserver active for %s on %s with IP address %s"),
          (sysCfg.webserver == 2) ? "ADMIN" : "USER", Hostname, WiFi.localIP().toString().c_str());
      } else {
        snprintf_P(svalue, sizeof(svalue), PSTR("OFF"));
      }
    }
    else if (!strcmp(type,"WEBLOG")) {
      if ((data_len > 0) && (payload >= LOG_LEVEL_NONE) && (payload <= LOG_LEVEL_ALL)) {
        sysCfg.weblog_level = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d"), sysCfg.weblog_level);
    }
#endif  // USE_WEBSERVER
    else if (!strcmp(type,"MQTTHOST")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.mqtt_host))) {
        strlcpy(sysCfg.mqtt_host, (payload == 1) ? MQTT_HOST : dataBuf, sizeof(sysCfg.mqtt_host));
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.mqtt_host);
    }
    else if (!strcmp(type,"MQTTPORT")) {
      if ((data_len > 0) && (payload > 0) && (payload < 32766)) {
        sysCfg.mqtt_port = (payload == 1) ? MQTT_PORT : payload;
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d"), sysCfg.mqtt_port);
    }
    else if (!grpflg && !strcmp(type,"MQTTCLIENT")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.mqtt_client))) {
        strlcpy(sysCfg.mqtt_client, (payload == 1) ? MQTT_CLIENT_ID : dataBuf, sizeof(sysCfg.mqtt_client));
        if (strstr(sysCfg.mqtt_client,"%"))
          strlcpy(sysCfg.mqtt_client, DEF_MQTT_CLIENT_ID, sizeof(sysCfg.mqtt_client));
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.mqtt_client);
    }
    else if (!strcmp(type,"MQTTUSER")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.mqtt_user))) {
        strlcpy(sysCfg.mqtt_user, (payload == 1) ? MQTT_USER : dataBuf, sizeof(sysCfg.mqtt_user));
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.mqtt_user);
    }
    else if (!strcmp(type,"MQTTPASSWORD")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.mqtt_pwd))) {
        strlcpy(sysCfg.mqtt_pwd, (payload == 1) ? MQTT_PASS : dataBuf, sizeof(sysCfg.mqtt_pwd));
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.mqtt_pwd);
    }
    else if (!strcmp(type,"MQTTUNITS")) {
      if ((data_len > 0) && (payload >= 0) && (payload <= 1)) {
        sysCfg.mqtt_units = payload;
      }
      strlcpy(svalue, (sysCfg.mqtt_units) ? "ON" : "OFF", sizeof(svalue));
    }
    else if (!strcmp(type,"TELEPERIOD")) {
      if ((data_len > 0) && (payload >= 0) && (payload < 3601)) {
        sysCfg.tele_period = (payload == 1) ? TELE_PERIOD : payload;
        tele_period = sysCfg.tele_period;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), sysCfg.tele_period, (sysCfg.mqtt_units) ? " Sec" : "");
    }
    else if (!strcmp(type,"GROUPTOPIC")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.mqtt_grptopic))) {
        for(i = 0; i <= data_len; i++)
          if ((dataBuf[i] == '/') || (dataBuf[i] == '+') || (dataBuf[i] == '#')) dataBuf[i] = '_';
        if (!strcmp(dataBuf, MQTTClient)) payload = 1;
        strlcpy(sysCfg.mqtt_grptopic, (payload == 1) ? MQTT_GRPTOPIC : dataBuf, sizeof(sysCfg.mqtt_grptopic));
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.mqtt_grptopic);
    }
    else if (!grpflg && !strcmp(type,"TOPIC")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.mqtt_topic))) {
        for(i = 0; i <= data_len; i++)
          if ((dataBuf[i] == '/') || (dataBuf[i] == '+') || (dataBuf[i] == '#')) dataBuf[i] = '_';
        if (!strcmp(dataBuf, MQTTClient)) payload = 1;
        strlcpy(sysCfg.mqtt_topic, (payload == 1) ? MQTT_TOPIC : dataBuf, sizeof(sysCfg.mqtt_topic));
        restartflag = 2;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.mqtt_topic);
    }
    else if (!grpflg && !strcmp(type,"BUTTONTOPIC")) {
      if ((data_len > 0) && (data_len < sizeof(sysCfg.mqtt_topic2))) {
        for(i = 0; i <= data_len; i++)
          if ((dataBuf[i] == '/') || (dataBuf[i] == '+') || (dataBuf[i] == '#')) dataBuf[i] = '_';
        if (!strcmp(dataBuf, MQTTClient)) payload = 1;
        strlcpy(sysCfg.mqtt_topic2, (payload == 1) ? MQTT_TOPIC : dataBuf, sizeof(sysCfg.mqtt_topic2));
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%s"), sysCfg.mqtt_topic2);
    }
    else if (!strcmp(type,"BUTTONRETAIN")) {
      if ((data_len > 0) && (payload >= 0) && (payload <= 1)) {
        strlcpy(sysCfg.mqtt_topic2, sysCfg.mqtt_topic, sizeof(sysCfg.mqtt_topic2));
        if (!payload) {
          for(i = 1; i <= Maxdevice; i++) {
            
//            snprintf_P(svalue, sizeof(svalue), commands[i]);
//            strtok(svalue, " ");
//            send_button(svalue);

            snprintf_P(stemp1, sizeof(stemp1), commands[1]);
            if (strchr(stemp1, '/')) {    // 1/light 2
              strtok(stemp1, "/");
              str = strtok(NULL, " ");
              snprintf_P(svalue, sizeof(svalue), PSTR("%d/%s"), i, str);
            } else {
              str = strtok(stemp1, " ");  // light 2
              if (Maxdevice == 1) {
                snprintf_P(svalue, sizeof(svalue), PSTR("%s"), str);
              } else {
                snprintf_P(svalue, sizeof(svalue), PSTR("%d/%s"), i, str);
              }
            }
            send_button(svalue);
          }
        }
        sysCfg.mqtt_retain = payload;
      }
      strlcpy(svalue, (sysCfg.mqtt_retain) ? "ON" : "OFF", sizeof(svalue));
    }
    else if (!strcmp(type,"RESTART")) {
      switch (payload) {
      case 1: 
        restartflag = 2;
        snprintf_P(svalue, sizeof(svalue), PSTR("Restarting"));
        break;
      case 99:
        addLog_P(LOG_LEVEL_INFO, PSTR("APP: Restarting"));
        ESP.restart();
        break;
      default:
        snprintf_P(svalue, sizeof(svalue), PSTR("1 to restart"));
      }
    }
    else if (!strcmp(type,"RESET")) {
      switch (payload) {
      case 1: 
        restartflag = 211;
        snprintf_P(svalue, sizeof(svalue), PSTR("Reset and Restarting"));
        break;
      case 2:
        restartflag = 212;
        snprintf_P(svalue, sizeof(svalue), PSTR("Erase, Reset and Restarting"));
        break;
      default:
        snprintf_P(svalue, sizeof(svalue), PSTR("1 to reset"));
      }
    }
    else if (!strcmp(type,"TIMEZONE")) {
      if ((data_len > 0) && (((payload >= -12) && (payload <= 12)) || (payload == 99))) {
        sysCfg.timezone = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), sysCfg.timezone, (sysCfg.mqtt_units) ? " Hr" : "");
    }
    else if ((!strcmp(type,"LIGHT")) || (!strcmp(type,"POWER"))) {
      snprintf_P(sysCfg.mqtt_subtopic, sizeof(sysCfg.mqtt_subtopic), PSTR("%s"), type);
      byte mask = 0x01 << (device -1);
      if ((data_len > 0) && (payload >= 0) && (payload <= 2)) {
        switch (payload) {
        case 0: { // Off
          power &= (0xFF ^ mask);
          break; }
        case 1: // On
          power |= mask;
          break;
        case 2: // Toggle
          power ^= mask;
          break;
        }
        setRelay(power);
      }
      strlcpy(svalue, (power & mask) ? "ON" : "OFF", sizeof(svalue));
    }
    else if (!strcmp(type,"LEDSTATE")) {
      if ((data_len > 0) && (payload >= 0) && (payload <= 1)) {
        sysCfg.ledstate = payload;
      }
      strlcpy(svalue, (sysCfg.ledstate) ? "ON" : "OFF", sizeof(svalue));
    }
#ifdef USE_POWERMONITOR
    else if (!strcmp(type,"POWERLOW")) {
      if ((data_len > 0) && (payload >= 0) && (payload < 3601)) {
        sysCfg.hlw_pmin = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), sysCfg.hlw_pmin, (sysCfg.mqtt_units) ? " W" : "");
    }
    else if (!strcmp(type,"POWERHIGH")) {
      if ((data_len > 0) && (payload >= 0) && (payload < 3601)) {
        sysCfg.hlw_pmax = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), sysCfg.hlw_pmax, (sysCfg.mqtt_units) ? " W" : "");
    }
    else if (!strcmp(type,"VOLTAGELOW")) {
      if ((data_len > 0) && (payload >= 0) && (payload < 501)) {
        sysCfg.hlw_umin = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), sysCfg.hlw_umin, (sysCfg.mqtt_units) ? " V" : "");
    }
    else if (!strcmp(type,"VOLTAGEHIGH")) {
      if ((data_len > 0) && (payload >= 0) && (payload < 501)) {
        sysCfg.hlw_umax = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), sysCfg.hlw_umax, (sysCfg.mqtt_units) ? " V" : "");
    }
    else if (!strcmp(type,"CURRENTLOW")) {
      if ((data_len > 0) && (payload >= 0) && (payload < 16001)) {
        sysCfg.hlw_imin = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), sysCfg.hlw_imin, (sysCfg.mqtt_units) ? " mA" : "");
    }
    else if (!strcmp(type,"CURRENTHIGH")) {
      if ((data_len > 0) && (payload >= 0) && (payload < 16001)) {
        sysCfg.hlw_imax = payload;
      }
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), sysCfg.hlw_imax, (sysCfg.mqtt_units) ? " mA" : "");
    }
#endif  // USE_POWERMONITOR
    else {
      type = NULL;
    }
  }
  if (type == NULL) {
    blinks = 1;
    snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/SYNTAX"), PUB_PREFIX, sysCfg.mqtt_topic);
#ifdef USE_WEBSERVER
    snprintf_P(svalue, sizeof(svalue), PSTR("Status, SaveData, SaveSate, Upgrade, Otaurl, Restart, Reset, WifiConfig, Seriallog, Weblog, Syslog, LogHost, LogPort, SSId, Password%s, Webserver"), (!grpflg) ? ", Hostname" : "");
#else      
    snprintf_P(svalue, sizeof(svalue), PSTR("Status, SaveData, SaveSate, Upgrade, Otaurl, Restart, Reset, WifiConfig, Seriallog, Syslog, LogHost, LogPort, SSId, Password%s"), (!grpflg) ? ", Hostname" : "");
#endif      
    mqtt_publish(stopic, svalue);
    snprintf_P(svalue, sizeof(svalue), PSTR("MqttHost, MqttPort, MqttUser, MqttPassword%s, MqttUnits, GroupTopic, Timezone, Light, Power, Ledstate, TelePeriod"), (!grpflg) ? ", MqttClient, Topic, ButtonTopic, ButtonRetain" : "");
#ifdef USE_POWERMONITOR
    mqtt_publish(stopic, svalue);
    snprintf_P(svalue, sizeof(svalue), PSTR("PowerLow, PowerHigh, VoltageLow, VoltageHigh, CurrentLow, CurrentHigh"));
#endif  
  }
  mqtt_publish(stopic, svalue);
}

/********************************************************************************************/

void send_button(char *cmnd)
{
  char stopic[128], svalue[128], log[LOGSZ];
  char *token;
  byte state, mask, device = 1;

  if (strchr(cmnd, '/')) {   // 1/light 2
    token = strtok(cmnd, "/");
    device = atoi(token);
    if (device > Maxdevice) device = 1;
    token = strtok(NULL, " ");
    if ((!strcmp(token,"light")) || (!strcmp(token,"power"))) strcpy(token, sysCfg.mqtt_subtopic);
    snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/%d/%s"), SUB_PREFIX, sysCfg.mqtt_topic2, device, token);
  } else {
    token = strtok(cmnd, " ");
    if ((!strcmp(token,"light")) || (!strcmp(token,"power"))) strcpy(token, sysCfg.mqtt_subtopic);
    snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/%s"), SUB_PREFIX, sysCfg.mqtt_topic2, token);
  }
  token = strtok(NULL, "");
  if (token != NULL) {
    state = atoi(token);
    if (state == 2) {
      mask = 0x01 << (device -1);
      state = power & mask ^ mask;
    }
    snprintf_P(svalue, sizeof(svalue), PSTR("%s"), (state) ? "ON" : "OFF");
  } else {
    snprintf_P(svalue, sizeof(svalue), "");
  }
  mqtt_publish(stopic, svalue, sysCfg.mqtt_retain);
}

void do_cmnd(char *cmnd)
{
  char stopic[128], svalue[128];
  char *token;

  token = strtok(cmnd, " ");
  snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/%s"), SUB_PREFIX, sysCfg.mqtt_topic, token);
  token = strtok(NULL, "");
  snprintf_P(svalue, sizeof(svalue), PSTR("%s"), (token == NULL) ? "" : token);
  mqttDataCb(stopic, (byte*)svalue, strlen(svalue));
}

void send_power()
{
  char stopic[TOPSZ], svalue[TOPSZ];

  snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/%s"), PUB_PREFIX, sysCfg.mqtt_topic, sysCfg.mqtt_subtopic);
  strlcpy(svalue, (power) ? "ON" : "OFF", sizeof(svalue));
  mqtt_publish(stopic, svalue);
}

void every_second()
{
  char log[LOGSZ], stopic[TOPSZ], svalue[TOPSZ];
  float t, h, ped, pi, pc;
  uint16_t piv, pe, pw, pu;
  byte flag;
    
  if (sysCfg.tele_period) {
    tele_period++;
    if (tele_period == sysCfg.tele_period -1) {
      
#ifdef SEND_TELEMETRY_DS18B20
      dsb_readTempPrep();
#endif  // SEND_TELEMETRY_DS18B20

#ifdef SEND_TELEMETRY_DHT
      dht_readPrep();
#endif  // SEND_TELEMETRY_DHT

    }
    if (tele_period >= sysCfg.tele_period) {
      tele_period = 0;

#ifdef SEND_TELEMETRY_UPTIME
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/UPTIME"), PUB_PREFIX2, sysCfg.mqtt_topic);
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), uptime, (sysCfg.mqtt_units) ? " Hr" : "");
      mqtt_publish(stopic, svalue);
#endif  // SEND_TELEMETRY_UPTIME

#ifdef SEND_TELEMETRY_RSSI
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/RSSI"), PUB_PREFIX2, sysCfg.mqtt_topic);
      snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), WIFI_getRSSIasQuality(WiFi.RSSI()), (sysCfg.mqtt_units) ? " %" : "");
      mqtt_publish(stopic, svalue);
#endif  // SEND_TELEMETRY_RSSI

#ifdef SEND_TELEMETRY_DS18B20
      if (dsb_readTemp(t)) {                 // Check if read failed
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/TEMPERATURE"), PUB_PREFIX2, sysCfg.mqtt_topic);
        dtostrf(t, 1, 1, svalue);
        snprintf_P(svalue, sizeof(svalue), PSTR("%s%s"), svalue, (sysCfg.mqtt_units) ? " C" : "");
        mqtt_publish(stopic, svalue);
      }
#endif  // SEND_TELEMETRY_DS18B20

#ifdef SEND_TELEMETRY_DHT
      if (dht_readTempHum(false, t, h)) {     // Read temperature as Celsius (the default)
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/TEMPERATURE"), PUB_PREFIX2, sysCfg.mqtt_topic);
        dtostrf(t, 1, 1, svalue);
        snprintf_P(svalue, sizeof(svalue), PSTR("%s%s"), svalue, (sysCfg.mqtt_units) ? " C" : "");
        mqtt_publish(stopic, svalue);
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/HUMIDITY"), PUB_PREFIX2, sysCfg.mqtt_topic);
        dtostrf(h, 1, 1, svalue);
        snprintf_P(svalue, sizeof(svalue), PSTR("%s%s"), svalue, (sysCfg.mqtt_units) ? " %" : "");
        mqtt_publish(stopic, svalue);
      }
#endif  // SEND_TELEMETRY_DHT

#ifdef SEND_TELEMETRY_ENERGY
      if (hlw_readEnergy(1, ped, pe, pw, pu, pi, pc)) {       // Read energy, power, voltage and current
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/TODAY_POWER"), PUB_PREFIX2, sysCfg.mqtt_topic);
        dtostrf(ped, 1, 3, svalue);
        snprintf_P(svalue, sizeof(svalue), PSTR("%s%s"), svalue, (sysCfg.mqtt_units) ? " kWh" : "");
        mqtt_publish(stopic, svalue);
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/PERIOD_POWER"), PUB_PREFIX2, sysCfg.mqtt_topic);
        snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), pe, (sysCfg.mqtt_units) ? " Wh" : "");
        mqtt_publish(stopic, svalue);
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/CURRENT_POWER"), PUB_PREFIX2, sysCfg.mqtt_topic);
        snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), pw, (sysCfg.mqtt_units) ? " W" : "");
        mqtt_publish(stopic, svalue);
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/POWER_FACTOR"), PUB_PREFIX2, sysCfg.mqtt_topic);
        dtostrf(pc, 1, 2, svalue);
        snprintf_P(svalue, sizeof(svalue), PSTR("%s"), svalue);
        mqtt_publish(stopic, svalue);
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/VOLTAGE"), PUB_PREFIX2, sysCfg.mqtt_topic);
        snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), pu, (sysCfg.mqtt_units) ? " V" : "");
        mqtt_publish(stopic, svalue);
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/CURRENT"), PUB_PREFIX2, sysCfg.mqtt_topic);
        dtostrf(pi, 1, 3, svalue);
        snprintf_P(svalue, sizeof(svalue), PSTR("%s%s"), svalue, (sysCfg.mqtt_units) ? " A" : "");
        mqtt_publish(stopic, svalue);
      }
#endif  // SEND_TELEMETRY_ENERGY

#ifdef SEND_TELEMETRY_POWER
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/%s"), PUB_PREFIX2, sysCfg.mqtt_topic, sysCfg.mqtt_subtopic);
      strlcpy(svalue, (power) ? "ON" : "OFF", sizeof(svalue));
      mqtt_publish(stopic, svalue);
#endif  // SEND_TELEMETRY_POWER

      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/TIME"), PUB_PREFIX2, sysCfg.mqtt_topic);
      snprintf_P(svalue, sizeof(svalue), PSTR("%04d-%02d-%02dT%02d:%02d:%02d"),
        rtcTime.Year, rtcTime.Month, rtcTime.Day, rtcTime.Hour, rtcTime.Minute, rtcTime.Second);
      mqtt_publish(stopic, svalue);
    }
  }

#ifdef USE_POWERMONITOR
  if (sysCfg.hlw_pmin || sysCfg.hlw_pmax || sysCfg.hlw_umin || sysCfg.hlw_umax || sysCfg.hlw_imin || sysCfg.hlw_imax) {
    hlw_readEnergy(0, ped, pe, pw, pu, pi, pc);
    piv = (uint16_t)(pi * 1000);
    if (hlw_pless(sysCfg.hlw_pmin, pw, flag)) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/POWER_LOW"), PUB_PREFIX2, sysCfg.mqtt_topic);
      strlcpy(svalue, (flag) ? "ON" : "OFF", sizeof(svalue));
      mqtt_publish(stopic, svalue);
    }
    if (hlw_pmore(sysCfg.hlw_pmax, pw, flag)) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/POWER_HIGH"), PUB_PREFIX2, sysCfg.mqtt_topic);
      strlcpy(svalue, (flag) ? "ON" : "OFF", sizeof(svalue));
      mqtt_publish(stopic, svalue);
    }
    if (hlw_uless(sysCfg.hlw_umin, pu, flag)) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/VOLTAGE_LOW"), PUB_PREFIX2, sysCfg.mqtt_topic);
      strlcpy(svalue, (flag) ? "ON" : "OFF", sizeof(svalue));
      mqtt_publish(stopic, svalue);
    }
    if (hlw_umore(sysCfg.hlw_umax, pu, flag)) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/VOLTAGE_HIGH"), PUB_PREFIX2, sysCfg.mqtt_topic);
      strlcpy(svalue, (flag) ? "ON" : "OFF", sizeof(svalue));
      mqtt_publish(stopic, svalue);
    }
    if (hlw_iless(sysCfg.hlw_imin, piv, flag)) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/CURRENT_LOW"), PUB_PREFIX2, sysCfg.mqtt_topic);
      strlcpy(svalue, (flag) ? "ON" : "OFF", sizeof(svalue));
      mqtt_publish(stopic, svalue);
    }
    if (hlw_imore(sysCfg.hlw_imax, piv, flag)) {
      snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/CURRENT_HIGH"), PUB_PREFIX2, sysCfg.mqtt_topic);
      strlcpy(svalue, (flag) ? "ON" : "OFF", sizeof(svalue));
      mqtt_publish(stopic, svalue);
    }
  }
#endif  // USE_POWERMONITOR
  
  if ((rtcTime.Minute == 2) && (rtcTime.Second == 30)) { 
    uptime++;
    snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/UPTIME"), PUB_PREFIX2, sysCfg.mqtt_topic);
    snprintf_P(svalue, sizeof(svalue), PSTR("%d%s"), uptime, (sysCfg.mqtt_units) ? " Hr" : "");
    mqtt_publish(stopic, svalue);
  }
}

void stateloop()
{
  uint8_t button, flag;
  char scmnd[20], log[LOGSZ], stopic[TOPSZ], svalue[TOPSZ];
  
  timerxs = millis() + (1000 / STATES);
  state++;
  if (state == STATES) {             // Every second
    state = 0;
    every_second();
  }

  if ((sysCfg.model >= SONOFF_DUAL) && (sysCfg.model <= CHANNEL_4)) {
    if (ButtonCode) {
      snprintf_P(log, sizeof(log), PSTR("APP: Button code %04X"), ButtonCode);
      addLog(LOG_LEVEL_DEBUG, log);
      button = PRESSED;
/*
      if ((ButtonCode >> 8) == 0x04) {
        if ((ButtonCode & 0x02) != (power & 0x02)) {
          multiwindow = STATES /2;
          multipress = 1;
        }
      } else {
*/
        if (ButtonCode == 0xF500) holdcount = (STATES *4) -1;
/*
      }
*/
      ButtonCode = 0;
    } else {
      button = NOT_PRESSED;
    }
  } else {
    button = digitalRead(KEY_PIN);
  }
  if ((button == PRESSED) && (lastbutton == NOT_PRESSED)) {
    multipress = (multiwindow) ? multipress +1 : 1;
    snprintf_P(log, sizeof(log), PSTR("APP: Multipress %d"), multipress);
    addLog(LOG_LEVEL_DEBUG, log);
    if (WIFI_State()) restartflag = 1;
    blinks = 1;
    multiwindow = STATES /2;         // 1/2 second multi press window
  }
  lastbutton = button;
  if (button == NOT_PRESSED) {
    holdcount = 0;
  } else {
    holdcount++;
    if (holdcount == (STATES *4)) {  // 4 seconds button hold
      snprintf_P(scmnd, sizeof(scmnd), commands[0]);
      multipress = 0;
      do_cmnd(scmnd);
    }
  }
  if (multiwindow) {
    multiwindow--;
  } else {
    if ((!restartflag) && (!holdcount) && (multipress > 0) && (multipress < MAX_BUTTON_COMMANDS)) {
      snprintf_P(scmnd, sizeof(scmnd), commands[multipress]);
      if ((sysCfg.model >= SONOFF_DUAL) && (sysCfg.model <= CHANNEL_4)) {
        flag = ((multipress == 1) || (multipress == 2));
      } else  {
        flag = (multipress == 1);
      }
      if (strcmp(sysCfg.mqtt_topic2,"0") && mqttClient.connected() && flag) {
        send_button(scmnd);          // Execute command via MQTT using ButtonTopic to sync external clients
      } else {
        do_cmnd(scmnd);              // Execute command internally 
      }  
      multipress = 0;
    }
  }

  if (!(state % ((STATES/10)*2))) {
    if (blinks || restartflag || otaflag) {
      if (restartflag || otaflag) {
        blinkstate = 1;   // Stay lit
      } else {
        blinkstate ^= 1;  // Blink
      }
      digitalWrite(LED_PIN, (LED_INVERTED) ? !blinkstate : blinkstate);
      if (!blinkstate) blinks--;
    } else {
      if (sysCfg.ledstate) {
        uint16_t mask = ((0x00FF << Maxdevice) >> 8) & power;
        digitalWrite(LED_PIN, (LED_INVERTED) ? !mask : mask);
      }
    }
  }
  
  switch (state) {
  case (STATES/10)*2:
    if (otaflag) {
      otaflag--;
      if (otaflag <= 0) {
        otaflag = 12;
        ESPhttpUpdate.rebootOnUpdate(false);
        otaok = (ESPhttpUpdate.update(sysCfg.otaUrl) == HTTP_UPDATE_OK);
      }
      if (otaflag == 10) {  // Allow MQTT to reconnect
        otaflag = 0;
        snprintf_P(stopic, sizeof(stopic), PSTR("%s/%s/UPGRADE"), PUB_PREFIX, sysCfg.mqtt_topic);
        if (otaok) {
          snprintf_P(svalue, sizeof(svalue), PSTR("Successful. Restarting"));
          restartflag = 2;
        } else {
          snprintf_P(svalue, sizeof(svalue), PSTR("Failed %s"), ESPhttpUpdate.getLastErrorString().c_str());
        }
        mqtt_publish(stopic, svalue);
      }
    }
    break;
  case (STATES/10)*4:
    if (savedatacounter) {
      savedatacounter--;
      if (savedatacounter <= 0) {
        if (sysCfg.savestate) sysCfg.power = power;
        CFG_Save();
        savedatacounter = sysCfg.savedata;
      }
    }
    if (restartflag) {
      if (restartflag == 211) {
        CFG_Default();
        restartflag = 2;
      }
      if (restartflag == 212) {
        CFG_Erase();
        CFG_Default();
        restartflag = 2;
      }
      if (sysCfg.savestate) sysCfg.power = power;
      CFG_Save();
      restartflag--;
      if (restartflag <= 0) {
        addLog_P(LOG_LEVEL_INFO, PSTR("APP: Restarting"));
        ESP.restart();
      }
    }
    break;
  case (STATES/10)*6:
    WIFI_Check(wificheckflag);
    wificheckflag = WIFI_STATUS;
    break;
  case (STATES/10)*8:
    if ((WiFi.status() == WL_CONNECTED) && (!mqttClient.connected())) {
      if (!mqttcounter) {
        mqtt_reconnect();
      } else {
        mqttcounter--;
      }
    }
    break;
  }
}

void serial()
{
  while (Serial.available())
  {
    yield();
    SerialInByte = Serial.read();

    // Sonoff dual 19200 baud serial interface
    if (Hexcode) {
      Hexcode--;
      if (Hexcode) {
        ButtonCode = (ButtonCode << 8) | SerialInByte;
        SerialInByte = 0;
      } else {
        if (SerialInByte != 0xA1) ButtonCode = 0;  // 0xA1 - End of Sonoff dual button code
      }
    }
    if (SerialInByte == 0xA0) {                    // 0xA0 - Start of Sonoff dual button code
      SerialInByte = 0;
      ButtonCode = 0;
      Hexcode = 3;
    }

    if (SerialInByte > 127) // binary data...
    {
      Serial.flush();
      SerialInByteCounter = 0;
      return;
    }
    if (isprint(SerialInByte))
    {
      if (SerialInByteCounter < INPUT_BUFFER_SIZE) {  // add char to string if it still fits
        serialInBuf[SerialInByteCounter++] = SerialInByte;
      } else {
        SerialInByteCounter = 0;
      }
    }
    if (SerialInByte == '\n')
    {
      serialInBuf[SerialInByteCounter] = 0;  // serial data completed
      addLog(LOG_LEVEL_NONE, serialInBuf);
      SerialInByteCounter = 0;
      do_cmnd(serialInBuf);
      Serial.flush();
    }
  }
}

/********************************************************************************************/

void setup()
{
  char log[LOGSZ];
  byte idx;

  Serial.begin(Baudrate);
  delay(10);
  Serial.println();
  sysCfg.seriallog_level = LOG_LEVEL_INFO;  // Allow specific serial messages until config loaded

  snprintf_P(Version, sizeof(Version), PSTR("%d.%d.%d"), VERSION >> 24 & 0xff, VERSION >> 16 & 0xff, VERSION >> 8 & 0xff);
  if (VERSION & 0x1f) {
    idx = strlen(Version);
    Version[idx] = 96 + (VERSION & 0x1f);
    Version[idx +1] = 0;
  }
  if (!spiffsPresent())
    addLog_P(LOG_LEVEL_ERROR, PSTR("SPIFFS: ERROR - No spiffs present. Please reflash with at least 16K SPIFFS"));
#ifdef USE_SPIFFS
  initSpiffs();
#endif
  CFG_Load();
  CFG_Delta();
  
  if (!sysCfg.model) {
#if MODULE == SONOFF
    pinMode(REL_PIN, INPUT_PULLUP);
    sysCfg.model = digitalRead(REL_PIN) +1;  // SONOFF (1) or SONOFF_DUAL (2)
#else
    sysCfg.model = SONOFF;
#endif
  }
  
  if ((sysCfg.model >= SONOFF_DUAL) && (sysCfg.model <= CHANNEL_4)) {
    Baudrate = 19200;
    Maxdevice = sysCfg.model;
  }
  if (Serial.baudRate() != Baudrate) {
    snprintf_P(log, sizeof(log), PSTR("APP: Need to change baudrate to %d"), Baudrate);
    addLog(LOG_LEVEL_INFO, log);
    delay(100);
    Serial.flush();
    Serial.begin(Baudrate);
    delay(10);
    Serial.println();
  }
  
  sysCfg.bootcount++;
  snprintf_P(log, sizeof(log), PSTR("APP: Bootcount %d"), sysCfg.bootcount);
  addLog(LOG_LEVEL_DEBUG, log);
  savedatacounter = sysCfg.savedata;
  power = sysCfg.power;

  if (strstr(sysCfg.hostname, "%")) strlcpy(sysCfg.hostname, DEF_WIFI_HOSTNAME, sizeof(sysCfg.hostname));
  if (!strcmp(sysCfg.hostname, DEF_WIFI_HOSTNAME)) {
    snprintf_P(Hostname, sizeof(Hostname)-1, sysCfg.hostname, sysCfg.mqtt_topic, ESP.getChipId() & 0x1FFF);
  } else {
    snprintf_P(Hostname, sizeof(Hostname)-1, sysCfg.hostname);
  }
  WIFI_Connect(Hostname);

  if (strstr(sysCfg.mqtt_client, "%")) strlcpy(sysCfg.mqtt_client, DEF_MQTT_CLIENT_ID, sizeof(sysCfg.mqtt_client));
  if (!strcmp(sysCfg.mqtt_client, DEF_MQTT_CLIENT_ID)) {
    snprintf_P(MQTTClient, sizeof(MQTTClient), sysCfg.mqtt_client, ESP.getChipId());
  } else {
    snprintf_P(MQTTClient, sizeof(MQTTClient), sysCfg.mqtt_client);
  }
  mqttClient.setServer(sysCfg.mqtt_host, sysCfg.mqtt_port);
  mqttClient.setCallback(mqttDataCb);

  pinMode(LED_PIN, OUTPUT);
  digitalWrite(LED_PIN, (LED_INVERTED) ? !blinkstate : blinkstate);

  if ((sysCfg.model < SONOFF_DUAL) || (sysCfg.model > CHANNEL_8)) {
    pinMode(KEY_PIN, INPUT_PULLUP);
    pinMode(REL_PIN, OUTPUT);
  }
  if (sysCfg.savestate) setRelay(power);

#ifdef SEND_TELEMETRY_DHT
  dht_init();
#endif

#ifdef USE_POWERMONITOR
  hlw_init();
#endif  // USE_POWERMONITOR

  rtc_init();

  snprintf_P(log, sizeof(log), PSTR("APP: Project %s (Topic %s, Fallback %s, GroupTopic %s) Version %s"),
    PROJECT, sysCfg.mqtt_topic, MQTTClient, sysCfg.mqtt_grptopic, Version);
  addLog(LOG_LEVEL_INFO, log);
}

void loop()
{
#ifndef USE_TICKER
  if (millis() >= timersec) {
    timersec = millis() + 1000;
    rtc_second();
  }
#endif  // USE_TICKER

#ifdef USE_WEBSERVER
  pollDnsWeb();
#endif  // USE_WEBSERVER

  if (millis() >= timerxs) stateloop();

  mqttClient.loop();

  if (Serial.available()) serial();
 
  yield();
}

