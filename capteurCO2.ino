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
#define TS_ENABLE_SSL   // see thingspeak-arduino @ github (to keep before include library)
#include <ThingSpeak.h>
#include "secret.h"     // contain all personal data in same directory

WiFiMulti wifiMulti;
TFT_eSPI tft = TFT_eSPI(135, 240);

/************************************************
     Paramètres utilisateur
 ***********************************************/
 
// Thingspeak user parameters

// field1 --> black sensor
// field2 --> white sensor
// filed3 --> gold sensor
// filed4 --> orange sensor
// filed5 --> silver sensor
// filed6 --> grey sensor

const char* HostName        = "Detecteur CO2 white";       // changer HostName en fonction du détecteur
String field                = "field2";                    // changer field en fonction du détecteur
const unsigned int FieldNum = 2;                           // changer FieldNum en fonction du détecteur

const String host            = "api.thingspeak.com";       // "api.thingspeak.com" "3.213.58.187" "34.231.233.177"
const char* apiKey           = ME_APIKEY;                  // My API key @ thingspeak.com
const unsigned long ChanNum  = ME_CHANEL;                  // My channel numeber @ thingspeak.com

#define TXD2 21             // série capteur TX
#define RXD2 22             // série capteur RX
#define BOUTON_CAL 35
#define DEBOUNCE_TIME 1000
#define MESURE_TIME 5000    // 5 secondes avant la prochaine mesure
#define SENDING_TIME 300000 // 5 min avant le prochain envoi

// Wifi WPA2 Entreprise
#define EAP_IDENTITY EMAIL_PRO          //email pro identity
#define EAP_USERNAME ME_USER            //user name pro
#define EAP_PASSWORD ME_PWD             //your pro password
const char* ssidPRO     = SSID_PRO;
const char* proxy_name  = PROXY1_PRO;
const int proxy_port    = PROXY_PORT_PRO;
const char* ntp_pro     = NTP_PRO;

// for eduroam connect (?)
const static char* test_root_ca PROGMEM = \
"-----BEGIN CERTIFICATE-----\n"
"MIID3DCCA0WgAwIBAgIJANe5ZSCKoB8fMA0GCSqGSIb3DQEBCwUAMIGTMQswCQYD\n"
"VQQGEwJGUjEPMA0GA1UECAwGUmFkaXVzMRIwEAYDVQQHDAlTb21ld2hlcmUxFTAT\n"
"BgNVBAoMDEV4YW1wbGUgSW5jLjEgMB4GCSqGSIb3DQEJARYRYWRtaW5AZXhhbXBs\n"
"ZS5jb20xJjAkBgNVBAMMHUV4YW1wbGUgQ2VydGlmaWNhdGUgQXV0aG9yaXR5MB4X\n"
"DTE2MTEyMzAyNTUwN1oXDTE3MDEyMjAyNTUwN1owgZMxCzAJBgNVBAYTAkZSMQ8w\n"
"DQYDVQQIDAZSYWRpdXMxEjAQBgNVBAcMCVNvbWV3aGVyZTEVMBMGA1UECgwMRXhh\n"
"bXBsZSBJbmMuMSAwHgYJKoZIhvcNAQkBFhFhZG1pbkBleGFtcGxlLmNvbTEmMCQG\n"
"A1UEAwwdRXhhbXBsZSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHkwgZ8wDQYJKoZIhvcN\n"
"AQEBBQADgY0AMIGJAoGBAL03y7N2GvNDO9BN8fVtdNonp0bMiqpj1D0He5+OTM+9\n"
"3ZTIsJCNrbzhLQrRI3vMW7UDy8U7GeWORN9W4dWYlYiy/NFRp3hNMrbePhVmNIOV\n"
"ww4ovGzbD+Xo31gPVkhzQ8I5/jbOIQBmgKMAMZyOMlG9VD6yMmAeYqnZYz68WHKt\n"
"AgMBAAGjggE0MIIBMDAdBgNVHQ4EFgQUf1MLQIzAEZcRsgZlS8sosfmVI+UwgcgG\n"
"A1UdIwSBwDCBvYAUf1MLQIzAEZcRsgZlS8sosfmVI+WhgZmkgZYwgZMxCzAJBgNV\n"
"BAYTAkZSMQ8wDQYDVQQIDAZSYWRpdXMxEjAQBgNVBAcMCVNvbWV3aGVyZTEVMBMG\n"
"A1UECgwMRXhhbXBsZSBJbmMuMSAwHgYJKoZIhvcNAQkBFhFhZG1pbkBleGFtcGxl\n"
"LmNvbTEmMCQGA1UEAwwdRXhhbXBsZSBDZXJ0aWZpY2F0ZSBBdXRob3JpdHmCCQDX\n"
"uWUgiqAfHzAMBgNVHRMEBTADAQH/MDYGA1UdHwQvMC0wK6ApoCeGJWh0dHA6Ly93\n"
"d3cuZXhhbXBsZS5jb20vZXhhbXBsZV9jYS5jcmwwDQYJKoZIhvcNAQELBQADgYEA\n"
"GepHc7TE/P+5t/cZPn5TTQkWQ/4/1lgQd82lF36RYWSIW3BdAc0zwYWYZaWixxyp\n"
"s0YOqwz6PZAGRV+SlYO2f8Kf+C3aZs4YHB0GsmksmFOb8r9d7xcDuOKHoA+QV0Zw\n"
"RaK6pttsBAxy7rw3kX/CgTp0Y2puaLdMXv/v9FisCP8=\n"
"-----END CERTIFICATE-----";

