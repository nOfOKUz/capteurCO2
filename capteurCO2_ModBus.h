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

void IRAM_ATTR etalonnage() {
  if ( millis() - DEBOUNCE_TIME  >= DebounceTimer ) {
    DebounceTimer = millis();
    Serial.println(F("##############"));
    Serial.println(F("Bouton down !!"));
    Serial.println(F("##############"));
    demandeEtalonnage = true;
  }
}
