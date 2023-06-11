/***********************************************
  ### fonctions affichage originale Rinolfi ###
 ***********************************************/

//
// nettoie l'écran et affiche les infos utiles version Rinolfi
//
void prepareEcranOrig() {
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

//
//   Affichage de l'écran Rinolfi
//
void afficheEcranOrig(unsigned long CO2) {

  int seuil = 0;
  // efface le chiffre du texte
  if (CO2 != ancienCO2) {
    tft.fillRect(0, 0, tft.width(), 60, TFT_BLACK);
  }

  if ( CO2 < 800 ) {
    tft.setTextColor(TFT_GREEN, TFT_BLACK);
    if ( seuil != 1 ) {
      tft.setTextSize(2);
      tft.fillRect(0, 69, tft.width(), 16, TFT_BLACK);
      tft.drawString("Air Excellent", tft.width() / 2, tft.height() / 2 + 10);
    }
    seuil = 1;
  } else if ( CO2 >= 800 && CO2 < 1000) {
    tft.setTextColor(TFT_ORANGE, TFT_BLACK);
    if ( seuil != 2 ) {
      tft.setTextSize(2);
      tft.fillRect(0, 69, tft.width(), 16, TFT_BLACK);
      tft.drawString("Air Moyen", tft.width() / 2, tft.height() / 2 + 10);
    }
    seuil = 2;
  } else if (CO2 >= 1000 && CO2 < 1500) {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    if ( seuil != 3 ) {
      tft.setTextSize(2);
      tft.fillRect(0, 69, tft.width(), 16, TFT_BLACK);
      tft.drawString("Air Mediocre", tft.width() / 2, tft.height() / 2 + 10);
    }
    seuil = 3;
  } else {
    tft.setTextColor(TFT_RED, TFT_BLACK);
    if ( seuil != 4 ) {
      tft.setTextSize(2);
      tft.fillRect(0, 69, tft.width(), 16, TFT_BLACK);
      tft.drawString("Air Vicie", tft.width() / 2, tft.height() / 2 + 10);
    }
    seuil = 4;
  }

  tft.setTextSize(8);
  tft.drawString(String(CO2), tft.width() / 2, tft.height() / 2 - 30);
  
}
