/***********************************************
     ### nouvelles fonctions by nOfOkUz ###
 ***********************************************/
 
// 
// nouvelle valeur
//
unsigned long Nouvelle_Valeur()
{
    Serial.println("Nouvelle mesure après " + String(millis()) + " ms");
    send_Request(CO2req, 8);
    read_Response(7);
    unsigned long CO2_val = get_Value(7);
    Serial.println("CO2 : " + String(CO2_val)+ " ppm");
    return CO2_val;
}

//
// envoi de la valeur
//
void Envoi_Valeur(unsigned long CO2_send) {
      if ( !WifiAvailable ) {
          Serial.println(F("Off line"));
          return;
          }
      if ( !ARsend ) {
          ARsendNum++ ;
          Serial.print(F("Valeur précédente non envoyée : ") );
          if ( ARsendNum >5 ) {
             Serial.println(F(" ... reboot cause ARsendNum >5 dans 5s...") );
             delay(5000);
             ESP.restart();
          }
          else {
             Serial.println(String( ARsendNum )+ F(" fois.") );
          }
      }
      if ( Pro ) {
         if ( (WiFi.status() == WL_CONNECTED ) ) {   
           WiFiClient client;
           Serial.print(F("### Connexion au proxy "));
           if ( !client.connect( proxy_name , proxy_port ) ) {
               Serial.println(F(" ..... echec de la connexion web, valeur non envoyée!"));
               ARsend = false ;
           }
           else {
               Serial.print(F(" et envoi de la valeur "));
               Serial.print(CO2_send);
               Serial.println(F(" ppm ###"));
               ARsend = true;
               ARsendNum = 0 ;
               lastTime = millis();
               timerDelay = SENDING_TIME; // attendra SENDING_TIME ms avant le prochain envoi
               String url = "https://" + host + "/update?api_key=" + apiKey + "&" + field + "=" + String(CO2_send);
               Serial.println( url );
               client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                            "Host: " + host + "\r\n" +
                            "User-Agent: greg's ESP32\r\n" +
                            "Connection: close\r\n\r\n");
               while (client.connected()) {
                     String line = client.readStringUntil('\n');
                     Serial.println(line);                            
                     }                         
               }
           Serial.println(F("######################### fin fonction envoi ##########################"));
           } 
         else {
             Serial.println(F("Valeur non envoyée via proxy"));
             Serial.print(F("Plus connecté au WiFi "));
             ARsend = false ;
             WiFi.disconnect();
             WiFi.begin();
             Serial.println(F("... reconnexion au WiFi!"));
         }
      return;
      }
      if( !Pro ) {
          if ((wifiMulti.run() == WL_CONNECTED)) {
// check time before ClientSecure            
              struct tm timeinfo;
              // La config correspond aux changements d'heure du fuseau horaire Europe / Paris.
              // configTzTime("CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", ntp1, ntp2, ntp_pro );
              getLocalTime(&timeinfo);
              Serial.print(F("Current time before ClientSecure : "));
              Serial.print(asctime(&timeinfo));
              WiFiClientSecure client;
              client.setCACert(rootCACertificate);
//              WiFiClient client;
             // with ThinkSpeak library
              Serial.print(F("### Connexion web à ThingSpeak via library "));        
              ThingSpeak.begin(client);
              int result = ThingSpeak.writeField( ChanNum , FieldNum , long(CO2_send) , apiKey );
              Serial.print("... return code : " + String(result) );
              if (result == 200) {
                  Serial.print(F(" ... et envoi de la valeur : "));
                  Serial.print(CO2_send);
                  Serial.println(F(" ppm ###"));
                  lastTime = millis();
                  timerDelay = SENDING_TIME; // attendra SENDING_TIME ms avant le prochain envoi
                  ARsend = true ;
                  ARsendNum = 0 ;
              }
              else {
              Serial.println(" ... problem updating channel, http error code : " + String(result));
              ARsend = false ;
              }
              return;
              // without ThinkSpeak library (need to remove previous and return)           
              Serial.print(F("### Connexion web à "));
              Serial.print(host);
              if (!client.connect(host.c_str(), 443)) {
                  Serial.println(F("... echec de la connexion web"));
                  ARsend = false ;
              } else {
              Serial.print(F(" et envoi de la valeur : "));
              Serial.print(CO2_send);
              Serial.println(F(" ppm ###"));
    
              String url = "https://" + host + "/update?api_key=" + apiKey + "&" + field + "=" + String(CO2_send);
              Serial.println( url );
              client.print(String("GET ") + url + " HTTP/1.1\r\n" +
                           "Host: " + host + "\r\n" +
                           "User-Agent: greg's ESP32\r\n" +
                           "Connection: close\r\n\r\n");
              while (client.connected()) {
                    String line = client.readStringUntil('\n');
                    Serial.println(line);                            
                    }
              Serial.println(F("######################### fin fonction envoi web ##########################"));
              lastTime = millis();
              timerDelay = SENDING_TIME; // attendra SENDING_TIME ms avant le prochain envoi
              ARsend = true ;
              ARsendNum = 0 ;
              }
          }
          else {
              Serial.println(F("Valeur non envoyée"));
              Serial.print(F("Plus connecté au WiFi "));
              if ( !ARsend ) {
                  Serial.println(F("... reboot cause plus connecté WiFi residentiel dans 5s..."));
                  delay(5000);
                  ESP.restart();
                  }
              WiFi.disconnect();
              WiFi.begin();
              Serial.println(F("... reconnexion au WiFi!"));
              }
          return;
          }
}

