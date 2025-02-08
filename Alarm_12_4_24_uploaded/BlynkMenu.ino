// For Blynk terminal commands
BLYNK_WRITE(V3) // for reading terminal commands written from Blynk
{
strcpy(cmd_str, param.asStr());  // copy Blynk app terminal input to cmd_str  
// terminal.println(cmd_str); // for testing
// terminal.flush();
// Blynk.run();
}

void menu() // main waterfall command menu
{ 
  // return if command already executed
  if (strcmp(cmd_str, "cleared") == 0) return;
  // get command length used for decoding below
  cmd_length_int = strlen(cmd_str);  // note - does not include '\0'.
  
  // page two commands start first
  // for second line Wifi ssid
    if ((serial_second_line_flag_int == 2) && (strcmp(cmd_str, "") != 0)) { // returns 0 if equal
     if (strlen(cmd_str) > 15) { // note- null character ‘\0’ not counted
       terminal.println ("Invalid entry");
       terminal.println();  // add line feed
       terminal.flush();
       Blynk.run();
       serial_second_line_flag_int = 0; // reset file read flag 
       return;
       }
     strcpy(ssid, cmd_str);
     terminal.print("Wifi SSID changed to: ");
     terminal.println(ssid); 
     terminal.println();  // add line feed
     terminal.flush();
     Blynk.run();
     EEPROMWrite(); 
     serial_second_line_flag_int = 0; // reset file read flag 
     return;
     }
  // for second line Wifi password
    if ((serial_second_line_flag_int == 3) && (strcmp(cmd_str, "") != 0)) { // returns 0 if equal
     if (strlen(cmd_str) > 15) { // note- null character ‘\0’ not counted
       terminal.println ("Invalid entry");
       terminal.println();  // add line feed
       terminal.flush();
       Blynk.run();
       serial_second_line_flag_int = 0; // reset file read flag 
       return;
       }
     strcpy(pass, cmd_str);
     terminal.print("Wifi password changed to: ");
     terminal.println(pass); 
     terminal.println();  // add line feed
     terminal.flush();
     Blynk.run();
     EEPROMWrite();
     serial_second_line_flag_int = 0; // reset file read flag 
     return;
     }
  // for second line GMT offset
    if ((serial_second_line_flag_int == 4) && (strcmp(cmd_str, "") != 0)) { // returns 0 if equal
      int l = atoi(cmd_str);
      if ((l > 12) || (l < -12)) {
        terminal.println ("Invalid entry");
        terminal.println();  // add line feed
        terminal.flush();
        Blynk.run();
        serial_second_line_flag_int = 0; // reset file read flag 
        return;
        }
      strcpy(k_GMT_str, cmd_str);
      k_GMT_int = atoi(k_GMT_str);
      EEPROMWrite();
      serial_second_line_flag_int = 0; // reset file read flag
      updateRTC();
      // read new time
      updateTime();
      //terminal.print("GMT offset changed to: ");
      //terminal.println(k_GMT_str); 
      //terminal.println("Clock updated"); 
      terminal.println(); // add line feed
      terminal.flush();
      Blynk.run();
      return;
      }
  esp_task_wdt_reset();  // refresh watch dog timer
  // end page two commands

  if (strcmp(cmd_str, "cmd") == 0) { // list commands
    terminal.println("z      - report zone status");
    terminal.println("zo     - report open zones");
    terminal.println("rst    - reset controller");
    terminal.println("sig    - report WiFi signal strength"); 
    terminal.println("v      - report version of code");
    terminal.println("c      - Blynk terminal clear");
    terminal.println("clr    - local terminal clear");
    terminal.println("cmd    - list available commands");
    terminal.println("cmdm   - list more commands");
    terminal.println(); // add line feed
    terminal.flush();
    Blynk.run();
    return;
    } // note 50 max terminal width
 if (strcmp(cmd_str, "cmdm") == 0) { // list more commands
    terminal.println("tr     - report time once");
    terminal.println("ts     - report/syncs rtc/WiFi times");
    terminal.println("st     - report op status");
    terminal.println("tled   - test nano leds");
    terminal.println("tbled  - test Blynk virtual leds");
    terminal.println("tinp   - test inputs");
    terminal.println("tbs    - test bell states");
    terminal.println("wdt    - test watchdog timer");
    terminal.println("cssid  - change Wifi SSID (eeprom)");  // second page flag 2
    terminal.println("cpass  - change Wifi password (eeprom)");   // second page flag 3
    terminal.println("cgmto  - change GMT offset (eeprom)");  // second page flag 4
    terminal.println(); // add line feed
    terminal.flush();
    Blynk.run();
    return;
    }
  
  if (strcmp(cmd_str, "z") == 0){
    // report zone status
    terminal.println("Zone status: O-open or C-closed");
    //terminal.println();
    // T1T2
    for (int z = 0; z < 8; z++){
      // zone status
      if (T1T2Value[z]) terminal.print("O - ");
      else terminal.print("C - ");    
      // zone name
      terminal.println(T1T4Zones[z]); 
      }
    // T3T4
    for (int z = 0; z < 8; z++){
      // zone status
      if (T3T4Value[z]) terminal.print("O - ");
      else terminal.print("C - ");    
      // zone name
      terminal.println(T1T4Zones[z+8]); 
      }
    terminal.println();
    terminal.flush();
    Blynk.run();
    return;
    }
  if (strcmp(cmd_str, "zo") == 0){
    int zoneFlag = 0;
    // report open zones
    terminal.println("Open Zones:");
    //terminal.println();
    // T1T2
    for (int z = 0; z < 8; z++){
      // zone status
      if (T1T2Value[z]){
        terminal.print("O - ");
        // zone name
        terminal.println(T1T4Zones[z]); 
        zoneFlag = 1;
        } 
      }
    // T3T4
    for (int z = 0; z < 8; z++){
      // zone status
      if (T3T4Value[z]){
        terminal.print("O - ");
        // zone name
        terminal.println(T1T4Zones[z+8]); 
        zoneFlag = 1;
        }
      }   
    if (zoneFlag == 0) terminal.println("No open zones");
    terminal.println();
    terminal.flush();
    Blynk.run();
    return;
    }

  if (strcmp(cmd_str, "tled") == 0){
    // tled - test leds
    terminal.println();
    terminal.println("Testing Nano ESP32 built-in leds for 12 seconds");
    terminal.println();
    terminal.flush();
    Blynk.run();
    for (int iled = 0; iled <4; iled++){
      digitalWrite(LED_BUILTIN, HIGH);  // Note also used by Blynk to show link status active high
      digitalWrite(LEDR, LOW);  // active low red 
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, HIGH);
      delay1s();
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, LOW); // active low green
      digitalWrite(LEDB, HIGH);
      delay1s();
      digitalWrite(LED_BUILTIN, HIGH);
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, LOW); // active low blue
      delay1s();
      digitalWrite(LEDR, HIGH);
      digitalWrite(LEDG, HIGH);
      digitalWrite(LEDB, HIGH); // off
      digitalWrite(LED_BUILTIN, LOW);
      delay1s();
      }
    delay(50);
    return;
    }
  if (strcmp(cmd_str, "tbled") == 0){
    // tbled - test blynk virtual leds
    terminal.println();
    terminal.println("Testing Blynk virtural leds for 12 seconds");
    terminal.println();
    terminal.flush();
    Blynk.run();
    for (int iled = 0; iled <4; iled++){
      ledRTA.on();
      delay1s();
      ledRTA.off();
      ledStayArmed.on();
      delay1s();
      ledStayArmed.off();
      ledAwayArmed.on();
      delay1s();
      ledAwayArmed.off();
      ledBell.on();
      delay1s();
      ledBell.off();
      ledFireBell.on();
      delay1s();
      ledFireBell.off();
      ledWiredSmoke.on();
      delay1s();
      ledWiredSmoke.off();
      ledACSense.on();
      delay1s();
      ledACSense.off();
      }
    delay(50);
    return;
    }
  if (strcmp(cmd_str, "tinp") == 0){
    // tinp - test inputs
    updateInputs();    
    terminal.println();
    terminal.print("RTALEDValue = ");
    terminal.println(RTALEDValue);
    terminal.print("armedValue = ");
    terminal.println(armedValue);
    terminal.print("awayArmedValue = ");
    terminal.println(awayArmedValue);
    terminal.print("bellPinValue = ");
    bellPinValue = !digitalRead(bellPin);  // oc active low
    terminal.println(bellPinValue);
    terminal.print("ACSenseLEDValue = ");
    terminal.println(ACSenseLEDValue);
    terminal.print("wiredSDAlarmLEDValue = ");
    terminal.println(wiredSDAlarmLEDValue);
    terminal.print("T1 T2 T3 T4 values = ");
    terminal.print(T1T2Value[0]);
    terminal.print(T1T2Value[1]);
    terminal.print(T1T2Value[2]);
    terminal.print(T1T2Value[3]);
    terminal.print("  ");
    terminal.print(T1T2Value[4]);
    terminal.print(T1T2Value[5]);
    terminal.print(T1T2Value[6]);
    terminal.print(T1T2Value[7]);
    terminal.print("  ");
    terminal.print(T3T4Value[0]);
    terminal.print(T3T4Value[1]);
    terminal.print(T3T4Value[2]);
    terminal.print(T3T4Value[3]);
    terminal.print("  ");
    terminal.print(T3T4Value[4]);
    terminal.print(T3T4Value[5]);
    terminal.print(T3T4Value[6]);
    terminal.print(T3T4Value[7]);
    terminal.println();
    terminal.flush();
    Blynk.run();
    bellLEDValue = 0; // reset values read
    inputT1T2Value = 0;
    inputT3T4Value = 0;
    }
  if (strcmp(cmd_str, "tbs") == 0){
    // tbs - test bell states
    // std bell is 3.1kHz pulsed 12v signal on Bell input (320us period).
    // file bell is same but with +12v steady input every other second
    // check bell voltage over 2 seconds
    terminal.println("Reading bell input states evary 500ms over 20 seconds");
    for (int h = 0; h < 40; h++){
      updateBellState();
      terminal.print(bv[0]);
      terminal.print(bv[1]);
      terminal.print(bv[2]);
      terminal.println(bv[3]);
      delay500ms();
      }
    terminal.flush();
    Blynk.run();
    return;
    }
  if (strcmp(cmd_str, "rst") == 0) {  // reset nano
    terminal.println(); // add line feed
    terminal.println("device reset through Blynk terminal");
    // report type of error
    strcpy(error_type_str, "BT");
    nanoError();
    return;
    }
  if (strcmp(cmd_str, "tr") == 0) {  // report time
    // report time data
    terminal.println(); // add line feed
    // terminal.print("nano rtc ");
    // (String) returns time with specified format 
    //terminal.println(rtc.getTime("%A, %B %d %Y %H:%M:%S"));   // for testing
    updateRTC();
    updateDate();
    updateTime();
    terminal.print(date_str);
    terminal.print("  ");
    terminal.println(time_str);
    terminal.println();
    }  
  if (strcmp(cmd_str, "ts") == 0) {  // report and sync rtc and current wifi times
    // get time data
    terminal.println(); // add line feed
    updateTime();
    terminal.print("rtc time was ");
    terminal.println(time_str);
    // update rtc
    updateRTC();
    // read new time
    terminal.println("WiFi and rtc time synced");
    updateTime();
    terminal.print("rtc time is now ");
    terminal.println(time_str);
    terminal.println(); // add line feed
    terminal.flush();
    Blynk.run();
    return;
    }
  if (strcmp(cmd_str, "c") == 0) { // Clear the terminal content    // note - returns 0 if equal
    terminal.clear();  // this is the remote clear.  type clr for a local clear.
    terminal.flush();
    Blynk.run();
    return;
    }
  if (strcmp(cmd_str, "cssid") == 0) { // change Wifi SSID
    terminal.println("Enter new Wifi SSID: ");
    serial_second_line_flag_int = 2;  // set flag for next WifI SSID line read
    terminal.println(); // add line feed
    terminal.flush();
    Blynk.run();
    return;
    }
  if (strcmp(cmd_str, "cpass") == 0) { // change Wifi password
    terminal.println("Enter new Wifi password: ");
    serial_second_line_flag_int = 3;  // set flag for next WifI password line read
    terminal.println(); // add line feed
    terminal.flush();
    Blynk.run();
    return;
    }
  if (strcmp(cmd_str, "cgmto") == 0) { // change GMT offset
    terminal.println("Enter new GMT offset: ");
    serial_second_line_flag_int = 4;  // set flag for next GMT offset line read
    terminal.println(); // add line feed
    terminal.flush();
    Blynk.run();
    return;
    }
  if (strcmp(cmd_str, "v") == 0) {  // report version
    terminal.println(); // add line feed
    terminal.print("Version of Controller Code is: ");
    terminal.println(version_str);
    terminal.println(); // add line feed
    terminal.flush();
    Blynk.run();
    return;
    }
  if (strcmp(cmd_str, "sig") == 0) {  // report wifi signal strength
    rssi = WiFi.RSSI();
    terminal.println(); // add line feed
    terminal.print("Signal strength (RSSI) is ");
    terminal.print(rssi);
    terminal.println(" dBm");
    terminal.println(); // add line feed
    terminal.flush();
    Blynk.run();
    return;
    }
  if (strcmp(cmd_str, "wdt") == 0) {  // check wdt function
    unsigned int t;
    terminal.println("\nWatchdog Test - run 18 seconds with a WDTimer.clear()\n");
    //Serial.println("\nWatchdog Test - run 18 seconds with a WDT.clear()\n");
    for (t = 1; t <= 18; ++t) {
      esp_task_wdt_reset();  // refresh wdt - before it loops
      delay(950);
      terminal.print(t);
      terminal.print(".");
      terminal.flush();
      Blynk.run(); 
      }
    terminal.println("\n\nWatchdog Test - free run wait for reset at 8 seconds\n");
    for (t = 1; t >= 1; ++t) {
      delay(950);
      terminal.print(t);
      terminal.print(".");
      terminal.flush();
      Blynk.run();
      }
    return;
    }   
  if (strcmp(cmd_str, "st") == 0) {
    //terminal.print("redLedState = ");
    //terminal.println(redLedState);
    //terminal.print("blueLedState = ");
    //terminal.println(blueLedState);
    //terminal.flush();  // output to terminal immediately
    terminal.print("uptime = ");
    terminal.print(millis() / 60000);
    terminal.println(" minutes");
    terminal.print("ssid = ");
    terminal.println(ssid);
    terminal.print("pass = ");
    terminal.println(pass);
    terminal.print("GMT offset = ");
    terminal.println(k_GMT_int);
    terminal.println(); // add line
    terminal.flush();
    Blynk.run();
    return;
    }
  
// end of command waterfall
}

// for alarm buttons on Blynk app
BLYNK_WRITE(V4)  // stay
{
stay_sw4Value = param.asInt(); // assigning incoming value from pin V4 to a variable
}
BLYNK_WRITE(V5)  // away
{
away_sw5Value = param.asInt(); // assigning incoming value from pin V5 to a variable
}
BLYNK_WRITE(V6)  // disarm
{
disarm_sw6Value = param.asInt(); // assigning incoming value from pin V6 to a variable
}
BLYNK_WRITE(V7)  // panic
{
panic_sw7Value = param.asInt(); // assigning incoming value from pin V7 to a variable
}
