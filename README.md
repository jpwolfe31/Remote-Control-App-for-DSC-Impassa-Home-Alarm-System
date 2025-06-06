Remote Control iPhone App for DSC Impassa Home Alarm System using the Blynk app

This Arduino Nano-ESP32 based application allows the user to control certain functions of an Impassa Home Alarm System remotely with the user’s iPhone.  Primary functions include, arming, disarming and alarm monitoring, and monitoring wired alarm zones connected through a RE508X translator.   The application can also be configured to monitor networked smoke detectors.

Arm and disarm functions operate through a key fob board mounted next to the Nano-ESP32.

The application uses the two Impassa program outputs for Away Armed State and Bell State.  The user will need the alarm installer code to program these.  The application also uses two led outputs to show ready to arm and armed/arming.  The two led outputs were connected to the alternate communication screw terminals as shown below.  The traces to these terminals were cut near the terminals with a drill bit to enable their redirection.  These could also be hooked up with wire connectors to avoid any trace cutting.

The iPhone uses the Blynk app to send and receive messages to the Nano-ESP32.  The Nano-ESP32 sends alerts and emails to the iPhone through the Blynk app in the event of an alarm.  

Unfortunately, Blynk is not supporting new makers on its app right now, but I believe this program could be modified to work with the Aurduino Cloud IOT and its messenger, button and led widgets.

See folders above for code, schematics and build information.  Feel free to email me at jpwolfe31@yahoo.com if you have any questions.

Another project just finished and posted here uses a WT5500 keypad and the Blynk app to view the alarm system’s LCD display and control the system’s keypad.  This app has its own virtual lcd display and keypad.

DSC Impassa operation, installation and programming documents are available for download at alarmgrid.com and elsewhere.