const char * client_crt = "Certificate:\n"
"    Data:\n"
"        Version: 3 (0x2)\n"
"        Serial Number: 44 (0x2c)\n"
"    Signature Algorithm: sha1WithRSAEncryption\n"
"        Issuer: C=FR, ST=Radius, L=Somewhere, O=Example Inc./emailAddress=admin@example.com, CN=Example Certificate Authority\n"
"        Validity\n"
"            Not Before: Nov 23 02:55:07 2016 GMT\n"
"            Not After : Jan 22 02:55:07 2017 GMT\n"
"        Subject: C=FR, ST=Radius, O=Example Inc., CN=user@example.com/emailAddress=user@example.com\n"
"        Subject Public Key Info:\n"
"            Public Key Algorithm: rsaEncryption\n"
"                Public-Key: (2048 bit)\n"
"                Modulus:\n"
"                    00:ac:41:d4:a2:46:0c:dc:67:1d:7b:89:36:7c:15:\n"
"                    be:a2:c1:fe:4c:f2:fa:af:5d:76:0e:ee:b5:ca:d4:\n"
"                    d3:01:c8:6b:30:50:df:2d:57:17:f4:43:47:97:ca:\n"
"                    f1:8d:f7:c0:9d:56:b3:e7:17:7c:58:59:de:f3:be:\n"
"                    b5:08:5d:f8:3a:ad:83:44:0d:31:c9:f1:3d:f1:9a:\n"
"                    cf:84:0c:ad:d3:be:5c:bd:3d:58:b5:1d:2c:fe:70:\n"
"                    8d:c5:b0:17:87:d4:8e:85:f7:51:4c:0f:d1:e0:8c:\n"
"                    7b:a0:25:ab:91:7c:7f:eb:47:73:c9:4b:6c:8b:e6:\n"
"                    c1:06:d5:94:30:63:ec:45:1a:f5:7f:46:2f:b3:84:\n"
"                    78:5d:1c:37:1a:fa:57:ea:45:5e:45:40:ab:14:c7:\n"
"                    81:b0:26:3d:7e:cf:da:db:f0:f1:40:a7:a1:4b:54:\n"
"                    f3:96:1b:c9:30:3c:3c:d8:19:ba:c7:df:b1:ad:a2:\n"
"                    d6:17:0a:d6:ed:31:b5:cb:12:39:f5:6e:92:6b:85:\n"
"                    f2:9e:c7:06:6b:bb:89:ed:a7:5f:ec:56:12:46:fd:\n"
"                    3a:74:d1:d2:31:30:1d:58:19:25:33:ff:11:ea:3a:\n"
"                    52:33:b1:fb:d3:75:8d:1f:5e:36:a5:35:e0:11:5a:\n"
"                    4a:2d:97:58:2c:3d:62:3c:32:af:83:69:a9:1a:32:\n"
"                    1b:b7\n"
"                Exponent: 65537 (0x10001)\n"
"        X509v3 extensions:\n"
"            X509v3 Extended Key Usage: \n"
"                TLS Web Client Authentication\n"
"            X509v3 CRL Distribution Points: \n"
"\n"
"                Full Name:\n"
"                  URI:http://www.example.com/example_ca.crl\n"
"\n"
"    Signature Algorithm: sha1WithRSAEncryption\n"
"         8b:8d:b6:19:ce:6f:6b:9e:1d:03:8b:6b:10:fc:99:d0:7a:2f:\n"
"         e0:37:ce:b8:a4:e4:b9:a1:c2:36:ff:76:b2:ad:d7:d0:df:d1:\n"
"         03:27:93:a7:4e:1e:bf:ed:d2:b7:65:2a:c9:c3:ab:20:aa:e3:\n"
"         10:4c:75:3b:c4:02:ab:34:08:6e:61:91:cf:e3:02:35:6a:e5:\n"
"         f3:25:96:51:92:82:6e:52:81:c1:f1:7b:68:02:b0:ce:f4:ba:\n"
"         fd:6e:68:35:b3:7e:77:cb:a0:1e:11:5e:58:bf:f3:2a:ed:b3:\n"
"         4c:82:21:5e:1b:47:b6:2f:f3:f5:c9:1b:6a:70:44:6d:ff:ad:\n"
"         a6:e3\n"
"-----BEGIN CERTIFICATE-----\n"
"MIIDTjCCAregAwIBAgIBLDANBgkqhkiG9w0BAQUFADCBkzELMAkGA1UEBhMCRlIx\n"
"DzANBgNVBAgMBlJhZGl1czESMBAGA1UEBwwJU29tZXdoZXJlMRUwEwYDVQQKDAxF\n"
"eGFtcGxlIEluYy4xIDAeBgkqhkiG9w0BCQEWEWFkbWluQGV4YW1wbGUuY29tMSYw\n"
"JAYDVQQDDB1FeGFtcGxlIENlcnRpZmljYXRlIEF1dGhvcml0eTAeFw0xNjExMjMw\n"
"MjU1MDdaFw0xNzAxMjIwMjU1MDdaMHExCzAJBgNVBAYTAkZSMQ8wDQYDVQQIDAZS\n"
"YWRpdXMxFTATBgNVBAoMDEV4YW1wbGUgSW5jLjEZMBcGA1UEAwwQdXNlckBleGFt\n"
"cGxlLmNvbTEfMB0GCSqGSIb3DQEJARYQdXNlckBleGFtcGxlLmNvbTCCASIwDQYJ\n"
"KoZIhvcNAQEBBQADggEPADCCAQoCggEBAKxB1KJGDNxnHXuJNnwVvqLB/kzy+q9d\n"
"dg7utcrU0wHIazBQ3y1XF/RDR5fK8Y33wJ1Ws+cXfFhZ3vO+tQhd+Dqtg0QNMcnx\n"
"PfGaz4QMrdO+XL09WLUdLP5wjcWwF4fUjoX3UUwP0eCMe6Alq5F8f+tHc8lLbIvm\n"
"wQbVlDBj7EUa9X9GL7OEeF0cNxr6V+pFXkVAqxTHgbAmPX7P2tvw8UCnoUtU85Yb\n"
"yTA8PNgZusffsa2i1hcK1u0xtcsSOfVukmuF8p7HBmu7ie2nX+xWEkb9OnTR0jEw\n"
"HVgZJTP/Eeo6UjOx+9N1jR9eNqU14BFaSi2XWCw9Yjwyr4NpqRoyG7cCAwEAAaNP\n"
"ME0wEwYDVR0lBAwwCgYIKwYBBQUHAwIwNgYDVR0fBC8wLTAroCmgJ4YlaHR0cDov\n"
"L3d3dy5leGFtcGxlLmNvbS9leGFtcGxlX2NhLmNybDANBgkqhkiG9w0BAQUFAAOB\n"
"gQCLjbYZzm9rnh0Di2sQ/JnQei/gN864pOS5ocI2/3ayrdfQ39EDJ5OnTh6/7dK3\n"
"ZSrJw6sgquMQTHU7xAKrNAhuYZHP4wI1auXzJZZRkoJuUoHB8XtoArDO9Lr9bmg1\n"
"s353y6AeEV5Yv/Mq7bNMgiFeG0e2L/P1yRtqcERt/62m4w==\n"
"-----END CERTIFICATE-----";

