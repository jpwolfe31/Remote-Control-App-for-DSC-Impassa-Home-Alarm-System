// Arduino Nano-ESP32 Program  
// Alarm Controller for DSC Impassa system

// Template ID, Device Name and Auth Token are provided by Blynk
// See the Device Info tab, or Template settings
#define BLYNK_TEMPLATE_ID "XXXXXXXXXX"
#define BLYNK_TEMPLATE_NAME "Alarm"
#define BLYNK_AUTH_TOKEN "XXXXXXXXXX"
// not used in agent, just uploaded app
#define BLYNK_FIRMWARE_VERSION "1.0.1"
char auth[] = BLYNK_AUTH_TOKEN;
char version_str[32] = "V.120424";  // month/day/year

// for eeprom emulation in nano ESP32 flash memory
// esp32 EEPROM retains its value between program uploads
// if the EEPROM is written, then there is a written signature at address 0
// update signature when eeprom data structure is changed
#include <EEPROM.h>
#define EEPROM_SIZE 1000  // This is 1k Byte
uint16_t storedAddress = 0;
int signature;
const int WRITTEN_SIGNATURE = 0xabcdabc3;
char ssid[20] = "XXXXXXXXXX";
char pass[20] = "XXXXXXXXXX"; // Set password to "" for open networks.
char k_GMT_str[20] = "-8"; //-8 for California ST and -7 for California DST
int k_GMT_int;
struct memory  // create data structure for easier EEPROM reads/writes
{ // note - all strings - max length 19
  int eeprom_signature;
  char eeprom_ssid[20];
  char eeprom_pass[20]; 
  char eeprom_k_GMT[20];
} flash;
int eepromFirstWriteFlag = 0;

// for wifi and Blynk
#include <SPI.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h>

// for time and clock
#include "time.h"
#include <ESP32Time.h>
ESP32Time rtc(0*3600);
const long  gmtOffset_sec = 3600 * -8; // Pacific Standard Time
const int   daylightOffset_sec = 3600;
const char* ntpServer = "pool.ntp.org";
struct tm timeinfo;

long rssi;
char rssi_str[16];

// for date and time functions
char second_str[8];
int second_int;
char minute_str[8];
int minute_int;
char hour_str[8];
int hour_int;
char days_str[8];
int days_int;
char months_str[8];
int months_int;
char years_str[8];
int years_int;
char date_str[16]; // date string
char time_str[16]; // time string

// for error logging and resets
char error_type_str[32];
char error_type_display_str[32];
// set the reset flag on error and clear the reset flag when restarting
int nano_reset_flag_int = 0;

// for watch dog timer
#include <esp_task_wdt.h>
#define WDT_TIMEOUT 8 // 3, 8 and 16  seconds tested

// for Blynk
WidgetLED ledRTA(V10);
WidgetLED ledStayArmed(V11);
WidgetLED ledAwayArmed(V12);
WidgetLED ledBell(V13);
WidgetLED ledFireBell(V14);
WidgetLED ledWiredSmoke(V15);
WidgetLED ledACSense(V16);

// for Blynk virtual push buttons V4-V7
// used to output to FOB  
int stay_sw4Value = 0;  // stay
int away_sw5Value = 0;  // away
int disarm_sw6Value = 0;  // disarm
int panic_sw7Value = 0;  // panic

// Attach virtual serial terminal to Virtual Pin V3
WidgetTerminal terminal(V3);

// for reading a second input line in the Blynk terminal
int terminal_second_line_flag_int = 0;

// for USB Serial and Blynk commands
char cmd_str[640];
int cmd_length_int = 0;
int cmd_flag_int = 0;
int serial_second_line_flag_int = 0;

// for manual timers
unsigned long currentMillis;
unsigned long previousMillis_Blynk = 0;
unsigned long previousMillis_updateInputs = 0;
unsigned long previousMillis_blinkOnBoardLED = 0;
unsigned long previousMillis_nanoReset = 0; 

// for notifications
char notification_str[256]; 

/*
If using Nano ESP32 - in the IDE Tools menu, enter Pin Numbering and choose By GPIO number (legacy).
Then sketch should always use labels to refer to Arduino pins (e.g if you used the 
number 2, replace this with the symbol D2).
This is a more library-compatible scheme and avoids confusion.  
Also see pin table at end of this sketch.
*/

