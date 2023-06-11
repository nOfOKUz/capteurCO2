# capteurCO2

2022 UPDATE
 
Based on https://co2.rinolfi.ch/ , i modified the Arduino prog to accommodate both several residential WiFis and WPA2 enterprise professional WiFi with a proxy, and upload to Thingspeak.com via library API.


A few lines are useless for now, but could be used for Eduroam in near future... (maybe not so close...)


There are also a lot of debug lines via serial print...


Also added mechanism :


    to avoid dropping value for too long when update on Thingspeak is invalid

  
    to reconnect to wifi if connection lost

  
    to reboot if nothing uploads more than 5 times

  
    in residential Wifi sending subroutines, you can find both Thingspeak sending value with library or without

    
https://github.com/mathworks/thingspeak-arduino

Updated version with more include files more for simplicity and easily adapt to several detectors corresponding to different ThnigSpeak channels.

2023 UPDATE

	Add more subfiles for clarity

	Improved classic display (Rinolfi's original still possible)

	Include AirScore display from nousaerons.fr

	Possibility to alternate between AirScore and CO2 value

	Add time slot to turn off screen during night (for bedrooms)

	Time slot also possible in off line mode if detector turn on at given time

	When sreen off for night, press button displays temporary on instead of manuel calibration (still manual calibration when screen-on time).






