/***********************************************
               Initialisation
 ***********************************************/

void setup() {
  
  // bouton de calibration
  pinMode(BOUTON_CAL, INPUT);
  pinMode(TFT_BL,OUTPUT); // TFT Backlight (TFT_BL = 4)

  // ports série de debug et de communication capteur
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // vérif détecteur
  Serial.println();
  Serial.println(F("##########################"));
  Serial.println( HostName );  
  Serial.println( "ThingSpeak Field : " + String(field) );   
  Serial.println( "Channel Number : " + String(FieldNum) );
  Serial.println(F("##########################"));
  Serial.println();
  
  // initialise l'écran
  tft.init();
  delay(10);
  tft.fillScreen(TFT_BLACK);
  tft.setRotation(1);
  tft.setTextDatum(TL_DATUM); // imprime la string top left
  tft.setTextColor(TFT_GREEN,TFT_BLACK);
  tft.drawString(F("Wake up, Neo..."),10,0,2);
  tft.drawString(F("The Matrix has you..."),10,16,2);
  tft.drawString(F("Follow the white rabbit."),10,32,2);
  tft.drawString(F("Knock, knock, Neo..."),10,48,2);
  delay(3000);
  tft.fillScreen(TFT_BLACK);
  tft.setTextDatum(MC_DATUM); // imprime la string middle centre
 
  // vérifie l'état de l'ABC
  Serial.println(F("Vérification de l'ABC"));
  send_Request(ABCreq, 8);
  read_Response(7);
  Serial.print(F("Période ABC : "));
  Serial.printf("%02ld", get_Value(7));
  Serial.println();
  Serial.println();
  int abc = get_Value(7);

  // active ou désactive l'ABC au démarrage
  Serial.println(F("Activation ou désactivation de l'ABC au démarrage"));
  if (digitalRead(BOUTON_CAL) == LOW) {
    if (abc == 0) {
      send_Request(enableABC, 8);
    } else {
      send_Request(disableABC, 8);
    }
    read_Response(7);
    get_Value(7);
  }
  Serial.println(F("Lecture bouton finie"));
  tft.setTextSize(2);
  tft.setTextColor(TFT_BLUE, TFT_BLACK);
  tft.drawString("Autocalibration", tft.width() / 2, 10);
  if ( abc != 0 ) {
    tft.drawString(String(abc) + "h", tft.width() / 2, 40);
  } else {
    tft.drawString("OFF", tft.width() / 2, 40);
  }

  // gestion du wifi
  Serial.println();
  WiFi.setHostname(HostName); //set Hostname for your device - not neccesary
  Serial.println("WiFi HostName : " + String(HostName) );  // vérif HostName
  wifiMulti.addAP(ssid1, password1);
  wifiMulti.addAP(ssid2, password2);
  wifiMulti.addAP(ssid3, password3);
  wifiMulti.addAP(ssid4, password4);

  Serial.print(F("Connexion au WiFi résidentiel :"));
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Recherche WiFi", tft.width() / 2, tft.height() / 2);

  int i = 0;
  while (wifiMulti.run() != WL_CONNECTED && i < 5) {
    Serial.print(F("."));
    delay(500);
    i++;
  }
    if (wifiMulti.run() == WL_CONNECTED) {
    Serial.println(F(" CONNECTED!"));
    WifiAvailable = true;
    Pro = false;
    Serial.println("Connecté au WiFi " + WiFi.SSID() );    
    Serial.print(F("RRSI : "));
    Serial.print(WiFi.RSSI());
    Serial.println(F(" db."));
    Serial.print(F("IP address set : "));
    Serial.println(WiFi.localIP());        //print LAN IP
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Wifi OK " + WiFi.SSID(), tft.width() / 2, 100);
    setClockOnLine();
      } else {
            Serial.println(F(" ECHEC!"));
            WifiAvailable = false;
            WiFi.disconnect(true);   //disconnect from wifi to set new wifi connection
            Serial.print(F("Connecting to WiFi WPA2 entreprise : "));
            Serial.println(ssidPRO);
            WiFi.mode(WIFI_STA);     //init wifi mode infrastructure (!= AP)
            Serial.print(F("MAC >> "));
            Serial.println(WiFi.macAddress());
// for Eduroam only
//            if( esp_wifi_sta_wpa2_ent_set_ca_cert((uint8_t *)test_root_ca, strlen(test_root_ca) + 1) ){
//              Serial.println(F("Failed to set WPA2 certificat"));
//                 } else { Serial.println(F("ESP_OK WPA2 certificat,"));}    
//            if( esp_wifi_sta_wpa2_ent_set_cert_key((uint8_t *)client_crt, strlen(client_crt), (uint8_t *)client_key, strlen(client_key), NULL, 0) ){
//              Serial.println("Failed to set WPA2 Client Certificate and Key");   
//                 } else { Serial.println(F("ESP_OK WPA2 client certificat and key,"));}    
// WPA2 enterprise magic            
            Serial.println(F("Setting WiFi WPA2 magic :"));
            esp_err_t error = ESP_OK;
            error += esp_wifi_sta_wpa2_ent_set_identity((uint8_t *)EAP_IDENTITY, strlen(EAP_IDENTITY)); //provide identity
              if( error != ESP_OK ){
                 Serial.println(F("Failed to set WPA2 Identity"));
                 } else { Serial.println(F("ESP_OK WPA2 identity,"));}
            error += esp_wifi_sta_wpa2_ent_set_username((uint8_t *)EAP_USERNAME, strlen(EAP_USERNAME)); //provide username
            if( error != ESP_OK ){
                  Serial.println(F("Failed to set WPA2 Username"));
                  } else { Serial.println(F("ESP_OK WPA2 username,"));}
            error += esp_wifi_sta_wpa2_ent_set_password((uint8_t *)EAP_PASSWORD, strlen(EAP_PASSWORD)); //provide password
            if( error != ESP_OK ){
                  Serial.println(F("Failed to set WPA2 Password"));
                  } else { Serial.println(F("ESP_OK WPA2 password,"));}
            if ( error != ESP_OK ) {
                Serial.println(F("Error setting WPA2 properties."));
                } else { Serial.println(F("ESP_OK WPA2 properties."));}
           Serial.println(F("--> END Setting WiFi WPA2 magic"));
           Serial.println(F("Enabling WiFi STA"));
           WiFi.enableSTA(true);    // from ESP32-Eduroam-2018.ino martinius96
// for ESP32 cards v1.0.6
//           esp_wpa2_config_t configWPA2 = WPA2_CONFIG_INIT_DEFAULT(); //set config settings to default for ESP32 cards before v 1.0.6
//           error = esp_wifi_sta_wpa2_ent_enable(&configWPA2);         //set config settings to enable function
// for ESP32 cards v2.0.2 or higher (tested up to v2.0.9)
           error = esp_wifi_sta_wpa2_ent_enable();                  
           if( error != ESP_OK ){
              ESP_LOGV(TAG, "esp_wifi_sta_wpa2_ent_enable failed! %d", err);
              Serial.println(F("Failed to enable WPA2"));
              } else { Serial.println(F("ESP_OK WPA2 enable"));}

//            Serial.print(F("ESP WiFi connecting : "));
//            error = esp_wifi_connect();                         // ajout de ESP32_WPA2.ino (tous les tests)    
//            if ( error == ESP_OK ) {
//              Serial.println(F("ESP_OK"));
//              } else {
//                Serial.print("error code : ");
//                Serial.println( error );
//                }
            
            Serial.print(F("Beginning WiFi SSID_PRO : "));  
            WiFi.begin(ssidPRO);
            int counter = 0 ;
            while (WiFi.status() != WL_CONNECTED && counter < 15) {
                  delay(1000);
                  Serial.print(F("."));
                  counter++;
             }
            if (WiFi.status() == WL_CONNECTED) {
              Serial.println(F(" CONNECTED!"));
              WifiAvailable = true;
              Pro = true;
              Serial.print(F("RRSI: "));
              Serial.print(WiFi.RSSI());
              Serial.println(F(" db."));
              Serial.print(F("IP address set : "));
              Serial.println(WiFi.localIP());         //print LAN IP
              Serial.println("Connecté au WiFi " + WiFi.SSID() ); 
              tft.setTextColor(TFT_GREEN, TFT_BLACK);
              tft.drawString("Wifi OK " + WiFi.SSID(), tft.width() / 2, 100);
              setClockOnLine();
                } else {
                  tft.setTextColor(TFT_RED, TFT_BLACK);
                  Serial.println(F(" ECHEC!"));
                  WifiAvailable = false;
                  tft.drawString("Pas de WiFi", tft.width() / 2, 100);
                  setClockOffLine();
                }
             Serial.println();
             }
   delay(3000); // laisse un temps pour lire les infos

  // préparation de l'écran
      if ( orig ) {
      prepareEcranOrig();
    } else {
        if ( !NousAerons ) {  prepareEcran(); }
        }

  //interruption de lecture du bouton
  
  attachInterrupt(BOUTON_CAL, etalonnage, FALLING);
}