// for leds
// configure leds
// Nano ESP32 on board RGB led
#define LEDR 46  // Note - Boot0  active low
#define LEDB 45  // Note - not an on-board pinout  active low
#define LEDG 0   // Note - B00t1 active low
// green power led always on with power
// yellow on board led - also called LED builtin -
//   is on D13 or GPIO48 - SPI Serial Clock
int onBoardLEDValue = 0; // for blinking on board led

// FOB outputs
int stayPin = D8; // active high
int awayPin = D9; // active high
int disarmPin = D10; // active high
int panicPin = D11; // active high

// future bell output
int bellOutputPin = D12; // active high

// for inputs and display
// note - display variables used to only update Blynk on change
// RTA
int awayArmedPin = D4; // OC active low - need pullup
int awayArmedValue = 0;
// Armed
int bellPin = D5; // OC active low - need pullup
int bellLEDValue = 0;

int RTAPin = D6; // OC active low - need pullup
int RTALEDValue = 0;
int display_RTALEDValue = 0;
int armedPin = D7; // OC active low - need pullup
int armedValue = 0;
int display_armedValue = 0; 

int stayArmedLEDValue = 0;
int display_stayArmedLEDValue = 0;  
int awayArmedLEDValue = 0;
int display_awayArmedLEDValue = 0;

// Bell
int display_bellLEDValue = 0;
int bellFireLEDValue = 0;
int display_bellFireLEDValue = 0;
int bellInputFlag = 0;
int bv[4] = {0 ,0, 0, 0};
int bv_cnt = 0;
int bv_sum;
int bellFireFlag = 0;
int bellPinValue = 0;
// Wired Smoke
int wiredSDAlarmPin = A6; // active low - need pullup
int wiredSDAlarmLEDValue = 0;
int display_wiredSDAlarmLEDValue = 0;
// AC sense
int ACSensePin = A5; // digital active high
int ACSenseLEDValue = 1;
int display_ACSenseLEDValue = 1;

// for T1 to T4 harwired sensor outputs/inputs
// all digital
int select_A_Pin = A0; // outputs to CD4051 selectors
int select_B_Pin = A1;
int select_C_Pin = A2;
int inputT1T2_Pin = A3;  // CD 4051 outputs to nano - active high, no pullup
int inputT1T2Value = 0;
int inputT3T4_Pin = A4;
int inputT3T4Value = 0;
int T1T2Value[8] = {0, 0, 0, 0, 0, 0, 0, 0};
int T3T4Value[8] = {0, 0, 0, 0, 0, 0, 0, 0};
const char T1T4Zones[16][32] = {  
  // note - alarm zones do not correspond to array values e.g. zone 2 is array 0
  "02  Main Garage Doors",
  "03  Main Garage Storage Door",
  "04  Bedroom 4 Door",
  "05  Bedroom 3 Door",
  "06  Stairs Door",
  "07  Family Room Single Door",
  "08  Family Room Double Doors",
  "09  Studio Garage Doors",
  "11  Studio Kitchen Door",
  "12  Front Door",
  "13  Main Garage Door",
  "14  Laundry Room Delay Door",
  "15  Hall Delay Door",  
  "16  Studio Door",
  "17  Not used",
  "18  Not used"};

void setup()
{   

Serial.begin(38400); // for serial monitor
//while (!Serial) {}; // wait for serial port to connect.

// for eeprom
// In the ESP32, a typical Flash page is 64-Bytes and you need to read-modify-write
// an entire page at a time.  The library saves the data to a buffer with the write() 
// or put() function and it is not actually written to Flash memory until 
// EEPROM.commit() is called. 
// Write eeprom data or if reset, obtain eeprom data
// Check signature at address 0
// If the eeprom is written, then there is a correct written signature.
// Note - unlike Arduino MKR1010 flash memory, this flash memory persists after  
// reprogramming.
EEPROM.begin(EEPROM_SIZE);
EEPROM.get(storedAddress, signature);
// If the EEPROM is written, then there is a orrect written signature
if (signature == WRITTEN_SIGNATURE){
  EEPROMRead();  // print confirmation of data written and update variables
  }
else { // eeprom is not written and needs to be written
  EEPROMWrite();
  eepromFirstWriteFlag = 1;  // report new write through Blynk terminal
  }

// for watch dog timer
esp_task_wdt_init(WDT_TIMEOUT, true); // enable wdt
esp_task_wdt_add(NULL); //add current thread to WDT watch
// when time runs out, processor does a hardware reset

// read eeprom first - for time, wifi inputs

// connect to Blynk and WiFi network  // note - this needs to have the eeprom read first to connect to storred ssid
Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);  // this code works well with the nano, not so much with mkr
esp_task_wdt_reset();
// print out the status on the serial port
Serial.print("SSID: ");
Serial.println(WiFi.SSID());
// print out the WiFi IP address:
IPAddress ip = WiFi.localIP();
Serial.print("IP Address: ");
Serial.println(ip);
// print and display the received signal strength
rssi = WiFi.RSSI();
Serial.print("Signal strength (RSSI):");
Serial.print(rssi);
Serial.println(" dBm");
terminal.println();