const char * client_key = "-----BEGIN RSA PRIVATE KEY-----\n"
"MIIEpgIBAAKCAQEArEHUokYM3Gcde4k2fBW+osH+TPL6r112Du61ytTTAchrMFDf\n"
"LVcX9ENHl8rxjffAnVaz5xd8WFne8761CF34Oq2DRA0xyfE98ZrPhAyt075cvT1Y\n"
"tR0s/nCNxbAXh9SOhfdRTA/R4Ix7oCWrkXx/60dzyUtsi+bBBtWUMGPsRRr1f0Yv\n"
"s4R4XRw3GvpX6kVeRUCrFMeBsCY9fs/a2/DxQKehS1TzlhvJMDw82Bm6x9+xraLW\n"
"FwrW7TG1yxI59W6Sa4XynscGa7uJ7adf7FYSRv06dNHSMTAdWBklM/8R6jpSM7H7\n"
"03WNH142pTXgEVpKLZdYLD1iPDKvg2mpGjIbtwIDAQABAoIBAQCMhO9GqUpYia2d\n"
"VyOhOcPX1dTzRMuHPwDN0aFvIwo2zB3UvkQxInkiA7hldWJz44W3VEFR5PDEyht8\n"
"Tzgy6SVUCLOqUfEpwag8bYOXPxiWQRY6Mc8pf/FyZrLgb3PilFznoAcru0QEn9VB\n"
"oTlCZ4OalSE5NlQIFGemgZhvmTPmcm4OwPW2diBjLtb3AA8eaaw8okWZwr8g4Bcd\n"
"el5KX6pZpDRpGQueh3iKaKxYWbxLYK+c30gKWD65tsAqKyVg2Tm1R2c+kFXgizZt\n"
"EexD95SGMjSkGg3R05sKv6m71iJhlOzVQ4ZCKm18Kqa7wZuZ4SIehVmKIV0gaupz\n"
"gjyr7+NBAoGBAOGjjGI3nxJTZY3O+KeaQo/jqrKowqZxzMOsCgAvW56xDuAvx9TJ\n"
"m4428NGubMl/0RwX6TnxJDm6oe+tnOxLIgE/VnsQLiNzQuFJxrs5JYctdGc4uvk2\n"
"KuXDr7tPEYlU/7OLRReov9emydIXJnsGejkIPllUj+DGNjNFqtXh2VoHAoGBAMNv\n"
"eSgJSkcM6AUaDuUKaXBL2nkKHNoTtRQ0eCEUds6arKyMo0mSP753FNEuOWToVz1O\n"
"oaddSFw81J9t+Xd6XSRbhMj63bQ9nvFKBA1lJfLu+xe3ts0f+vmp1PguOuUHsgNP\n"
"aAm/gLPSKUpBO46NG6KhUrZ2ej6AEg7SuGXrDITRAoGBAKK7s6m6d81dvGZ0GT23\n"
"sb3Y8ul7cTdd59JPp77OaQOgqxvhGfxLkxcUZMa1R9xjhMsAK8MQOZIxGk2kJwL8\n"
"hP/lUFfdKYmDvX6CGQQ6iOhfTg6MCb1m5bVkVr9+nSUw2mIBVclkeUftEK2m6Kfd\n"
"2hR774u5wzLXgYuk+TrcckfNAoGBAJ9X8hacjH0lnr8aIe7I8HLoxbZOcnuz+b4B\n"
"kbiW8M8++W6uNCw2G9b1THnJEG6fqRGJXPASdH8P8eQTTIUHtY2BOOCM+dqNK1xc\n"
"FrW9NJXAF+WcmmTgoEaTG9tGBirafV+JjK/1/b+fqJ6sVRzDHDcbBU9ThhQTY6XG\n"
"VSZz4H8hAoGBAMeQQjiUlKBnpGt1oTgKDZo58b7ui61yftg+dEAwIKs6eb5X20vZ\n"
"Ca4v/zg06k9lKTzyspQjJZuzpMjFUvDK4ReamEvmwQTIc+oYVJm9Af1HUytzrHJH\n"
"u0/dDt0eYpZpzrFqxlP+0oXxlegD8REMVvwNCy+4isyCvjogDaYRfJqi\n"
"-----END RSA PRIVATE KEY-----";
     
