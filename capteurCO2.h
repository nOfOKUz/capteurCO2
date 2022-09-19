/******************************************************
     Inspiré de Capteur de CO2 par Grégoire Rinolfi
     https://co2.rinolfi.ch
 ******************************************************/
#include <TFT_eSPI.h>
#include <SPI.h>
#include <Wire.h>
#include <WiFiMulti.h>
#include <WiFiClientSecure.h>
#include <WiFi.h>
#include <esp_wifi.h>    // ajout de ESP32_WPA2.ino (wpa2 error tests)
#include <esp_wpa2.h>
#include <WiFiClient.h> 

//#define TS_ENABLE_SSL    // for HTTP secure @ TS, see thingspeak-arduino @ github
#include <ThingSpeak.h>
#include "certificate.h" // contains root CA Certificates fro HTTPS
#include "secret.h"      // contains all personal data in same directory

WiFiMulti wifiMulti;
TFT_eSPI tft = TFT_eSPI(135, 240);

/************************************************
     Paramètres utilisateur
 ***********************************************/

#define TXD2 21         // série capteur TX
#define RXD2 22         // série capteur RX
#define BOUTON_CAL 35
#define DEBOUNCE_TIME 1000
#define MESURE_TIME 5000    // 5 secondes avant la prochaine mesure
#define SENDING_TIME 300000 // 5 min avant le prochain envoi

// Wifi WPA2 Entreprise user parameters
#define EAP_IDENTITY EMAIL_PRO          //email pro identity
#define EAP_USERNAME ME_USER            //user name pro
#define EAP_PASSWORD ME_PWD             //your pro password
const char* ssidPRO     = SSID_PRO;
const char* proxy_name  = PROXY1_PRO;
const int proxy_port    = PROXY_PORT_PRO;
const char* ntp_pro     = NTP_PRO;

// WiFi Eduroam connection
#include "eduroam.h"
const static char* test_root_ca PROGMEM = EDUROAM_TEST_ROOT_CA;
const char * client_crt = EDUROAM_CLIENT_CRT;
const char * client_key = EDUROAM_CLIENT_KEY;
     
// Wifi résidentiel user parameters
const char* ssid1     = SSID_RES1;
const char* password1 = PWD_RES1;
const char* ssid2     = SSID_RES2;
const char* password2 = PWD_RES2;
const char* ssid3     = SSID_RES3;
const char* password3 = PWD_RES3;
const char* ssid4     = SSID_RES4;
const char* password4 = PWD_RES4;
const char* ntp1      = NTP_RES1;
const char* ntp2      = NTP_RES2;

// Thingspeak user parameters
const String host     = "api.thingspeak.com";        // "api.thingspeak.com" "3.213.58.187" "34.231.233.177"
const char* apiKey    = ME_APIKEY;
String field    = DETEC_FIELD;
const unsigned int FieldNum = DETEC_NUM;
const unsigned long ChanNum = ME_CHANEL;
const char* HostName  = DETEC_NAME;        // changer HostName en fonction du détecteur

// Certificat HTTPS pour envoi à thingspeak.com
const char* rootCACertificate = SECRET_TS_ROOT_CA ;
  
// variables globales du programme pricipal
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;  // valeur initiale de 10s (puis SENDING_TIME)
bool WifiAvailable = false;
bool ARsend = true;
bool Pro = false;
int ARsendNum = 0;

/***********************************************
           Définition des fonctions
 ***********************************************/
 