// The 'ESP32Time' library is just a wrapper interface for the functions available 
// in 'esp_sntp.h'.  There is no real need for ESP32Time.h other than convenience.
//  As long as WiFi is connected, the ESP32's internal RTC will be periodically 
//  synched to NTP.  The synch interval can be reported and can be changed.  

configTime(k_GMT_int *3600, 0, ntpServer);
//configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
// change to dayligh savings time by changing GMT offset to -7 from -8

//struct tm timeinfo; - defined above 
if(!getLocalTime(&timeinfo)){
  Serial.println("Failed to obtain time");
  }
rtc.setTimeStruct(timeinfo); 
delay(500);

esp_task_wdt_reset();  // refresh watch dog timer
updateDate();
updateTime();
Serial.println();  // for testing
Serial.print(date_str);
Serial.print("  ");
Serial.println(time_str);
Serial.println("Alarm Remote is online.");
Serial.print(ssid);
Serial.print("  ");
rssi = WiFi.RSSI();
Serial.print(rssi);
Serial.println(" dBm");
Serial.println(); // add line feed
Serial.println("Type cmd in Blynk App for list of commands.");
Serial.println(); // add line feed

terminal.clear();
terminal.print(date_str);
terminal.print("  ");
terminal.println(time_str);
terminal.println("Alarm Remote is online.");
terminal.print(ssid);
terminal.print("  ");
terminal.print(rssi);
terminal.println(" dBm");
terminal.println();  // add line feed
esp_task_wdt_reset();  // refresh watch dog timer

Blynk.logEvent("alarm_restarted"); // log event to timeline

// for eeprom if first write
if (eepromFirstWriteFlag == 1){  // report new write through Blynk terminal
  terminal.println("EEPROM did not contain data.  Code defaults written to EEPROM.");
  terminal.println();
  EEPROMRead();  // print confirmation of data written and update variables
  }

terminal.println("Type cmd for list of commands.");
terminal.println(); // add line feed
terminal.flush();

//for Nano LEDS
pinMode(LED_BUILTIN, OUTPUT);
pinMode(LEDR, OUTPUT);
pinMode(LEDG, OUTPUT);
pinMode(LEDB, OUTPUT);
digitalWrite(LED_BUILTIN, LOW);  // turn the LED off by making the voltage LOW
digitalWrite(LEDR, HIGH); // These are active LOW
digitalWrite(LEDG, HIGH); 
digitalWrite(LEDB, HIGH); 

//for other outputs
pinMode(stayPin, OUTPUT);
pinMode(awayPin, OUTPUT);
pinMode(disarmPin, OUTPUT);
pinMode(panicPin, OUTPUT);
pinMode(bellOutputPin, OUTPUT);
digitalWrite(stayPin, LOW);
digitalWrite(awayPin, LOW);
digitalWrite(disarmPin, LOW);
digitalWrite(panicPin, LOW);
digitalWrite(bellOutputPin, LOW);
pinMode(select_A_Pin, OUTPUT);
pinMode(select_B_Pin, OUTPUT);
pinMode(select_C_Pin, OUTPUT);
digitalWrite(select_A_Pin, LOW);
digitalWrite(select_B_Pin, LOW);
digitalWrite(select_C_Pin, LOW);
pinMode(inputT1T2_Pin, INPUT); // digital active high, no pullup
pinMode(inputT3T4_Pin, INPUT); // digital active high, no pullup

