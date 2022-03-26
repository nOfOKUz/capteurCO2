# capteurCO2

Based on https://co2.rinolfi.ch/ , i modified the Arduino prog to accommodate both several residential WiFis and WPA2 enterprise professional WiFi with a proxy, and upload to Thingspeak.com via library API.

A few lines are useless for now, but could be use for Eduroam in near futur...

There are also a lot of debug lines via serial print...

Also added mecanisism to avoid dopped value for too long when update on Thingspeak is invalid.
