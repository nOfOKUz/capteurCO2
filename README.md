# capteurCO2

Based on https://co2.rinolfi.ch/ , i modified the Arduino prog to accommodate both several residential WiFis and WPA2 enterprise professional WiFi with a proxy, and upload to Thingspeak.com via library API.

A few lines are useless for now, but could be use for Eduroam in near futur...

There are also a lot of debug lines via serial print...

Also added mecanisism :

    to avoid dropped value for too long when update on Thingspeak is invalid
  
    to reconnect to wifi if connection lost
  
    to reboot if nothing uploads more than 3 times
  
    in residential Wifi sending sub routine, you can find both Thingspeak sending value with library or without
  