// for inputs
pinMode(awayArmedPin, INPUT_PULLUP); // OC active low - need pullup
pinMode(bellPin, INPUT_PULLUP); // OC active low - need pullup
pinMode(RTAPin, INPUT_PULLUP); // OC active low - need pullup
pinMode(armedPin, INPUT_PULLUP); // OC active low - need pullup
pinMode(wiredSDAlarmPin, INPUT_PULLUP); // active low - need pullup
pinMode(ACSensePin, INPUT); // digital active high, no pullup

// turn off all Blynk leds
ledRTA.off();
ledStayArmed.off();
ledAwayArmed.off();
ledBell.off();
ledFireBell.off();
ledWiredSmoke.off();
ledACSense.off();

// end setup
}

void loop()
{
esp_task_wdt_reset();  // refresh watch dog timer
currentMillis = millis();
// check inputs and update outputs every 0.25 second
if (currentMillis - previousMillis_updateInputs >= 250) { 
  previousMillis_updateInputs = currentMillis;  // Remember the time
  updateBlynkSwitches();
  updateInputs(); // RTA, Armed, awayArmed, bell, AC Sense, Wired SD Alarm
  }
// Check for alarm bell and Blynk terminal command every 0.5 second
if (currentMillis - previousMillis_Blynk >= 500) { 
  previousMillis_Blynk = currentMillis;  // Remember the time
  updateBellState();
  Blynk.run();
  if (strcmp(cmd_str, "cleared") != 0) {
    menu();
    strcpy(cmd_str, "cleared"); // after menu() runs, clear com_str to avoid repeated commands
    }
  }
// blink on board led every 1 second
if(currentMillis - previousMillis_blinkOnBoardLED >= 1000) {
  previousMillis_blinkOnBoardLED = currentMillis;  // Remember the time
  blinkOnBoardLED();
  }
// Error routine after 5 seconds
if((nano_reset_flag_int == 1) && (currentMillis - previousMillis_nanoReset >= 5000)) {  
  nanoReset();
  }
}  // end of loop

void printLocalTime() // prints local time to Blynk terminal
{
// struct tm timeinfo; - defined globally above  
//getLocalTime(&timeinfo);
//terminal.print("ntp ");
//terminal.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");  // see strftime for formating options
// rtc loaded above in setup ??
// terminal.print("nano rtc ");
terminal.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));   // (String) returns time with specified format 
terminal.flush();
Blynk.run();
return;
}

void updateBlynkSwitches(){ // use for Blynk switches
  if (stay_sw4Value) {
    strcpy(notification_str, "Stay pressed.");
    terminal_output();
    // turn on alarm in stay mode
    digitalWrite(stayPin, HIGH);  // output to FOB
    delay2s(); // Blynk continues to run
    digitalWrite(stayPin, LOW);
    stayArmedLEDValue = 1;
    awayArmedLEDValue = 0;
    }
if (away_sw5Value) {
    strcpy(notification_str, "Away pressed.");
    terminal_output();
    // turn on alarm in away mode
    digitalWrite(awayPin, HIGH);  // output to FOB
    delay2s(); // Blynk continues to run
    digitalWrite(awayPin, LOW);
    awayArmedLEDValue = 1;
    stayArmedLEDValue = 0;
    }
if (disarm_sw6Value) {
    strcpy(notification_str, "Disarm pressed.");
    terminal_output();
    // turn on alarm in away mode
    digitalWrite(disarmPin, HIGH);  // output to FOB
    delay2s(); // Blynk continues to run
    digitalWrite(disarmPin, LOW);
    }
if (panic_sw7Value) {
    strcpy(notification_str, "Panic pressed.");
    terminal_output();
    // turn on alarm in panic mode
    digitalWrite(panicPin, HIGH);  // output to FOB
    delay4s(); // longer press needed. Blynk continues to run
    digitalWrite(panicPin, LOW);
    }
terminal.flush();
Blynk.run();
}

