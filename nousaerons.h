/************************************************
          Printing Screen Infos fonction
          by nousaerons.fr with AirScore
          https://nousaerons.fr
***********************************************/

void afficheEcranAirScore(unsigned long CO2) {

  String CO2s, airScore;
  CO2s = "CO2 " + String(CO2) + " ppm";

  if (CO2 != ancienCO2) {   
    tft.fillScreen(TFT_BLACK);
  }
   
  tft.fillRoundRect(0,10,48,60,10,TFT_DARKGREEN);
  tft.fillRect(10,10,38,60,TFT_DARKGREEN);
  tft.setTextSize(5);
  tft.setTextColor(TFT_DARKGREY, TFT_DARKGREEN);
  tft.drawChar('A',10,22);
  
  tft.fillRect(48,10, 48, 60, TFT_GREEN);
  tft.setTextSize(5);
  tft.setTextColor(TFT_DARKGREY, TFT_GREEN);
  tft.drawChar('B',59,22);
  
  tft.fillRect(96,10,48,60,TFT_GREENYELLOW);
  tft.setTextSize(5);
  tft.setTextColor(TFT_DARKGREY, TFT_GREENYELLOW);
  tft.drawChar('C',107,22);
 
  tft.fillRect(144,10,48,60,TFT_ORANGE);
  tft.setTextSize(5);
  tft.setTextColor(TFT_DARKGREY, TFT_ORANGE);
  tft.drawChar('D',156,22);

  tft.fillRoundRect(192,10,48,60,10,TFT_RED);
  tft.fillRect(192,10,20,60,TFT_RED);
  tft.setTextSize(5);
  tft.setTextColor(TFT_DARKGREY,TFT_RED);
  tft.drawChar('E',204,22);


  if (CO2 == 0)
  {
    airScore = "Initialisation...";    
  }
  else if (CO2 < 601)
  {
    tft.fillRoundRect(0,0,68,80,15,TFT_DARKGREEN);
    tft.drawRoundRect(0,0,68,80,15,TFT_WHITE);
    tft.drawRoundRect(1,1,66,78,14,TFT_WHITE);
    tft.setTextSize(6);
    tft.setTextColor(TFT_WHITE, TFT_DARKGREEN);
    tft.drawChar('A',18,20);
    airScore = "AirScore excellent";
  }
  else if (CO2 < 801)
  {
    tft.fillRoundRect(40,0,68,80,15,TFT_GREEN);
    tft.drawRoundRect(40,0,68,80,15,TFT_WHITE);
    tft.drawRoundRect(41,1,66,78,14,TFT_WHITE);
    tft.setTextSize(6);
    tft.setTextColor(TFT_WHITE, TFT_GREEN);
    tft.drawChar('B',59,20);
    airScore = "AirScore satisfaisant";  
  }
  else if (CO2 < 1001)
  {
    tft.fillRoundRect(88,0,68,80,15,TFT_YELLOW);
    tft.drawRoundRect(88,0,68,80,15,TFT_WHITE);
    tft.drawRoundRect(89,1,66,78,14,TFT_WHITE);
    tft.setTextSize(6);
    tft.setTextColor(TFT_WHITE, TFT_YELLOW);
    tft.drawChar('C',107,20);
    airScore = "AirScore moyen";  
  }
  else if (CO2 < 1501)
  {
    tft.fillRoundRect(136,0,68,80,15,TFT_ORANGE);
    tft.drawRoundRect(136,0,68,80,15,TFT_WHITE);
    tft.drawRoundRect(137,1,66,78,14,TFT_WHITE);
    tft.setTextSize(6);
    tft.setTextColor(TFT_WHITE, TFT_ORANGE);
    tft.drawChar('D',156,20);
    airScore = "AirScore mediocre";  
  }
  else if (CO2 >= 1501)
  {
    tft.fillRoundRect(171,0,68,80,15,TFT_RED);
    tft.drawRoundRect(171,0,68,80,15,TFT_WHITE);
    tft.drawRoundRect(172,1,66,78,14,TFT_WHITE);
    tft.setTextSize(6);
    tft.setTextColor(TFT_WHITE, TFT_RED);
    tft.drawChar('E',192,20);
    airScore = "AirScore insuffisant";  
  }

  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString(airScore, 120, 100, 4);

  CO2s = "CO2  " + String(CO2) + "  ppm";  
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  if (CO2 != ancienCO2) { tft.fillRect(0, 113,tft.width(),26 ,TFT_BLACK); }
  tft.drawString(CO2s, 120, 127, 4);
}
