/***********************************************
            Programme pricipale
 ***********************************************/
 
void loop() {

  // effectue l'étalonnage si on a appuyé sur le bouton
  if ( demandeEtalonnage && !displayOFF ) {
    Serial.println(F("Etalonnage manuel !"));
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Etalonnage", tft.width() / 2, tft.height() / 2);
  
    // nettoye le registre de verification
    send_Request(clearHR1, 8);
    read_Response(8);
    delay(100);
    // demande la calibration
    send_Request(calReq, 8);
    read_Response(8);
    delay(4500); // attend selon le cycle de la lampe

    // lit le registre de verification    
    send_Request(HR1req, 8);
    read_Response(7);
    int verif = get_Value(7);
    Serial.println("resultat calibration " + String(verif));
    if (verif == 32) {
      tft.setTextColor(TFT_GREEN);
      tft.drawString("OK", tft.width() / 2, tft.height() / 2 + 30);
    } else {
      tft.setTextColor(TFT_RED);
      tft.drawString("Erreur", tft.width() / 2, tft.height() / 2 + 20);
    }
    Serial.println();
    delay(5000);
         
    // envoi nouvelle mesure après étalonage
    Serial.println(F("Envoi nouvelle valeur après étalonage"));
    unsigned long CO2_tmp = Nouvelle_Valeur();
    Envoi_Valeur(CO2_tmp);
    delay(3000); // attend 3 secondes de plus pour retour cycle normal
    Serial.println();
    
    // Préparation de l'écran après calibration
    if ( orig ) {
      prepareEcranOrig();
    } else {
        if ( !NousAerons ) {
          prepareEcran();
          }
          else {
            tft.fillScreen(TFT_BLACK);
          }
        }
    cligno = ALTERN;    // pour repartir sur le type d'affichage demandé initialement
    demandeEtalonnage = false;  
  // fin demande étalonage  
  }


  // lecture du capteur  
  unsigned long CO2 = Nouvelle_Valeur();
  
  // envoi de la valeur sur le cloud tous les timerDelay
  if ((millis() - lastTime) > timerDelay) {
    Serial.println();
    Envoi_Valeur(CO2);  
    }

  // affichage de la valeur si dans la plage horaire
  time_t CurentTime = time( NULL );
  Serial.print(F("Current time for display : "));
  Serial.print(ctime(&CurentTime));
  struct tm *Info = localtime( &CurentTime );
  int nb_sec = Info->tm_hour*3600+Info->tm_min*60+Info->tm_sec;
  bool debut = nb_sec >= OnHour*3600+OnMin*60;
  bool fin   = nb_sec < OffHour*3600+OffMin*60;

  if ( orig ) {
       afficheEcranOrig(CO2);
       } else {
         if ( debut && fin ){
          displayOFF = false;
          tft.writecommand(0x11); // wake-up display from sleep
          digitalWrite(TFT_BL,HIGH); // force backlight on
          Serial.println(F("Display ON"));
          cligno = cligno xor ALTERN;
          if (NousAerons xor cligno) {
            if ( ALTERN ) { tft.fillScreen(TFT_BLACK); }
            afficheEcranAirScore(CO2);
            } else {
            if ( ALTERN ) { prepareEcran(); }
            afficheEcran(CO2);
            }         
         } else {
          if ( demandeEtalonnage && displayOFF  ) {  //display 5s if button pressed while display off
            demandeEtalonnage = false;
            tft.writecommand(0x11); // wake-up display from sleep
            digitalWrite(TFT_BL,HIGH); // force backlight on
            Serial.println(F("Display temporary on"));
            displayOFF = false;
            prepareEcran();
            afficheEcran(CO2);
            delay (7000);
          }            
          Serial.println(F("Display OFF"));
           if (!displayOFF) {
            tft.fillScreen(TFT_BLACK);  
            tft.setTextColor(TFT_BLUE, TFT_BLACK);
            tft.setTextSize(1);
            tft.drawString("Bonne nuit...", tft.width()/2, tft.height()/2, 4);
            delay (5000);
            tft.writecommand(0x10); // put display to sleep
            delay (3000);
            digitalWrite(TFT_BL,LOW); // force backlight off
           }
          displayOFF = true;
        }
      }
  
  ancienCO2 = CO2;
  delay(MESURE_TIME); // attend MESURE_TIME ms avant la prochaine mesure
  Serial.println();
}