void updateInputs()  
// RTA, Armed, Bell, Wired SD Alarm, AC Sense, 
{// RTA
RTALEDValue = !digitalRead(RTAPin);  // OC active low - input has pullup
if (RTALEDValue == 1) {
  if (RTALEDValue != display_RTALEDValue){ // prevents repeated Blynk writes
    display_RTALEDValue = RTALEDValue;
    ledRTA.on();
    }
  }
if (RTALEDValue == 0) {
  if (RTALEDValue != display_RTALEDValue){ // prevents repeated Blynk writes
    display_RTALEDValue = RTALEDValue;
    ledRTA.off();
    }
  }
// Armed
armedValue = !digitalRead(armedPin); // OC active low - input has pullup - this is the LED
awayArmedValue  = !digitalRead(awayArmedPin); // OC active low - input has pullup - thisis the pgm output
// if armed, one led on
if (armedValue == 1){
  if (awayArmedValue == 0){ //stay armed
    stayArmedLEDValue = 1;
    if (display_stayArmedLEDValue != 1){
      display_stayArmedLEDValue = 1;
      ledStayArmed.on();
      display_awayArmedLEDValue = 0;  // addresses arming to stay from away
      ledAwayArmed.off();
      // send notification
      strcpy(notification_str, "Alarm armed in stay mode.");
      Blynk.logEvent("alarm_armed", String(notification_str)); 
      terminal_output();
      }
    }
  if (awayArmedValue == 1){  // away armed
    if (display_awayArmedLEDValue != 1){
      display_awayArmedLEDValue = 1;
      ledAwayArmed.on();
      display_stayArmedLEDValue = 0;
      ledStayArmed.off(); // addresses arming to away from stay
      // send notification
      strcpy(notification_str, "Alarm armed in away mode.");
      Blynk.logEvent("alarm_armed", String(notification_str)); 
      terminal_output();
      }
    }
  } 
// if not armed, both leds off
// Note - entering some commands while armed causes the led to turn off for up to 12 seconds 
// (e.g. *1 for interior arming).  The system remains armed in this event, so a disarmed 
// notification should not be sent.
// Possible resolutions
// limit this exception to stay mode
// change program output to armed only not away armed.
 
if (armedValue == 0){ // not armed - this is the LED
  if (display_stayArmedLEDValue != 0){
    display_stayArmedLEDValue = 0;
    ledStayArmed.off();
    // send notification
    strcpy(notification_str, "Alarm disarmed.");
    Blynk.logEvent("alarm_disarmed", String(notification_str)); 
    terminal_output();
    }
  if (display_awayArmedLEDValue != 0){
    display_awayArmedLEDValue = 0;
    ledAwayArmed.off();
    // send notification
    strcpy(notification_str, "Alarm disarmed.");
    Blynk.logEvent("alarm_disarmed", String(notification_str)); 
    terminal_output();
    }
  }
// Bell
if (bellInputFlag == 0){  // no alarm bell
  bellLEDValue = 0;
  bellFireLEDValue = 0;
   if (bellLEDValue != display_bellLEDValue){
    display_bellLEDValue = 0;
    ledBell.off();
    }
  if (bellFireLEDValue != display_bellFireLEDValue){
    display_bellFireLEDValue = 0;
    ledFireBell.off();
    }
  }
if (bellInputFlag == 1){   // standard alarm bell 
  bellLEDValue = 1;
  if (bellLEDValue != display_bellLEDValue){ // prevents repeated Blynk writes
    display_bellLEDValue = bellLEDValue;
    ledBell.on();
    // send notification
    strcpy(notification_str, "Standard Alarm Bell detected.");
    Blynk.logEvent("alarm_bell", String(notification_str)); 
    display_bellFireLEDValue = 0;  // turn off fire bell
    ledFireBell.off();
    terminal_output();
    }
  }
if (bellInputFlag == 2){    // fire alarm bell
  bellFireLEDValue = 1;
  if (bellFireLEDValue != display_bellFireLEDValue){
    display_bellFireLEDValue = bellFireLEDValue;
    ledFireBell.on();
    // send notification
    strcpy(notification_str, "Alarm Fire Bell detected.");
    // alarm fire bell includes std notification to jpwolfe@gmail.com, and in app.
    Blynk.logEvent("alarm_fire_bell", String(notification_str)); 
    // see alarm_bell above
    Blynk.logEvent("alarm_bell", String(notification_str)); 
    display_bellLEDValue = 0;   // turn off stdandard bell
    ledBell.off();
    terminal_output();
    }
  }
// Wired Smoke
wiredSDAlarmLEDValue = !digitalRead(wiredSDAlarmPin); // OC active highlow - need pullup
if (wiredSDAlarmLEDValue == 1) {
  if (wiredSDAlarmLEDValue != display_wiredSDAlarmLEDValue){ // prevents repeated Blynk writes
    display_wiredSDAlarmLEDValue = wiredSDAlarmLEDValue;
    ledWiredSmoke.on();
    strcpy(notification_str, "Wired Smoke Bell detected.");
    Blynk.logEvent("wired_smoke_bell", String(notification_str)); 
    // see alarm_bell above
    Blynk.logEvent("alarm_bell", String(notification_str));
    terminal_output();
    }
  }
if (wiredSDAlarmLEDValue == 0) {
  if (wiredSDAlarmLEDValue != display_wiredSDAlarmLEDValue){ // prevents repeated Blynk writes
    display_wiredSDAlarmLEDValue = wiredSDAlarmLEDValue;
    ledWiredSmoke.off();
    }
  }
// AC Sense
ACSenseLEDValue = digitalRead(ACSensePin); // digital active high
if (ACSenseLEDValue == 0) { // led turns on when ACSense is low - AC not detected 
  if (ACSenseLEDValue != display_ACSenseLEDValue){ // prevents repeated Blynk writes
    display_ACSenseLEDValue = ACSenseLEDValue;
    ledACSense.on();
    // send notification
    strcpy(notification_str, "AC not detected.");
    // ac_sense_bell includes std notification to jpwolfe@gmail.com, and in app.  
    // may add alarm_bell notification and automation as well? 
    Blynk.logEvent("ac_sense_bell", String(notification_str)); 
    // see alarm_bell above
    Blynk.logEvent("alarm_bell", String(notification_str)); 
    terminal_output();
    }
  }  
if (ACSenseLEDValue == 1) {  // led turns off when ACSense is high - AC detected 
  if (ACSenseLEDValue != display_ACSenseLEDValue){ // prevents repeated Blynk writes
    display_ACSenseLEDValue = ACSenseLEDValue;
    ledACSense.off();
    }
  }
// for T1 to T4 harwired sensor outputs/inputs
updateT1T4();
}

