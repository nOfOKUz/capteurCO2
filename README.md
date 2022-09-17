# capteurCO2


Based on https://co2.rinolfi.ch/ , i modified the Arduino prog to accommodate both several residential WiFis and WPA2 enterprise professional WiFi with a proxy, and upload to Thingspeak.com via library API.


A few lines are useless for now, but could be used for Eduroam in near future...


There are also a lot of debug lines via serial print...


Also added mechanism :


    to avoid dropping value for too long when update on Thingspeak is invalid
  
    to reconnect to wifi if connection lost
  
    to reboot if nothing uploads more than 5 times
  
    in residential Wifi sending subroutines, you can find both Thingspeak sending value with library or without
    
https://github.com/mathworks/thingspeak-arduino
  
Updated version with more include files more for simplicity and easily adapt to several detectors corresponding to different ThnigSpeak channels.


NOTE also I cannot have the secure connection to ThingSpeak anymore since 19 July 2022, so I switched to non-secure connection (I left the secured code part as comments).