// gestion de l'horloge pour la validation des certificats HTTPS
void setClock() {
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

/***************************************************
   Code de gestion du capteur CO2 via ModBus
   inspiré de : https://github.com/SFeli/ESP32_S8
 ***************************************************/
volatile uint32_t DebounceTimer = 0;

byte CO2req[] = {0xFE, 0X04, 0X00, 0X03, 0X00, 0X01, 0XD5, 0XC5};
byte ABCreq[] = {0xFE, 0X03, 0X00, 0X1F, 0X00, 0X01, 0XA1, 0XC3};
byte disableABC[] = {0xFE, 0X06, 0X00, 0X1F, 0X00, 0X00, 0XAC, 0X03};  // écrit la période 0 dans le registre HR32 à adresse 0x001f
byte enableABC[] = {0xFE, 0X06, 0X00, 0X1F, 0X00, 0XB4, 0XAC, 0X74};   // écrit la période 180
byte clearHR1[] = {0xFE, 0X06, 0X00, 0X00, 0X00, 0X00, 0X9D, 0XC5};    // ecrit 0 dans le registe HR1 adresse 0x00
byte HR1req[] = {0xFE, 0X03, 0X00, 0X00, 0X00, 0X01, 0X90, 0X05};      // lit le registre HR1 (vérifier bit 5 = 1 )
byte calReq[] = {0xFE, 0X06, 0X00, 0X01, 0X7C, 0X06, 0X6C, 0XC7};      // commence la calibration background
byte Response[20];
uint16_t crc_02;
int ASCII_WERT;
int int01, int02, int03;
unsigned long ReadCRC;      // CRC Control Return Code

void send_Request (byte * Request, int Re_len)
{
  while (!Serial1.available())
  {
    Serial1.write(Request, Re_len);   // Send request to S8-Sensor
    delay(50);
  }
  Serial.print(F("Requete : "));
  for (int02 = 0; int02 < Re_len; int02++)    // Empfangsbytes
  {
    Serial.print(Request[int02], HEX);
    Serial.print(F(" "));
  }
  Serial.println();
}

void read_Response (int RS_len)
{
  int01 = 0;
  while (Serial1.available() < 7 )
  {
    int01++;
    if (int01 > 10)
    {
      while (Serial1.available())
        Serial1.read();
      break;
    }
    delay(50);
  }
  Serial.print(F("Reponse : "));
  for (int02 = 0; int02 < RS_len; int02++)    // Empfangsbytes
  {
    Response[int02] = Serial1.read();

    Serial.print(Response[int02], HEX);
    Serial.print(F(" "));
  }
  Serial.println();
}

unsigned short int ModBus_CRC(unsigned char * buf, int len)
{
  unsigned short int crc = 0xFFFF;
  for (int pos = 0; pos < len; pos++) {
    crc ^= (unsigned short int)buf[pos];   // XOR byte into least sig. byte of crc
    for (int i = 8; i != 0; i--) {         // Loop over each bit
      if ((crc & 0x0001) != 0) {           // If the LSB is set
        crc >>= 1;                         // Shift right and XOR 0xA001
        crc ^= 0xA001;
      }
      else                            // else LSB is not set
        crc >>= 1;                    // Just shift right
    }
  }  // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
  return crc;
}

unsigned long get_Value(int RS_len)
{
  // Check the CRC //
  ReadCRC = (uint16_t)Response[RS_len - 1] * 256 + (uint16_t)Response[RS_len - 2];
  if (ModBus_CRC(Response, RS_len - 2) == ReadCRC) {
    // Read the Value //
    long val = (uint16_t)Response[3] * 256 + (uint16_t)Response[4];
    return val * 1;       // S8 = 1. K-30 3% = 3, K-33 ICB = 10
  }
  else {
    Serial.print(F("CRC Error"));
    return 99;
  }
}

// interruption pour lire le bouton d'étalonnage
bool demandeEtalonnage = false;
void IRAM_ATTR etalonnage() {
  if ( millis() - DEBOUNCE_TIME  >= DebounceTimer ) {
    DebounceTimer = millis();
    Serial.println(F("Etalonnage manuel !!"));
    tft.fillScreen(TFT_BLACK);
    tft.setTextSize(3);
    tft.setTextColor(TFT_WHITE);
    tft.drawString("Etalonnage", tft.width() / 2, tft.height() / 2);
    demandeEtalonnage = true;
  }
}

// nettoie l'écran et affiche les infos utiles
void prepareEcran() {
  tft.fillScreen(TFT_BLACK);
  // texte co2 à gauche
  tft.setTextSize(4);
  tft.setTextColor(TFT_WHITE);
  tft.drawString("CO", 20, 110);
  tft.setTextSize(3);
  tft.drawString("2", 55, 115);
  // texte PPM à droite ppm
  tft.drawString("ppm", 190, 105);
  // écriture du chiffre
  tft.setTextColor(TFT_GREEN, TFT_BLACK);
  tft.setTextSize(8);
}

/***********************************************
     ### nouvelles fonctions by nOfOkUz ###
 ***********************************************/
 
// nouvelle valeur
unsigned long Nouvelle_Valeur()
{
    Serial.println("Nouvelle mesure après " + String(millis()) + " ms");
    send_Request(CO2req, 8);
    read_Response(7);
    unsigned long CO2_val = get_Value(7);
    Serial.println("CO2 : " + String(CO2_val)+ " ppm");
    return CO2_val;
}

// envoi de la valeur
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
              Serial.println(asctime(&timeinfo));
//              WiFiClientSecure client;
//              client.setCACert(rootCACertificate);
              WiFiClient client;
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

/***********************************************
               Initialisation
 ***********************************************/
 
void setup() {
  // bouton de calibration
  pinMode(BOUTON_CAL, INPUT);

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
  delay(20);
  tft.setRotation(1);
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

  Serial.print(F("Connexion au wifi résidentiel :"));
  tft.setTextSize(2);
  tft.setTextColor(TFT_WHITE, TFT_BLACK);
  tft.drawString("Recherche wifi", tft.width() / 2, tft.height() / 2);

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
    Serial.println("Connecté au wifi " + WiFi.SSID() );    
    Serial.print(F("RRSI : "));
    Serial.print(WiFi.RSSI());
    Serial.println(F(" db."));
    Serial.print(F("IP address set : "));
    Serial.println(WiFi.localIP());        //print LAN IP
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    tft.drawString("Wifi OK " + WiFi.SSID(), tft.width() / 2, 100);
    setClock();
      } else {
            Serial.println(F(" ECHEC!"));
            WifiAvailable = false;
            WiFi.disconnect(true);   //disconnect form wifi to set new wifi connection
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
// for ESP32 cards v2.0.2 or higher (tested up to v2.0.5)
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
              Serial.println("Connecté au wifi " + WiFi.SSID() ); 
              tft.setTextColor(TFT_GREEN, TFT_BLACK);
              tft.drawString("Wifi OK " + WiFi.SSID(), tft.width() / 2, 100);
              setClock();
                } else {
                  tft.setTextColor(TFT_RED, TFT_BLACK);
                  Serial.println(F(" ECHEC!"));
                  WifiAvailable = false;
                  tft.drawString("Pas de wifi", tft.width() / 2, 100);
                }
             Serial.println();
             }
   delay(3000); // laisse un temps pour lire les infos

  // préparation de l'écran
  prepareEcran();

  //interruption de lecture du bouton
  attachInterrupt(BOUTON_CAL, etalonnage, FALLING);
}