void terminal_output()
{
updateTime();
terminal.print(time_str);
terminal.print("  ");
terminal.println(notification_str);
}

void updateT1T4()
{
// read each 508x 8 times - once for each zone
for (int p = 0; p < 8; p++){
  inputT1T2Value = 0; // rest input values
  inputT3T4Value = 0;
  int A = bitRead(p,0);
  int B = bitRead(p,1);
  int C = bitRead(p,2);
  digitalWrite(select_A_Pin, A);
  digitalWrite(select_B_Pin, B);
  digitalWrite(select_C_Pin, C);
  delay(1); // allow for settling
  // enable pin on 4051 not used - tied to ground
  // input values T1-T4
  for (int r = 0; r < 20; r++){ // digital read of 50hz pulse from RE508x when open
    // 2ms on, 18 ms off - 20ms period
    inputT1T2Value = max(inputT1T2Value, int(digitalRead(inputT1T2_Pin)));
    inputT3T4Value = max(inputT3T4Value, int(digitalRead(inputT3T4_Pin)));
    delay(1); // 20 times 1 - check the entire period
    }
  // save values to arrays
  T1T2Value[p] = inputT1T2Value;
  T3T4Value[p] = inputT3T4Value;
  }
}

void updateBellState() // runs every 500ms
{
// If reading from sounder wire, std bell is 3.1kHz pulsed 12v signal on Bell input (320us period).
// file bell is same but with +12v steady input every other second
// If reading from alarm output on panel pgm 0 or 1 output, standard bell is constant oc low output and 
// fire bell is 1s on 1s off oc output (2 second period) if set that way in panel.
// This is current setup.
// Call every 500ms
// Need six fire bell reads in a row to avoid false bell during bell transitions on and off
bv[bv_cnt] = !digitalRead(bellPin); // note inversion as active low
bv_sum = bv[0] + bv[1] + bv[2] + bv[3];
if (bv_sum == 0) {
  bellInputFlag = 0;  // no bell
  bellFireFlag = 0;  // reset bellFireFlag
  }
if (bv_sum == 4){
  bellInputFlag = 1;  // standard bell
  bellFireFlag = 0;  // reset bellFireFlag
  }
if (bv_sum > 0 && bv_sum < 4 && bellFireFlag == 5) bellInputFlag = 2; // fire bell
if (bv_sum > 0 && bv_sum < 4 && bellFireFlag == 4) bellFireFlag = 5; // possible fire bell
if (bv_sum > 0 && bv_sum < 4 && bellFireFlag == 3) bellFireFlag = 4; // possible fire bell
if (bv_sum > 0 && bv_sum < 4 && bellFireFlag == 2) bellFireFlag = 3; // possible fire bell
if (bv_sum > 0 && bv_sum < 4 && bellFireFlag == 1) bellFireFlag = 2; // possible fire bell
if (bv_sum > 0 && bv_sum < 4 && bellFireFlag == 0) bellFireFlag = 1; // possible fire bell
bv_cnt++;
if (bv_cnt == 4) bv_cnt = 0;
return;
}