//
// nettoie l'écran et affiche les infos utiles nOfOkUz
//
void prepareEcran() {
  tft.fillScreen(TFT_BLACK);
  // texte co2 à gauche
  tft.setTextSize(1);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("CO", 35, 128,4);
  tft.setTextSize(1);
  tft.drawString("2", 60, 128,2);
  // texte PPM à droite ppm
  tft.setTextSize(1);
  tft.drawString("ppm", 210, 125,4);
  // écriture du chiffre
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(8);
}

//
//   Affichage de l'écran nOfOKUz
//
void afficheEcran(unsigned long CO2) {

  int seuil = 0;
  // efface le chiffre du texte
  if (CO2 != ancienCO2) {
    tft.fillRect(0, 0, tft.width(), 60, TFT_BLACK);
  }

  if ( CO2 < 800 ) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    if ( seuil != 1 ) {
      tft.setTextSize(1);
      tft.fillRect(0, 77, tft.width(), 26, TFT_BLACK);
      tft.drawString("Air Excellent", tft.width() / 2, tft.height() / 2 + 23,4);
    }
    seuil = 1;
  } else if ( CO2 >= 800 && CO2 < 1000) {
    tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    if ( seuil != 2 ) {
      tft.setTextSize(1);
      tft.fillRect(0, 77, tft.width(), 26, TFT_BLACK);
      tft.drawString("Air Moyen", tft.width() / 2, tft.height() / 2 + 23,4);
    }
    seuil = 2;
  } else if (CO2 >= 1000 && CO2 < 1500) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    if ( seuil != 3 ) {
      tft.setTextSize(1);
      tft.fillRect(0, 77, tft.width(), 26, TFT_BLACK);
      tft.drawString("Air Mediocre", tft.width() / 2, tft.height() / 2 + 23,4);
    }
    seuil = 3;
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    if ( seuil != 4 ) {
      tft.setTextSize(1);
      tft.fillRect(0, 77, tft.width(), 26, TFT_BLACK);
      tft.drawString("Air Vicie", tft.width() / 2, tft.height() / 2 + 23,4);
    }
    seuil = 4;
  }

  tft.setTextSize(3);
  tft.drawString(String(CO2), tft.width() / 2, tft.height() / 2 - 40,4);
  
}

// 
// gestion de l'horloge pour la validation des certificats HTTPS
//
void setClockOnLine() {
  
  configTime(3600, 3600, ntp1 , ntp_pro , ntp2 ); // Paris time
// void configTime(int timezone * 3600, int daylightOffset_sec * 3600, const char* server1, const char* server2, const char* server3)
  Serial.print(F("Waiting for NTP time sync : "));
  time_t nowSecs = time(nullptr);
  while (nowSecs < 8 * 3600 * 2) {
    delay(500);
    Serial.print(F("."));
    yield();
    nowSecs = time(nullptr);
  }
  Serial.println();
  struct tm timeinfo;
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current GMT time : "));
  Serial.print(asctime(&timeinfo));
  getLocalTime(&timeinfo);
  Serial.print(F("Current LOC time : "));
  Serial.println(asctime(&timeinfo));
}

//
//   Set arbitrary time for off line mode nOfOKUz
//
void setClockOffLine() {

  struct tm tm;
    tm.tm_year = 2023 - 1900;
    tm.tm_mon = 06;
    tm.tm_mday = 21;
    tm.tm_hour = 21;
    tm.tm_min = 00;
    tm.tm_sec = 00;
    time_t t = mktime(&tm);
    printf("Setting time for off line mode : %s", asctime(&tm));
    struct timeval now = { .tv_sec = t };
    settimeofday(&now, NULL); 
  struct tm timeinfo;
  time_t nowSecs = time(nullptr);
  gmtime_r(&nowSecs, &timeinfo);
  Serial.print(F("Current GMT time : "));
  Serial.print(asctime(&timeinfo));
  getLocalTime(&timeinfo);
  Serial.print(F("Current LOC time : "));
  Serial.println(asctime(&timeinfo));
}
