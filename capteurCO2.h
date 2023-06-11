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
#include <time.h>

#define TS_ENABLE_SSL    // for HTTP secure @ TS, see thingspeak-arduino @ github
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
unsigned long ancienCO2 = 0;

bool demandeEtalonnage = false;
bool displayOFF =false;
bool WifiAvailable = false;
bool ARsend = true;
bool Pro = false;
bool NousAerons = AIR_SCORE;
bool cligno = ALTERN;
bool orig = RINOLFI;

int OnHour = ON_HOUR;
int OnMin = ON_MIN;
int OffHour = OFF_HOUR;
int OffMin = OFF_MIN;
int ARsendNum = 0;

/***********************************************
           Définition des fonctions
 ***********************************************/
 
#include "capteurCO2_ModBus.h" // contains ModBus code focntions
#include "nofokuz.h" // contains new nOfOkUz focntions
#include "rinolfi.h" // contains Rinolfo display focntions
#include "nousaerons.h" // contains AirScore display focntions

/***********************************************
               Initialisation
 ***********************************************/

#include "capteurCO2_setup.h" // contains setup void 

/***********************************************
            Programme pricipale
 ***********************************************/
 
#include "capteurCO2_loop.h" // contains loop void