void nanoError()
{
  // do not log new errors if one has been reported 
  //   and now in prcess of logging and resetting
  if (nano_reset_flag_int == 1) {return;}
  // general case errors
  strcpy(error_type_display_str, error_type_str);
  strcat(error_type_display_str, " Error");
  // special case error
  if (strcmp(error_type_str, "BT") == 0) {
    strcpy(error_type_display_str, "BT restart");
    }  
  if (strcmp(error_type_str, "ST") == 0) {
    strcpy(error_type_display_str, "ST restart");
    }  
  // set flag for error reporting and shutdown
  nano_reset_flag_int = 1;
}

// processor software reset 
void nanoReset()  // runs at end of 5 second nanoReset timer
{
  // send notification
  strcpy(notification_str, error_type_display_str);
  Blynk.logEvent("alarm_restarted", String(notification_str)); 
  // above log restart in timeline nnnnn
  Serial.println(notification_str);
  ESP.restart (); 
}

void updateRTC()
{
configTime(k_GMT_int *3600, 0, ntpServer);
//configTime(gmtOffset_sec, daylightOffset_sec, ntpServer); 
// change to dayligh savings time by changing GMT offset to -7 from -8
//struct tm timeinfo; - defined above 
if(!getLocalTime(&timeinfo)){
  terminal.println("Failed to obtain time");
  }
rtc.setTimeStruct(timeinfo); 
// printLocalTime();
return;
}

void updateDate()
{
  // get date data
  years_int = rtc.getYear(); // 4 digits 2021
  //sprintf(years_str, "%4d", years_int); 
  sprintf(years_str, "%2d", years_int);  // 2 digits 21 ?
  months_int = rtc.getMonth();  // returns 0-11
  months_int++;
  sprintf(months_str, "%02d", months_int);
  // above converts to 2 character decimal base - pads leading 0s by adding the 0
  days_int = rtc.getDay(); // returns 1-31
  sprintf(days_str, "%02d", days_int);
  strcpy(date_str, years_str);
  strcat(date_str, "-");
  strcat(date_str, months_str);
  strcat(date_str, "-");
  strcat(date_str, days_str);
}  

void updateTime()
{
  //get time data
  hour_int = rtc.getHour(true); // true is 24 hour time 0-23
  sprintf(hour_str, "%02d", hour_int); 
  // above converts to 2 character decimal base - pads leading 0s by adding the 0
  minute_int = rtc.getMinute(); // 0-59
  sprintf(minute_str, "%02d", minute_int); 
  second_int = rtc.getSecond();
  sprintf(second_str, "%02d", second_int); 
  strcpy(time_str, hour_str);
  strcat(time_str, ":");
  strcat(time_str, minute_str);
  strcat(time_str, ":");
  strcat(time_str, second_str);
}  

void EEPROMWrite()
{
// note - function changing these variables must also update
//   the working variables - e.g. k_GMT_int and k_GMT_str
// store signature first
flash.eeprom_signature = WRITTEN_SIGNATURE;
strcpy(flash.eeprom_ssid, ssid);
strcpy(flash.eeprom_pass, pass);
strcpy(flash.eeprom_k_GMT, k_GMT_str);
EEPROM.put(storedAddress, flash);
EEPROM.commit();
EEPROMRead();
}

