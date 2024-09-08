/*
  BlinkSerialReadAmps

  Platform ESP32 by Espressif 2.0.17

  Board: ESP32 Dev Module (variants/esp32/pins_arduino.h)

  JSY-MK-194 on pins 26/27
  Triac RobotDyn AC Module Dimmer HL

  inspiration https://www.aranacorp.com/fr/utilisation-dun-variateur-de-tension-ac-avec-esp32/

*/
/**
Erreur
c:\Users\flav\Documents\Arduino\libraries\RBDDimmer\src/esp32/RBDmcuESP32.h:10:10: fatal error: esp_intr.h: No such file or directory
   10 | #include "esp_intr.h"
  */
#include <HardwareSerial.h>
#include <RBDdimmer.h> // https://github.com/flav1972/RBDDimmer/tree/1.1
#include <Wire.h>

// fonction de scan I2C definie plus bas
void scanI2C();

/*********************************/
/* Configuration pour JSY-MK-194 */
/*********************************/
//Port Serie 2 - Remplace Serial2 qui bug
HardwareSerial MySerial(2);

// pour lecture du JSY-MK-194 (amperemetre)
#define RXD2 16  //Pour Routeur Linky ou UxIx2 (sur carte ESP32 simple): Couple RXD2=26 et TXD2=27 . Pour carte ESP32 4 relais : Couple RXD2=17 et TXD2=27
#define TXD2 17

#define SER_BUF_SIZE 4096
//Parameters for JSY-MK-194T module
// buffer pour le retour du JSY
byte ByteArray[130];

// Vitesse
int speed[] = {4800, 9600, 19600, 38400};
int speed_size = 4;
int speed_n = 0;

// Commandes pour changement de Vitesse
int vitesse_souhaitee = 38400;
byte msg[] = {0x00,0x10,0x00,0x04,0x00,0x01,0x02,0x01,0x08,0xAA,0x12}; // passage du port TTL en 38400
byte msg_ok[] = {0x01, 0x10, 0x00, 0x04, 0x00, 0x01, 0x40, 0x08 };

// TX[11]:
// 00 10 00 04 00 01 02 01 05 6B D7 pour passer à 4800
// 00 10 00 04 00 01 02 01 06 2B D6 pour passer à 9600
// 00 10 00 04 00 01 02 01 07 EA 16 pour passer à 19600
// 00 10 00 04 00 01 02 01 08 AA 12 pour passer à 38400
// reponse attendue
// RX[8]:01 10 00 04 00 01 40 08 
// valeurs obtenues avec l'executable dans JSY-MK-194T-test-software1.rar (sur le site du fabricant: https://www.jsypowermeter.com/download/)


long LesDatas[14];
int Sens_1, Sens_2;

int JSY; // 0 = erreur, sinon vitesse definie pour le JSY

// Fonctions
int Change_JSY_Speed();
void Setup_JSY(int speed);
void ResetEnergie_JSY();
void Lecture_JSY();

int nb_lectures = 0;

// Configuration Triac Dimmer
//Parameters
const int zeroCrossPin  = 35;
const int dimmer1Pin  = 25;
const int dimmer2Pin  = 26;
int MIN_POWER  = 0;
int MAX_POWER  = 100;
int POWER_STEP  = 2;

//Variables
int power  = 0;

//Objects
dimmerLamp dimmer1(dimmer1Pin,zeroCrossPin);
dimmerLamp dimmer2(dimmer2Pin,zeroCrossPin);
void testDimmer(dimmerLamp &);