// Wifi résidentiel
const char* ssid1     = SSID_RES1;
const char* password1 = PWD_RES1;
const char* ssid2     = SSID_RES2;
const char* password2 = PWD_RES2;
const char* ssid3     = SSID_RES3;
const char* password3 = PWD_RES3;
const char* ssid4     = SSID_RES4;
const char* password4 = PWD_RES4;

// certificat HTTPS pour envoi à thingspeak
// DigiCert High Assurance EV Root CA
const char* rootCACertificate =
  "-----BEGIN CERTIFICATE-----\n"
  "MIIDxTCCAq2gAwIBAgIQAqxcJmoLQJuPC3nyrkYldzANBgkqhkiG9w0BAQUFADBs\n"
  "MQswCQYDVQQGEwJVUzEVMBMGA1UEChMMRGlnaUNlcnQgSW5jMRkwFwYDVQQLExB3\n"
  "d3cuZGlnaWNlcnQuY29tMSswKQYDVQQDEyJEaWdpQ2VydCBIaWdoIEFzc3VyYW5j\n"
  "ZSBFViBSb290IENBMB4XDTA2MTExMDAwMDAwMFoXDTMxMTExMDAwMDAwMFowbDEL\n"
  "MAkGA1UEBhMCVVMxFTATBgNVBAoTDERpZ2lDZXJ0IEluYzEZMBcGA1UECxMQd3d3\n"
  "LmRpZ2ljZXJ0LmNvbTErMCkGA1UEAxMiRGlnaUNlcnQgSGlnaCBBc3N1cmFuY2Ug\n"
  "RVYgUm9vdCBDQTCCASIwDQYJKoZIhvcNAQEBBQADggEPADCCAQoCggEBAMbM5XPm\n"
  "+9S75S0tMqbf5YE/yc0lSbZxKsPVlDRnogocsF9ppkCxxLeyj9CYpKlBWTrT3JTW\n"
  "PNt0OKRKzE0lgvdKpVMSOO7zSW1xkX5jtqumX8OkhPhPYlG++MXs2ziS4wblCJEM\n"
  "xChBVfvLWokVfnHoNb9Ncgk9vjo4UFt3MRuNs8ckRZqnrG0AFFoEt7oT61EKmEFB\n"
  "Ik5lYYeBQVCmeVyJ3hlKV9Uu5l0cUyx+mM0aBhakaHPQNAQTXKFx01p8VdteZOE3\n"
  "hzBWBOURtCmAEvF5OYiiAhF8J2a3iLd48soKqDirCmTCv2ZdlYTBoSUeh10aUAsg\n"
  "EsxBu24LUTi4S8sCAwEAAaNjMGEwDgYDVR0PAQH/BAQDAgGGMA8GA1UdEwEB/wQF\n"
  "MAMBAf8wHQYDVR0OBBYEFLE+w2kD+L9HAdSYJhoIAu9jZCvDMB8GA1UdIwQYMBaA\n"
  "FLE+w2kD+L9HAdSYJhoIAu9jZCvDMA0GCSqGSIb3DQEBBQUAA4IBAQAcGgaX3Nec\n"
  "nzyIZgYIVyHbIUf4KmeqvxgydkAQV8GK83rZEWWONfqe/EW1ntlMMUu4kehDLI6z\n"
  "eM7b41N5cdblIZQB2lWHmiRk9opmzN6cN82oNLFpmyPInngiK3BD41VHMWEZ71jF\n"
  "hS9OMPagMRYjyOfiZRYzy78aG6A9+MpeizGLYAiJLQwGXFK3xPkKmNEVX58Svnw2\n"
  "Yzi9RKR/5CYrCsSXaQ3pjOLAEFe4yHYSkVXySGnYvCoCWw9E1CAx2/S6cCZdkGCe\n"
  "vEsXCS+0yx5DaMkHJ8HSXPfqIbloEpw8nL+e/IBcm2PN7EeqJSdnoDfzAIJ9VNep\n"
  "+OkuE6N36B9K\n"
  "-----END CERTIFICATE-----";
  