// read eeprom
void EEPROMRead()
{
EEPROM.get(storedAddress, signature);
// If the EEPROM is written, then there is a written signature
if (signature == WRITTEN_SIGNATURE){
  EEPROM.get(storedAddress, flash);
  // Print a confirmation of the EEPROM data
  terminal.println("EEPROM data:  ");
  terminal.print("ssid ");
  terminal.println(flash.eeprom_ssid); 
  terminal.print("pass: ");  
  terminal.println(flash.eeprom_pass);  
  terminal.print("k_GMT: ");
  terminal.println(flash.eeprom_k_GMT); 
  terminal.println();
  // convert eeprom data to strings and numbers used in the program
  strcpy(ssid, flash.eeprom_ssid);
  strcpy(pass, flash.eeprom_pass);
  strcpy(k_GMT_str, flash.eeprom_k_GMT);
  k_GMT_int = atoi(flash.eeprom_k_GMT);
  }
else { // eeprom is not written and needs to be written
  terminal.println("EEPROM does not contain data.");
  }
terminal.flush();
Blynk.run();
return;
}  

// blinks Nano ESP32 on board yellow LED
void blinkOnBoardLED() 
{
if (onBoardLEDValue == 1) {
  digitalWrite(LED_BUILTIN, LOW);    // turn the LED off by making the voltage LOW
  onBoardLEDValue = 0;
  }
else {
  digitalWrite(LED_BUILTIN, HIGH);   // turn the LED on (HIGH is the voltage level)
  onBoardLEDValue = 1;
  }
}

// delays that maintain on board led blinking  
//   and WDT clearing
void delay25ms(){ 
  currentMillis = millis();
  if(currentMillis - previousMillis_blinkOnBoardLED >= 1000) {
    previousMillis_blinkOnBoardLED = currentMillis;  // Remember the time
    blinkOnBoardLED();
    } 
  delay(25);
  esp_task_wdt_reset();
  }

void delay30ms(){delay(5); delay25ms();}
void delay50ms(){delay(25); delay25ms();}
void delay75ms(){delay(50); delay25ms();}
void delay100ms(){delay(75); delay25ms();}
void delay150ms(){delay(100); delay50ms();}
void delay200ms(){delay(175); delay25ms();}
void delay250ms(){delay(225); delay25ms();}
void delay300ms(){delay(275); delay25ms();}
void delay350ms(){delay(325); delay25ms();}
void delay400ms(){delay(375); delay25ms();}
void delay450ms(){delay(425); delay25ms();}
void delay500ms(){delay250ms(); delay250ms();}
void delay600ms(){delay300ms(); delay300ms();}
void delay700ms(){delay350ms(); delay350ms();}
void delay750ms(){delay500ms(); delay250ms();}
void delay800ms(){delay400ms(); delay400ms();}
void delay900ms(){delay450ms(); delay450ms();}
void delay1s(){delay500ms(); delay500ms();}
void delay2s(){delay1s(); delay1s();}
void delay3s(){delay2s(); delay1s();}
void delay4s(){delay2s(); delay2s();}
void delay5s(){delay3s(); delay2s();}
void delay6s(){delay4s(); delay2s();}
void delay7s(){delay4s(); delay3s();}
void delay8s(){delay4s(); delay4s();}
void delay9s(){delay5s(); delay4s();}
void delay10s(){delay5s(); delay5s();}

/*
In the IDE Tools menu, enter Pin Numbering and choose By GPIO number (legacy);
Make sure the sketch always uses labels to refer to pins. If you used the number 2, 
replace this with the symbol D2 everywhere.  This will switch to a more library-compatible 
scheme and avoid the above confusion.  Do not include GPIO in the number. 
See pin table below.

Nano	ESP32
D0	GPIO44
D1	GPIO43
D2	GPIO5  reported that this esp32 pin 5 may conflict with use of eeprom - have not seen this to date.
D3	GPIO6
D4	GPIO7
D5	GPIO8
D6	GPIO9  reported do not want to use gpi0 6-11 as also used for integrated flash?? eeprom?? - have not seen this to date. 
D7	GPIO10
D8	GPIO17
D9	GPIO18
D10	GPIO21
D11	GPIO38
D12	GPIO47
D13	GPIO48  also built in led and SPI clock and used with Blynk for link indication
A0	GPIO1
A1	GPIO2
A2	GPIO3
A3	GPIO4
A4	GPIO11
A5	GPIO12
A6	GPIO13
A7	GPIO14
BOOT0	GPIO46  also Red on rgb led 
BOOT1	GPIO0   also Green on rgb led

GPIO 45 is not on board pinout but is Blue on rgb 
Note - some early boards (not mine) have different rgb colors

w5500 Ethernet module uses 5, 16-19 and 23, so could be a problem with this board as 16 and 19 and 23 have no outputs.
use uno and ethernet board instead?

*/