// the setup routine runs once when you press reset:
void setup() {
  // initialize serial communication at 115200 bits per second:
  Serial.begin(115200);
  delay(1000);
  Serial.println("Verification Montage Routeur");
  
  // initialisation bus I2C
  Wire.begin();

  Serial.println("Recherche JSY-MK-194");
  // Changement vitesse Amperemetre
  JSY = Change_JSY_Speed();                          // retour == 0 si erreur
  // initialisation communication avec amperemetre
  if(JSY) {
    Setup_JSY(JSY);
    delay(1000);
    ResetEnergie_JSY();
    Serial.print("JSY-MK-194 configure a la vitesse de : ");
    Serial.print(JSY);
    Serial.println(" bauds");
  }
  else {
    Serial.println("Pas de JSY-MK-194 trouve");
  }
  Serial.println("Pause 10s");
  delay(10000); // Pause de 10 secondes pour qu'on puisse lire

  // initialisation Dimmer
  dimmer1.begin(NORMAL_MODE, ON);
  dimmer1.setPower(0);
  dimmer2.setMode(NORMAL_MODE);
  dimmer2.setState(ON);
  dimmer2.setPower(0);

  // test du Dimmer
  Serial.println("Test Triac 1");
  testDimmer(dimmer1);
  Serial.println("Test Triac 2 et 3");
  testDimmer(dimmer2);

  // puis on passe à 50% & 25%
  dimmer1.setPower(50);
  dimmer2.setPower(25);
}

// the loop routine runs over and over again forever:
void loop() {
  Serial.print("nb = ");
  Serial.println(nb_lectures);

  if(JSY) {
    Lecture_JSY();
  }
  else {
    Serial.println("Pas de JSY trouvé au demarrage...");
  }
  nb_lectures++;           // on incremente environ toutes les 5 secondes
  if(nb_lectures >= 152 ) { //150 environ 10 minutes
    nb_lectures = 0;
    if(JSY) ResetEnergie_JSY();
  }
  Serial.println();
  scanI2C();
  Serial.println("Pause 5s");
  delay(5000);
}

// *******************************
// * Source de Mesures UI Double *
// *      Capteur JSY-MK-194     *
// *******************************

void Setup_JSY(int speed) {
  MySerial.setRxBufferSize(SER_BUF_SIZE);
  MySerial.begin(speed, SERIAL_8N1, RXD2, TXD2);  //PORT DE CONNEXION AVEC LE CAPTEUR JSY-MK-194
}

int Change_JSY_Speed() {
  MySerial.setRxBufferSize(SER_BUF_SIZE);
  for(speed_n = 0; speed_n < speed_size; speed_n++) {
    Serial.print("\nConnexion a la vitesse: ");
    Serial.println(speed[speed_n]);
    MySerial.begin(speed[speed_n], SERIAL_8N1, RXD2, TXD2); //PORT DE CONNEXION AVEC LE CAPTEUR JSY-MK-194

    delay(200);
      
    int i;
    int len=11; 

    //  envoie la requête pour passer le port à vitesse_souhaitee //
    for(i = 0 ; i < len ; i++) {
      MySerial.write(msg[i]);         
    }
  
    // petit delay pour attendre la reponse (seulement 4800bauds)
    Serial.println("attente reponse 1 seconde");
    delay(1000);
    
    int a = 0;
    while (MySerial.available()) {
      ByteArray[a] = MySerial.read();
      a++;
    }
    Serial.print("nombre de donnes recues:");
    Serial.println(a);
    Serial.print("donnees recues: ");
    for (i = 0; i < a; i++ ) {
      Serial.print("0x");
      Serial.print(ByteArray[i] < 16 ? "0" : "");
      Serial.print(ByteArray[i], HEX);
      Serial.print(" ");
    }
    if(a>0)
      Serial.println();
    // comparaison avec la reponse attendue
    if(a==8) {
      if(memcmp(ByteArray, msg_ok,8) == 0) {
        Serial.println("Changement OK\nPause 5 secondes...");
        delay(5000);
        return(vitesse_souhaitee);
      }
    }
    Serial.println("Attente 5 seconde pour qu'on puisse lire...");
    delay(5000);
  }  
  Serial.println("Communication erreur avec JSY");
  return(0);
}