// variables globales du programme pricipal
unsigned long lastTime = 0;
unsigned long timerDelay = 10000;  // valeur initiale de 10s (puis SENDING_TIME)
bool WifiAvailable = false;        // Wifi dipo ou offline mode
bool ARsend = true;                // accusé de réception si envoi OK
bool Pro = false;                  // mode wifi pro entreprise ou residenteil
int ARsendNum = 0;                 // nombre d'envois ratés


// gestion de l'horloge pour la validation des certificats HTTPS
void setClock() {
  configTime(0, 0, "pool.ntp.org" , ntp_pro , "time.nist.gov" );
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
  Serial.print(F("Current time : "));
  Serial.println(asctime(&timeinfo));
}

/**************************************************
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

// ## nouvelles fonctions by nOfOkUz ##

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
      if ( !WifiAvailable ) { // Pas de WiFi mode offline
          Serial.println(F("Off line"));
          return;
          }
      if ( !ARsend ) {
          ARsendNum++ ;
          Serial.print(F("Valeur précédente non envoyée : ") );
          if ( ARsendNum >3 ) { // rebot après 3 essais ratés
             Serial.println(F(" ... reboot dans 5s...") );
             delay(5000);
             ESP.restart();
          }
          else {
             Serial.println(String( ARsendNum )+ F(" fois.") );
          }
      }
      if ( Pro ) { // Envoi en WiFi pro entreprise avec wpa2 et par un proxy
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
             if ( !ARsend ) {
                  Serial.println(F("... reboot dans 5s..."));
                  delay(5000);
                  ESP.restart();
                  }
             WiFi.disconnect();
             WiFi.begin();
             Serial.println(F("... reconnexion au WiFi!"));
         }
      return;
      }
      if( !Pro ) { // envoi par Wifi résidentiel
          if ((wifiMulti.run() == WL_CONNECTED)) {
              WiFiClientSecure client;
              client.setCACert(rootCACertificate);
              // with ThinkSpeak library
              Serial.print(F("### Connexion web à ThingSpeak via library "));            
              ThingSpeak.begin(client);  
              int result = ThingSpeak.writeField( ChanNum , FieldNum , long(CO2_send) , apiKey );
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
              // without ThinkSpeak library (need to remove previous and return          
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
              Serial.println(F("######################### fin fonction envoi ##########################"));
              lastTime = millis();
              timerDelay = SENDING_TIME; // attendra SENDING_TIME ms avant le prochain envoi
              ARsend = true ;
              ARsendNum = 0 ;
              }
          }
          else {
              Serial.println(F("Valeur non envoyée"));
              Serial.print(F("Plus connecté au WiFi "));
              if ( !ARsend ) { // reboot si non reconnexion au WiFi 2 fois de suite
                  Serial.println(F("... reboot dans 5s..."));
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

void setup() {
  // bouton de calibration
  pinMode(BOUTON_CAL, INPUT);

  // ports série de debug et de communication capteur
  Serial.begin(115200);
  Serial1.begin(9600, SERIAL_8N1, RXD2, TXD2);

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
// for ESP32 cards v2.0.2
           error = esp_wifi_sta_wpa2_ent_enable();                  
           if( error != ESP_OK ){
              ESP_LOGV(TAG, "esp_wifi_sta_wpa2_ent_enable failed! %d", err);
              Serial.println(F("Failed to enable WPA2"));
              } else { Serial.println(F("ESP_OK WPA2 enable"));}

// useless part for me kept for memory
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
    Serial.println();  // rend plus lisible le debug
    Envoi_Valeur(CO2);  
    }

  ancienCO2 = CO2;
  Serial.println();   // rend plus lisible le debug
  delay(MESURE_TIME); // attend MESURE_TIME ms avant la prochaine mesure
}