unsigned long ancienCO2 = 0;
int seuil = 0;

/***********************************************
            Programme pricipale
 ***********************************************/
 
void loop() {

  // effectue l'étalonnage si on a appuyé sur le bouton
  if ( demandeEtalonnage ) {
    demandeEtalonnage = false;
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
    prepareEcran();
    seuil = 0;
    
    // envoi nouvelle mesure après étalonage
    Serial.println(F("Envoi nouvelle valeur après étalonage"));
    unsigned long CO2_tmp = Nouvelle_Valeur();
    Envoi_Valeur(CO2_tmp);
    delay(3000); // attend 3 secondes de plus pour retour cycle normal
    Serial.println();
      
    }

  // lecture du capteur
  unsigned long CO2 = Nouvelle_Valeur();

  // efface le chiffre du texte
  if (CO2 != ancienCO2) {
    tft.fillRect(0, 0, tft.width(), 60, TFT_BLACK);
  }

  if ( CO2 < 800 ) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    if ( seuil != 1 ) {
      tft.setTextSize(2);
      tft.fillRect(0, 61, tft.width(), 25, TFT_BLACK);
      tft.drawString("Air Excellent", tft.width() / 2, tft.height() / 2 + 10);
    }
    seuil = 1;
  } else if ( CO2 >= 800 && CO2 < 1000) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    if ( seuil != 2 ) {
      tft.setTextSize(2);
      tft.fillRect(0, 61, tft.width(), 25, TFT_BLACK);
      tft.drawString("Air Moyen", tft.width() / 2, tft.height() / 2 + 10);
    }
    seuil = 2;
  } else if (CO2 >= 1000 && CO2 < 1500) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    if ( seuil != 3 ) {
      tft.setTextSize(2);
      tft.fillRect(0, 61, tft.width(), 25, TFT_BLACK);
      tft.drawString("Air Mediocre", tft.width() / 2, tft.height() / 2 + 10);
    }
    seuil = 3;
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    if ( seuil != 4 ) {
      tft.setTextSize(2);
      tft.fillRect(0, 61, tft.width(), 25, TFT_BLACK);
      tft.drawString("Air Vicie", tft.width() / 2, tft.height() / 2 + 10);
    }
    seuil = 4;
  }

  tft.setTextSize(8);
  tft.drawString(String(CO2), tft.width() / 2, tft.height() / 2 - 30);

  // envoi de la valeur sur le cloud tous les timerDelay
  if ((millis() - lastTime) > timerDelay) {
    Serial.println();
    Envoi_Valeur(CO2);  
    }

  ancienCO2 = CO2;
  Serial.println();
  delay(MESURE_TIME); // attend MESURE_TIME ms avant la prochaine mesure
}