void ResetEnergie_JSY() {
  /* 
  * doc du JSY-MK-194
    Clear two channels of power at the same time:
    Send data[13]: 01 10 00 0C 00 02 04 00 00 00 00 F3 FA
    Receive data[8]: 01 10 00 0C 00 02 81 CB
  */
  int i, j;
  byte msg_send[] = { 0x01, 0x10, 0x00, 0x0C, 0x00, 0x02, 0x04, 0x00, 0x00, 0x00, 0x00, 0xF3, 0xFA };
  byte msg_ok[] = {0x01,0x10,0x00,0x0C,0x00,0x02,0x81,0xCB };

  // vide le buffer le lecture
  while (MySerial.available()) {
    MySerial.read();
    delay(10);
  }
  // Envoi sur Serial port 2 (Modbus RTU)
  Serial.println("envoi requette reset");
  for (i = 0; i < 13; i++) {
    MySerial.write(msg_send[i]);
  }

  // petit delay pour attendre la reponse (seulement 4800bauds)
  delay(200);
  Serial.println("attente reponse reset");

  int a = 0;
  while (MySerial.available()) {
    ByteArray[a] = MySerial.read();
    a++;
  }
  Serial.print("donnes reset recues:");
  Serial.println(a);
  for (i = 0; i < a; i++ ) {
    Serial.print("0x");
    Serial.print(ByteArray[i] < 16 ? "0" : "");
    Serial.print(ByteArray[i], HEX);
    Serial.print(" ");
  }
  Serial.println();
  // comparaison avec la reponse attendue
  if(a==8) {
    if(memcmp(ByteArray, msg_ok,8) == 0) {
      Serial.println("Reset OK");
    }
  }
}

void Lecture_JSY() {  //Ecriture et Lecture port série du JSY-MK-194  .

  int i, j;
  byte msg_send[] = { 0x01, 0x03, 0x00, 0x48, 0x00, 0x0E, 0x44, 0x18 };
  // Demande Info sur le Serial port 2 (Modbus RTU)
  Serial.println("envoi requette get data");
  for (i = 0; i < 8; i++) {
    MySerial.write(msg_send[i]);
  }

  //Réponse en général à l'appel précédent (seulement 4800bauds)
  Serial.println("attente reponse get data");

  int a = 0;
  while (MySerial.available()) {
    ByteArray[a] = MySerial.read();
    a++;
  }
  Serial.print("donnes recues:");
  Serial.println(a);

  if (a == 61) {  //Message complet reçu
    j = 3;
    for (i = 0; i < 14; i++) {  // conversion séries de 4 octets en long
      LesDatas[i] = 0;
      LesDatas[i] += ByteArray[j] << 24;
      j += 1;
      LesDatas[i] += ByteArray[j] << 16;
      j += 1;
      LesDatas[i] += ByteArray[j] << 8;
      j += 1;
      LesDatas[i] += ByteArray[j];
      j += 1;
    }
    Sens_1 = ByteArray[27];  // Sens 1
    Sens_2 = ByteArray[28];

    //Données du canal 1 : celui sur la platine
    Serial.print("Canal 1 Tension V: ");
    Serial.println(LesDatas[0] * .0001);
    Serial.print("Canal 1 Curent A: ");
    Serial.println(LesDatas[1] * .0001);
    Serial.print("Canal 1 active power W: ");
    Serial.println(LesDatas[2] * .0001);
    Serial.print("Canal 1 positive active energy Wh: ");
    Serial.println(LesDatas[3] * .1);
    Serial.print("Canal 1 powerfactor: ");
    Serial.println(LesDatas[4] * .001);
    Serial.print("Canal 1 negative active energy Wh: ");
    Serial.println(LesDatas[5] * .1);
    Serial.print("Power direction channel 1, 0 positive, 1 negative: ");
    Serial.println(Sens_1);
    Serial.print("Power direction channel 2, 0 positive, 1 negative: ");
    Serial.println(Sens_2);
    Serial.print("Frequence Hz: ");
    Serial.println(LesDatas[7] * .01);
    
    // Données du canal 2 : celui sur le cable distant
    Serial.print("Canal 2 Tension V: ");
    Serial.println(LesDatas[8] * .0001);
    Serial.print("Canal 2 Curent A: ");
    Serial.println(LesDatas[9] * .0001);
    Serial.print("Canal 2 active power W: ");
    Serial.println(LesDatas[10] * .0001);
    Serial.print("Canal 2 positive active energy Wh: ");
    Serial.println(LesDatas[11] * .1);
    Serial.print("Canal 2 powerfactor: ");
    Serial.println(LesDatas[12] * .001);
    Serial.print("Canal 2 negative active energy Wh: ");
    Serial.println(LesDatas[13] * .1);
  }
}

// Test du Dimmer
void testDimmer(dimmerLamp &dimmer) {
  int i;
  Serial.println("Variation de 0 a 100% trois fois");
  for(i = 0; i < 3; i++) {
    ////Sweep light power to test dimmer
    for(power=MIN_POWER;power<=MAX_POWER;power+=POWER_STEP){
      dimmer.setPower(power); // setPower(0-100%);
      Serial.print('.');
      delay(100);
    }
    Serial.println();
    for(power=MAX_POWER;power>=MIN_POWER;power-=POWER_STEP){
      dimmer.setPower(power); // setPower(0-100%);
      Serial.print('.');
      delay(100);
    }
    Serial.println();
  }
  Serial.println("Forcage au MAX 5 secondes");
  dimmer.setMode(TOGGLE_MODE);
  dimmer.setState(ON);
  delay(5000);
  Serial.println("Forcage au MIN 5 secondes");
  dimmer.setState(OFF);
  delay(5000);
  Serial.println("Retour mode Normal");
  dimmer.setMode(NORMAL_MODE);
  dimmer.setState(ON);
  Serial.println("Dimmer test Ended");
}

void scanI2C()
{
  byte error, address;
  int nDevices;
  byte ecranOK = 0;
  byte rtcOK = 0;

  Serial.print("Scanning... Try ");
  nDevices = 0;
  for (address = 1; address < 127; address++) {
    // The i2c_scanner uses the return value of
    // the Write.endTransmisstion to see if
    // a device did acknowledge to the address.
    Wire.beginTransmission(address);
    error = Wire.endTransmission();

    if (error == 0) {
      Serial.print("I2C device found at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.print(address, HEX);
      Serial.println("  !");

      if (address == (byte)0x3c || address == (byte)0x3d) {
        Serial.println("  OLED adaptor ?");
        DisplayType(address);
        ecranOK = 1;
      }
      else if(address == (byte)0x57) {
        Serial.println("  EEPROM");
      }
      else if(address == (byte)0x68) {
        Serial.println("  Real-Time Clock (RTC): DS3231");
        rtcOK = 1;
      }
      nDevices++;
    } else if (error == 4) {
      Serial.print("Unknow error at address 0x");
      if (address < 16)
        Serial.print("0");
      Serial.println(address, HEX);
    }
  }
  if (nDevices == 0)
    Serial.println("No I2C devices found\n");
  else
    Serial.println("done I2C scan\n");
  if(ecranOK) {
    Serial.println("Ecran Trouvé : OK");
  } else {
    Serial.println("Erreur: pas d'écran");
  }
  if(rtcOK) {
    Serial.println("RTC Trouvé : OK");
  } else {
    Serial.println("Warning: pas de RTC");
  }
}


int DisplayType(int address) {
  byte buffer[0];

  //Serial.println("  check type");
  Wire.beginTransmission(address);
  Wire.write(0x00);
  Wire.endTransmission(false);
  uint8_t bytesReceived = Wire.requestFrom(address, 1);  // This register is 8 bits = 1 byte long
  //Serial.printf("  requestFrom: %u\n", bytesReceived);
  if (Wire.available() > 0) {
    Wire.readBytes(buffer, 1);
  }
  Wire.endTransmission();
  //Serial.println("   got response");
  buffer[0] &= 0x0f;        // mask off power on/off bit
  //Serial.print("  Read done: 0x");
  //Serial.println(buffer[0], HEX);
  Serial.print("  OLED Device may be: ");
  switch (buffer[0]) {
    case 0x7 || 0xf:  // SH1107
      Serial.println("SH1107");
      return 0;
    case 0x8:  // SH1106
      Serial.println("SH1106");
      return 1;
    case 0x6:  // SSD1306 6=128x64 display
      Serial.println("SSD1306 128x64");
      return 2;
    case 0x3:  // SSD1306 3=128x32 display
      Serial.println("SSD1306 128x32");
      return 3;
    default:
      Serial.print("UNKNOWN: 0x");
      Serial.println(buffer[0], HEX);
      return -1;  //Unknown
  }
}
