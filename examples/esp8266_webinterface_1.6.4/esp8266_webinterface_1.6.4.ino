
/*
  WS2812FX Webinterface.
  
  Harm Aldick - 2016
  www.aldick.org

  
  FEATURES
    * Webinterface with mode, color, speed and brightness selectors


  LICENSE

  The MIT License (MIT)

  Copyright (c) 2016  Harm Aldick 

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  
  CHANGELOG
  2016-11-26 initial version
  2018-01-06 added custom effects list option and auto-cycle feature
  
*/

/* Christophe Caron www.caron.ws
 * Sauvegarde dans EEPROM paramètres bandeau LED V1.1.0 19/01/2018
 * Sauvegarde dans EEPROM paramètres WIFI V1.2.0 19/01/2018
 * Sauvegarde dans EEPROM paramètres Réseau V1.2.1 21/01/2018
 * Ajout des information sur ESP01 V1.2.2 21/01/2018
 * Modification du client Web V1.3.0 21/01/2018
 * Utilisation de la bibliothèque FS pour le client web, ajout de jscolor dans la mémoire EEPROM V1.4.0 21/01/2018
 * Sauvegarde dans EEPROM paramètres nombre de led V1.5.0 21/01/2018
 * Mémorisation du marche/arrêt du bandeau LED V1.5.1 22/01/2018
 * Modification du client web V1.5.2 23/01/2018
 * Ajout gestion de la date et de l'heure avec serveur NTP V1.6.0 26/10/2018
 * Modification du client web V1.6.1 27/01/2018
 * Ajout de la fonction minuteur V1.6.2 28/01/2018
 * Ajout reset wifi et ping V1.6.3 29/01/2018
 * Ajout calendrier mise en marche et arrêt V1.6.4 18/02/2018
 */
#include <ESP8266WiFi.h>                    //Inclusion bibliothèque gestion du WIFI de l'ESP8266
#include <ESP8266WebServer.h>               //Inclusion bibliothèque gestion du serveur web de l'ESP8266
#include <WS2812FX_fr.h>                    //Inclusion bibliothèque gestion des LED WS2812
#include <EEPROM.h>                         //Inclusion bibliothèque gestion de l'EEPROM
#include <FS.h>                             //Inclusion bibliothèque SPIFFS
#include <WiFiUdp.h>                        //Inclusion bibliothèque pour la gestion de l'User Datagram Protocol
#include <NTPClient.h>                      //Inclusion bibliothèque gestion serveur NTP
#include <TimeLib.h>                        //Inclusion bibliothèque gestion des fonctionnalités de chronométrage
#include <TimeAlarms.h>                     //Inclusion bibliothèque gestion des fonctionnalités d'alarme et minuterie
#include <ESP8266Ping.h>                    //Inclusion bibliothèque pour le ping et modification de byte count = 1 dans la bibliothèque
#include <StringSplitter.h>                 //Inclusion bibliothèque pour création d'un tableau depuis chaine avec séparateur Modifier nbrs MAX 5 => 10


// QUICKFIX...See https://github.com/esp8266/Arduino/issues/263
#define min(a,b) ((a)<(b)?(a):(b))
#define max(a,b) ((a)>(b)?(a):(b))

#define LED_PIN 2                           // 0 = GPIO0, 2=GPIO2
#define LED_MAX 1000                        // Nombres de LED MAX 

#define WIFI_TIMEOUT 60000                  // Vérification du WIFI toute les xxxx ms.
#define WIFI_RESET 5                        // Reset du WIFI toute les xxxx minutes Valeur Max 255mn. Vauleur multiple de WIFI_TIMEOUT
#define HTTP_PORT 80                        // Port de connexion pour le serveur HTTP

#define ADRESS_LED_LUM 0                    // Adresse pour le stockage dans EEPROM de la Luminosité
#define ADRESS_LED_MOD 1                    // Adresse pour le stockage dans EEPROM du Mode
#define ADRESS_LED_VIT 2                    // Adresse pour le stockage dans EEPROM de la vitesse sur 2 mots (2 et 3)
#define ADRESS_LED_COL 4                    // Adresse pour le stockage dans EEPROM de la couleur sur 4 mots (4-7)

#define ADRESS_WIFI 8                       // Adresse de départ paramètres WIFI Mot de passe et SSID (Fin ADRESS_WIFI + 96 mots)8-103
#define ADRESS_RESEAU 104                   // Adresse de départ paramètres Reseau DHCP=0 Fixe=1, IP passerelle,Fin IP statique (Fin ADRESS_RESEAU + 6 mots)104-109
#define ADRESS_NLED 110                     // Adresse de départ paramètres nombres de LED (Fin ADRESS_NLED + 2 mots)110-111
#define ADRESS_ON_OFF 112                   // Adresse mémorisation Marche/Arrêt du bandeau LED 112
#define ADRESS_MINUTEUR 113                 // Adresse mémorisation Marche/Arrêt du bandeau LED 111 (Fin ADRESS_MINUTEUR + 2 mots)113-116

#define ADRESS_AL0 117                      // Adresse mémorisation paramètres alarme0 7 Bytes ID,Heure,Minute,On/Off, Jour/toujours,couleur,mode
#define ADRESS_AL1 124                      // Adresse mémorisation paramètres alarme1 7 Bytes ID,Heure,Minute,On/Off, Jour/toujours,couleur,mode 
#define ADRESS_AL2 131                      // Adresse mémorisation paramètres alarme2 7 Bytes ID,Heure,Minute,On/Off, Jour/toujours,couleur,mode 
#define ADRESS_AL3 138                      // Adresse mémorisation paramètres alarme3 7 Bytes ID,Heure,Minute,On/Off, Jour/toujours,couleur,mode
#define ADRESS_AL4 145                      // Adresse mémorisation paramètres alarme4 7 Bytes ID,Heure,Minute,On/Off, Jour/toujours,couleur,mode - Fin 151
#define ADRESS_ALB 7                        // Nombres de mots par alarme

String DateHeure = "";                      // Variable pour gestion de la date et de l'heure
String AffMinuteur = "";                    // Variable globale de la valeur du minuteur pour client web
int MinuteurStop=0;                         // Variable globale de la valeur du minuteur 
uint8_t AlarmeMin=dtINVALID_ALARM_ID;       // Identification de l'alarme minuteur 60 secondes dtINVALID_ALARM_ID= 255 dans bibliothèque
uint8_t Alarme[]={dtINVALID_ALARM_ID,dtINVALID_ALARM_ID,dtINVALID_ALARM_ID,dtINVALID_ALARM_ID,dtINVALID_ALARM_ID};       // Identification de l'alarme 0 à 4 (5 alarmes) valeur par defaut 255
uint8_t ReseauOut=0;                        // Variable Compteur Nombre de de perte du wifi
unsigned long auto_last_change = 0;
unsigned long last_wifi_check_time = 0;
String modes = "";                          // Variable pour la getsion des modes
uint8_t myModes[] = {};                     // *** optionally create a custom list of effect/mode numbers
boolean auto_cycle = false;

String getContentType(String filename);     // convert the file extension to the MIME type
bool handleFileRead(String path);           // send the right file to the client (if it exists)

WS2812FX ws2812fx = WS2812FX(1, LED_PIN, NEO_GRB + NEO_KHZ800);
ESP8266WebServer server(HTTP_PORT);
ADC_MODE(ADC_VCC);                        // utilisation de ESP.getVcc

WiFiUDP ntpUDP;
// You can specify the time server pool and the offset (in seconds, can be
// changed later with setTimeOffset() ). Additionaly you can specify the
// update interval (in milliseconds, can be changed using setUpdateInterval() ).
NTPClient timeClient(ntpUDP, "europe.pool.ntp.org", 3600, 60000);

void setup(){
  EEPROM.begin(512);                                //Initialise zone mémoire dans eeprom
  Serial.begin(115200);                             //Vitesse liaison série 115200
  Serial.println();                                 //Saut de ligne 
  Serial.println();                                 //Saut de ligne 
  Serial.println("**** Information ESP8266 ****");            
  Serial.print("Taille segment libre: "); Serial.print(ESP.getFreeHeap());Serial.println(" Octets");
  Serial.print("Version boot: "); Serial.println(ESP.getBootVersion());
  Serial.print("CPU: ");Serial.print(ESP.getCpuFreqMHz());Serial.println(" Mhz");
  Serial.print("Version SDK: "); Serial.println(ESP.getSdkVersion());
  Serial.print("ID de la puce: "); Serial.println(ESP.getChipId());
  Serial.print("ID de EEPROM: "); Serial.println(ESP.getFlashChipId());
  Serial.print("Taille de EEPROM: "); Serial.print(ESP.getFlashChipRealSize());Serial.println(" Octets");
  Serial.print("Alimentation Vcc: "); Serial.print((float)ESP.getVcc()/1024.0);Serial.println(" V");

  modes.reserve(5000);
  modes_setup();
  initLed();                                            // Initialisation du bandeau LED
  Serial.println("Démarrage...");
  Serial.println("Initialisation du Wifi");
  wifi_setup();                                         // Initialisation du wifi
  timeClient.begin();                                   // Démarrage du client NTP
  SPIFFS.begin();                                       // Start the SPI Flash Files System
  Serial.println("initialisation du serveur HTTP");
  server.onNotFound([]() {                              // If the client requests any URI
      if (!handleFileRead(server.uri()))                // send it if it exists
      server.send(404, "text/plain", "404: Not Found"); // otherwise, respond with a 404 (Not Found) error
      });
  server.on("/modes", srv_handle_modes);                // Serveur pour la liste des modes
  server.on("/set", srv_handle_set);                    // Serveur lecture des commandes depuis client
  server.on("/DateHeure", srv_handle_dateheure);        // Serveur pour affichage de la date et l'heure
  server.on("/Minuteur", srv_handle_minuteur);          // Serveur pour affichage tempo
  server.begin();
  Serial.println("Serveur HTTP démarré.");
  Serial.println("Serveur HTTP prêt");
  Date_Heure();                                       // initialisation de la date et de l'heure
  Serial.print("ID alarme Minuteur: ");
  AlarmeMin=(Alarm.timerRepeat(1, Timer1S));          // Timer toute les 60 secondes
  Serial.println(AlarmeMin);                          // ID de la minuterie
  Alarm.disable(AlarmeMin);                           // Arrêt de la minuterie
  AlarmInit();                                        // Initialisation des alarmes
 }
 
void loop() {
  unsigned long now = millis();
  
  conf_serie();                                        // Configuration via liaison série
  server.handleClient();
  ws2812fx.service();
  Date_Heure();
  Alarm.delay(10);
  if(now - last_wifi_check_time > WIFI_TIMEOUT) {
      Serial.print("Vérification du WiFi... ");
      if(WiFi.status() != WL_CONNECTED) {
          Serial.println("Erreur de connexion WIFI ...");
          Serial.println("Taper wifi pour modifier les paramètres du WIFI");
          if (ReseauOut>WIFI_RESET) raz(); 
          ReseauOut++;        
      } else {
          Serial.println("OK");
          Serial.printf("Etat du Wifi  %d\n",WiFi.isConnected());
          Serial.printf("Hostname est %s\n",WiFi.hostname().c_str());
          Serial.print("Addresse IP : "); Serial.println(WiFi.localIP());
          Serial.print("Passerelle IP: "); Serial.println(WiFi.gatewayIP());
          Serial.print("Masque sous réseau: "); Serial.println(WiFi.subnetMask());
          Serial.print("Adresse IP DNS: "); Serial.println(WiFi.dnsIP());
          if(Ping.ping(WiFi.gatewayIP())) {
                Serial.println("Ping Passerelle OK :)");
                ReseauOut=0;
          } else {
                Serial.println("Ping Passerelle erreur :(");
                if (ReseauOut>WIFI_RESET) raz();
                ReseauOut++;
          }
      }
  last_wifi_check_time = now;
  
  }

  if(auto_cycle && (now - auto_last_change > 10000)) { // cycle effect mode every 10 seconds
    uint8_t next_mode = (ws2812fx.getMode() + 1) % ws2812fx.getModeCount();
    if(sizeof(myModes) > 0) { // if custom list of modes exists
      for(uint8_t i=0; i < sizeof(myModes); i++) {
        if(myModes[i] == ws2812fx.getMode()) {
          next_mode = ((i + 1) < sizeof(myModes)) ? myModes[i + 1] : myModes[0];
          break;
        }
      }
    }
    ws2812fx.setMode(next_mode);
    Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
    auto_last_change = now;
  }

}

/*
 * Connect to WiFi. If no connection is made within WIFI_TIMEOUT, ESP gets resettet.
 */
void wifi_setup() {
  String WIFI_SSID;
  String WIFI_PASSWORD;
  WIFI_SSID=(LectureWifiEeprom(1));       // Lecture des paramètres WIFI SSID
  WIFI_PASSWORD=(LectureWifiEeprom(2));   // Lecture des paramètres WIFI Mot de passe
 
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(WIFI_SSID);
  WiFi.mode(WIFI_STA);
  if (EEPROM.read(ADRESS_RESEAU)==1){
      IPAddress ip(EEPROM.read(ADRESS_RESEAU+1),EEPROM.read(ADRESS_RESEAU+2),EEPROM.read(ADRESS_RESEAU+3),EEPROM.read(ADRESS_RESEAU+5));
      IPAddress gateway(EEPROM.read(ADRESS_RESEAU+1),EEPROM.read(ADRESS_RESEAU+2),EEPROM.read(ADRESS_RESEAU+3),EEPROM.read(ADRESS_RESEAU+4));
      IPAddress subnet(255,255,255,0);
      WiFi.config(ip, gateway, subnet, gateway); //WiFi.config(ip, gateway, subnet,dns) dns=gateway 
      }
  WiFi.begin(WIFI_SSID.c_str(), WIFI_PASSWORD.c_str());
  
  unsigned long connect_start = millis();
  while(WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");

    if(millis() - connect_start > WIFI_TIMEOUT) {
      Serial.println();
      Serial.println("Erreur de connexion WIFI. Vérifier vos paramètres WIFI");
      Serial.print("Dépassement du temps de connexion: ");
      Serial.println(WIFI_TIMEOUT);
      return;
    }
  }

  Serial.println("");
  Serial.println("WiFi est connecté");  
  Serial.print("Addresse IP: ");
  Serial.println(WiFi.localIP());
  Serial.println();
}


/*
 * Création de la liste des modes pour le client WEB.
 */
void modes_setup() {
  modes = "";
  uint8_t num_modes = sizeof(myModes) > 0 ? sizeof(myModes) : ws2812fx.getModeCount();
  for(uint8_t i=0; i < num_modes; i++) {
    uint8_t m = sizeof(myModes) > 0 ? myModes[i] : i;
    modes += "<li><a href='#' class='m' id='";
    modes += m;
    modes += "'>";
    modes += ws2812fx.getModeName(m);
    modes += "</a></li>";
  }
}

/* #####################################################
#  Webserver Functions
##################################################### */

String getContentType(String filename) { // convert the file extension to the MIME type
 if (filename.endsWith(".html")) return "text/html";
 else if (filename.endsWith(".css")) return "text/css";
 else if (filename.endsWith(".js")) return "application/javascript";
 else if (filename.endsWith(".ico")) return "image/x-icon";
 else if (filename.endsWith(".gz")) return "application/x-gzip";
 return "text/plain";
}

bool handleFileRead(String path){ // send the right file to the client (if it exists)
 Serial.println("handleFileRead: " + path);
 if(path.endsWith("/")) path += "index.html"; // If a folder is requested, send the index file
 String contentType = getContentType(path); // Get the MIME type
 String pathWithGz = path + ".gz";
 if(SPIFFS.exists(pathWithGz) || SPIFFS.exists(path)){ // If the file exists, either as a compressed archive, or normal
 if(SPIFFS.exists(pathWithGz)) // If there's a compressed version available
 path += ".gz"; // Use the compressed version
 File file = SPIFFS.open(path, "r"); // Open the file
 size_t sent = server.streamFile(file, contentType); // Send it to the client
 file.close(); // Close the file again
 Serial.println(String("\tSent file: ") + path);
 return true;
 }
 Serial.println(String("\tFile Not Found: ") + path);
 return false; // If the file doesn't exist, return false
}

void srv_handle_modes() {
  server.send(200,"text/plain", modes);
}
void srv_handle_dateheure() {
 server.send(200,"text/plain", DateHeure);
}

void srv_handle_minuteur() {
 server.send(200,"text/plain", AffMinuteur);
}

void srv_handle_set() {
  // c: pour la couleur m:pour le mode b: Luminosité s:Vitesse a: cycle auto p: marche/arrêt ta:minuteur
  
  for (uint8_t i=0; i < server.args(); i++){
    if(server.argName(i) == "c") {
      uint32_t tmp = (uint32_t) strtol(&server.arg(i)[0], NULL, 16);
      if(tmp >= 0x000000 && tmp <= 0xFFFFFF) {
        ws2812fx.setColor(tmp);
        EEPROMWritelong(ADRESS_LED_COL,tmp,4); // Sauvegarde de la couleur
        EEPROM.commit();
      }
    }

    if(server.argName(i) == "m") {
      uint8_t tmp = (uint8_t) strtol(&server.arg(i)[0], NULL, 10);
      ws2812fx.setMode(tmp % ws2812fx.getModeCount());
      Serial.print("mode is "); Serial.println(ws2812fx.getModeName(ws2812fx.getMode()));
      EEPROM.write(ADRESS_LED_MOD,ws2812fx.getMode()); // Sauvegarde du mode 
      EEPROM.commit();
    }

    if(server.argName(i) == "b") {
      uint8_t tmp = (uint8_t) strtol(&server.arg(i)[0], NULL, 10);
      if(server.arg(i)[0] == 'm') {
        ws2812fx.setBrightness(max((int)(ws2812fx.getBrightness() * 0.8), 5));
        tmp =0; 
      } else if (server.arg(i)[0] == 'p'){
        ws2812fx.setBrightness(min((int)(ws2812fx.getBrightness() * 1.2), 255));
         tmp =0; 
      } else if (tmp>0){
       ws2812fx.setBrightness(tmp);   
      }
      Serial.print("brightness is "); Serial.println(ws2812fx.getBrightness());
      EEPROM.write(ADRESS_LED_LUM,ws2812fx.getBrightness()); // Sauvegarde de la luminosité
      EEPROM.commit();
    }

    if(server.argName(i) == "s") {
      uint32_t tmp = (uint32_t) strtol(&server.arg(i)[0], NULL, 10); 
      if(server.arg(i)[0] == 'm') {
        ws2812fx.setSpeed(ws2812fx.getSpeed() * 0.8);
        tmp =0;
      } else if (server.arg(i)[0] == 'p'){
        ws2812fx.setSpeed(ws2812fx.getSpeed() * 1.2);
        tmp =0;
      } else if (tmp>0){       
       ws2812fx.setSpeed(tmp);   
      }
      Serial.print("speed is "); Serial.println(ws2812fx.getSpeed());
      EEPROMWritelong(ADRESS_LED_VIT,ws2812fx.getSpeed(),2);   // Sauvegarde de la vitesse
      EEPROM.commit();
    }

    if(server.argName(i) == "a") {
      if(server.arg(i)[0] == '-') {
        auto_cycle = false;
      } else {
        auto_cycle = true;
        auto_last_change = 0;
      }
    }
    if(server.argName(i) == "p") {                          // Marche arrêt bandeau LED
      if(server.arg(i)[0] == 'm') {
        MinuteurStop=(EEPROMReadlong(ADRESS_MINUTEUR,4));   // Lecture valeur minuteur dams Mémoire.
        Alarm.disable(AlarmeMin);
        ws2812fx.start();
        EEPROM.write(ADRESS_ON_OFF,1);                      // Sauvegarde LED en marche
      } else if (server.arg(i)[0] == 'a'){
        if (EEPROM.read(ADRESS_ON_OFF)==0){                 // Arrêt du bandeau apres deuxieme appuie sur arrêt  
          MinuteurStop=(EEPROMReadlong(ADRESS_MINUTEUR,4));   // Lecture valeur minuteur dams Mémoire.
          Alarm.disable(AlarmeMin);
          ws2812fx.stop();
          } 
        Alarm.enable(AlarmeMin);
        EEPROM.write(ADRESS_ON_OFF,0); // Sauvegarde LED à l'arrêt
      }
      EEPROM.commit();
    }
    if(server.argName(i) == "ta") { // Valeur de la minuterie 0- 600 mn pour arrêt en minutes
      MinuteurStop=60*((int)strtol(&server.arg(i)[0], NULL, 10));
      EEPROMWritelong(ADRESS_MINUTEUR,MinuteurStop,4);  // Sauvegarde valeur minuteur en seconde
      }
      // Alarme
    if(server.argName(i) == "tal") { // Alarme0
      Active_Alarme(String(&server.arg(i)[0]));    
      }
 
  }
  server.send(200, "text/plain", "OK");
}

/*Fonction pour ecrire dans l'EEPROM des valeurs en 32 bits (4 mots) ou 16 bits (2 mots)
* nbit=2 en 16 bits nbit=4 en 32 bits
*/
void EEPROMWritelong(int address, long value,int nbit) 
      {
      //Decomposition from a long to 4 bytes by using bitshift.
      //One = Most significant -> Four = Least significant byte
      byte four = (value & 0xFF);
      byte three = ((value >> 8) & 0xFF);
      byte two = ((value >> 16) & 0xFF);
      byte one = ((value >> 24) & 0xFF);

      //Write the 4 bytes into the eeprom memory.
      EEPROM.write(address, four);
      EEPROM.write(address + 1, three);
      if (nbit==4){
      EEPROM.write(address + 2, two);
      EEPROM.write(address + 3, one); 
      }
      EEPROM.commit();
      }

/*Fonction pour lire dans l'EEPROM des valeurs en 32 bits (4 mots) ou 16 bits (2 mots)
 * nbit=2 en 16 bits nbit=4 en 32 bits
*/
long EEPROMReadlong(long address,int nbit) 
      {
      //Read the 4 bytes from the eeprom memory.
      long four = EEPROM.read(address);
      long three = EEPROM.read(address + 1);
      long two = EEPROM.read(address + 2);
      long one = EEPROM.read(address + 3);

      //Return the recomposed long by using bitshift.
      if (nbit==4) return long((four << 0) & 0xFF) + ((three << 8) & 0xFFFF) + ((two << 16) & 0xFFFFFF) + ((one << 24) & 0xFFFFFFFF);
      if (nbit==2) return uint16_t(((four << 0) & 0xFF) + ((three << 8) & 0xFFFF));  
      }
// Effacement de la zone mémoire avant écriture
void EffaceWifiEeprom() {
 for (int i =ADRESS_WIFI ; i < ADRESS_WIFI + 96; ++i) {
 EEPROM.write(i, 0);
 }
 EEPROM.commit();
}

// Ecriture des paramètres WIFI
void EcitureWifiEeprom(String ssid,String password) {
 EffaceWifiEeprom();
 //Ecriture du SSID Max 32 caractères
 for (int i = 0; i < ssid.length(); ++i)
 {
 EEPROM.write(i + ADRESS_WIFI, ssid[i]);
 }
 //Ecriture du mot de passe Max 64 caractères
 for (int i = 0; i < password.length(); ++i)
 {
 EEPROM.write(32 + ADRESS_WIFI + i, password[i]);
 }
 EEPROM.commit();
}

// Lecture des paramètres WIFI si a=1 => SSID si a=2 => mot de passe
String LectureWifiEeprom(int a)
{
 // Lecture du SSID 
 String ssid = "";
 for (int i = 0; i < 32; ++i)
 {
 ssid += char(EEPROM.read(i+ADRESS_WIFI));
 }
 // Lecture du mot de passe
 String password = "";
 for (int i = 32; i < 96; ++i)
 {
 password += char(EEPROM.read(i + ADRESS_WIFI));
 } 
 if (a==1) return String(ssid);
 if (a==2) return String(password);
}

// conversion IP chaine en en valeur numérique
int IPnum(String valeur,int a){
 int str_len = valeur.length() + 1; 
 char *str="255.255.255.255";

valeur.toCharArray(str, str_len); // Transfert chaîne dans tableau de caractère
 unsigned char value[4] = {0};
 size_t index = 0;
 while (*str) {
 if (isdigit((unsigned char)*str)) {
 value[index] *= 10;
 value[index] += *str - '0';
 } else {
 index++;
 }
 str++;
 }
 return int(value[a]);
}

// Initialisation du bandeau LED
void initLed(){
  if (EEPROMReadlong(ADRESS_NLED,2)>LED_MAX) EEPROMWritelong(ADRESS_NLED,LED_MAX,2); 
  Serial.print("Nombres de Led: ");
  Serial.printf("%d",EEPROMReadlong(ADRESS_NLED,2));
  Serial.println();
  Serial.println("Initialisation de WS2812FX");
  if (EEPROM.read(ADRESS_ON_OFF)==0) ws2812fx.stop(); // Arrêt du bandeau
  if (EEPROM.read(ADRESS_ON_OFF)==1) ws2812fx.start(); // Marche du bandeau
  ws2812fx.setLength(EEPROMReadlong(ADRESS_NLED,2));
  ws2812fx.init();
  ws2812fx.setBrightness(EEPROM.read(ADRESS_LED_LUM));      // Lecture valeur Mémoire luminosité 
  ws2812fx.setSpeed(EEPROMReadlong(ADRESS_LED_VIT,2));      // Lecture valeur Mémoire vitesse
  ws2812fx.setMode(EEPROM.read(ADRESS_LED_MOD));            // Lecture valeur Mémoire mode
  ws2812fx.setColor(EEPROMReadlong(ADRESS_LED_COL,4));      // Lecture valeur Mémoire couleur
  MinuteurStop=(EEPROMReadlong(ADRESS_MINUTEUR,4));   // Lecture valeur minuteur dams Mémoire.
}

// Configuration des paramètres via la liaison série
void conf_serie(){
  String WIFI_SSID;
  String WIFI_PASSWORD;
  String valeur;

// Lecture port série
  while(Serial.available()) {
     valeur= Serial.readString();                             // Lecture des données sur la liaison série
     if (valeur=="raz"){
        raz(); 
     }
     
     if (valeur=="init"){
        AlarmInit();  
     }
     
     if (valeur=="led"){
        Serial.println("Entrer le nombres de LED");
        while (Serial.available()==0) {                      //Attendre la saisie
        }
        valeur=Serial.readString();
        EEPROMWritelong(ADRESS_NLED,valeur.toInt(),2);                   //  valeur nombres de LED
        Serial.println();
        Serial.print("Nombres de Led: ");
        Serial.printf("%d",EEPROMReadlong(ADRESS_NLED,2));
        EEPROM.commit();   
        raz();                                          // Reset du module ESP01 
     }
     if (valeur=="Alarme"){
        Serial.println("Voulez vous effacer les alarmes? Taper oui");
        while (Serial.available()==0) {                      //Attendre la saisie
        }
        valeur=Serial.readString();
        if (valeur=="oui"){
        Serial.println();
        Serial.print("Alarmes effacer: ");
        EEPROM.write((ADRESS_AL0),255);
        EEPROM.write((ADRESS_AL1),255); 
        EEPROM.write((ADRESS_AL2),255); 
        EEPROM.write((ADRESS_AL3),255);
        EEPROM.write((ADRESS_AL4),255);  
        EEPROM.commit();  
          
        } 
        
     }                 
     if (valeur=="wifi"){
        Serial.println("Modification des paramètres wifi");
        Serial.println("Entrer le SSID du wifi: ");          //Information de la saisie 
        while (Serial.available()==0) {                      //Attendre la saisie
        }
        WIFI_SSID=Serial.readString();                      //Lecture et enregistrement dans la variable
        Serial.println("Entrer le mot de passe du wifi: "); //Information de la saisie
        while (Serial.available()==0) {                     //Attendre la saisie
        }
        WIFI_PASSWORD=Serial.readString();                  //Lecture et enregistrement dans la variable
        EcitureWifiEeprom(WIFI_SSID,WIFI_PASSWORD);
        Serial.println("DHCP=0 ou IP FIXE=1?: ");           //Information de la saisie
        while (Serial.available()==0) {                     //Attendre la saisie
        }
        valeur=Serial.readString();                         //Lecture et enregistrement dans la variable
        if (valeur=="0") {
           EEPROM.write(ADRESS_RESEAU,0);                   //  DHCP valeur à 0
        }
        else
        {
          EEPROM.write(ADRESS_RESEAU,1);                         // IP statique à 1
          Serial.println("Adresse de la passerelle: ");          //Information de la saisie
          while (Serial.available()==0) {                        //Attendre la saisie
          }
          valeur=Serial.readString();                           //Lecture et enregistrement dans la variable                              
          for (int i = 0; i <4; ++i)
             {
             EEPROM.write(i + ADRESS_RESEAU +1, IPnum(valeur,i));
             }
          Serial.println("IP Statique : ");                       //Information de la saisie
          while (Serial.available()==0) {                        //Attendre la saisie
          }
          valeur=Serial.readString();                           //Lecture et enregistrement dans la variable  
          EEPROM.write(ADRESS_RESEAU +5, valeur.toInt());     
          }
     Serial.println("Vérification des paramètres de sauvegarde: "); //Information
     Serial.print("SSID: ");
     Serial.println(LectureWifiEeprom(1));
     Serial.print("Mot de passe: ");
     Serial.println(LectureWifiEeprom(2));
     Serial.print("Adresse IP:");
     if (EEPROM.read(ADRESS_RESEAU)==1){
      Serial.println("");
      Serial.print("Adresse IP Passerelle:");
          for (int i = 1; i <5; ++i)
          {   
               Serial.printf("%d",EEPROM.read(ADRESS_RESEAU+i));
               Serial.print(".");
          }
      Serial.println("");
      Serial.print("Fin de l'adresse IP Module:");
      Serial.printf("%d",EEPROM.read(ADRESS_RESEAU+5)); 
       }
       else {
       Serial.print("DCHP");
       }
     EEPROM.commit();                                      // Ecriture de la RAM vers EEPROM
     raz();                                          // Rest du module ESP01
}
}
}

void Date_Heure(){
 String temp="Date: ";
 timeClient.update();
 if (day()<10){
 temp+="0";
 temp+=day();
 }else temp+=day();
 temp+="/";
 if (month()<10){
 temp+="0";
 temp+=month();
 }else temp+=month();
 temp+="/";
 temp+=year();
 temp+=" Heure: ";
 DateHeure =temp+timeClient.getFormattedTime();
 setSyncProvider(getNtpTime);
}
/*-------- NTP code ----------*/
time_t getNtpTime(){
 return (timeClient.getEpochTime()); 
}

void Timer1S(){
  if (MinuteurStop==0) {
    ws2812fx.stop();
    Alarm.disable(AlarmeMin);
    MinuteurStop=(EEPROMReadlong(ADRESS_MINUTEUR,4));   // Lecture valeur minuteur dams Mémoire.
  }else {
    --MinuteurStop;
  }

  if ((MinuteurStop-60)>0){
    AffMinuteur="Minuteur temps restant: ";
    AffMinuteur+=((MinuteurStop/60)+1);
    AffMinuteur+=" mn";
    }
    else{
    AffMinuteur="Minuteur temps restant: ";
    AffMinuteur+=MinuteurStop;
    AffMinuteur+=" s";
}
}
// Raz esp8266
void raz(){
 delay(500);
 WiFi.mode(WIFI_OFF);     // Arrêt du WIFI
 ESP.restart();           // Redémarrage
}

// Gestion des alarmes
 
void Active_Alarme(String ParaAl){
      int NumAlarme=0, NumHeure=0, NumMinute=0, NumOnOff=0,NumJour=0,NumCouleur=0,NumMode=0;
      // Lecture des paramètres
      StringSplitter *splitter = new StringSplitter(ParaAl, ',', 7);  // Initialisation de la classe avec 7 paramètres
      int itemCount = splitter->getItemCount();
      String para[itemCount];
      for(int i = 0; i < itemCount; i++){
      String item = splitter->getItemAtIndex(i);
      Serial.print("Valeur" + String(i) + ": " + (item)+" ");
      para[i]=item;
      }
      Serial.println("Alarme"+(para[0])+" ");
      Serial.println("Pramètres Alarme "+ (para[0]) +": " + ParaAl);
      
     
      if ((para[0]).toInt()<5){                                      // Paramètre N° alarme de 0 à 4
         NumAlarme=(para[0]).toInt();
      } else return; // 
     
      if (Alarme[NumAlarme]!= dtINVALID_ALARM_ID) {                  // Libération de l'alarme
        Alarm.free(Alarme[NumAlarme]);
        Alarme[NumAlarme]=dtINVALID_ALARM_ID;
        EEPROM.write((ADRESS_AL0+ADRESS_ALB*NumAlarme),255);         // Paramètre 0 ID à 255
        EEPROM.commit();
      }
      
      if ((para[1]).toInt()<24 && (para[1]).toInt()>=0){             // Paramètre Heure de 0 à 23
         NumHeure=(para[1]).toInt();
      } else return; // 
      if ((para[2]).toInt()<60 && (para[2]).toInt()>=0){             // Paramètre Minute de 0 à 59
         NumMinute=(para[2]).toInt();
      } else return; // 
      if ((para[3]).toInt()<2 && (para[3]).toInt()>=0){              // Paramètre Marche=1 Arrêt=0
         NumOnOff=(para[3]).toInt();
      } else return; // 
      if ((para[4]).toInt()<8 && (para[4]).toInt()>=0){              // Paramètre Jour 0 à 7 Toujours=0
         NumJour=(para[4]).toInt();
      } else return; // 
      if ((para[5]).toInt()<6 && (para[5]).toInt()>=0){              // Paramètre Couleur 0 à 5 
         NumCouleur=(para[5]).toInt();
      } else return; // 
      if ((para[6]).toInt()<56 && (para[6]).toInt()>=0){             // Paramètre Mode 0 à 55 
         NumMode=(para[6]).toInt();
      } else return; //
    
      Alarm_Select(NumAlarme,NumHeure,NumMinute,NumOnOff,NumJour);
      //dowSunday, dowMonday, dowTuesday, dowWednesday, dowThursday, dowFriday, dowSaturday   
      Serial.print("ID alarme " + String(NumAlarme) +": ");
      Serial.println(Alarme[NumAlarme]);
      // Sauvegarde paramètres EEPROM
      EEPROM.write((ADRESS_AL0+ADRESS_ALB*NumAlarme),Alarme[NumAlarme]);           // Paramètre 0 ID
      EEPROM.write((ADRESS_AL0+1+ADRESS_ALB*NumAlarme),NumHeure);                  // Paramètre 1 Heure
      EEPROM.write((ADRESS_AL0+2+ADRESS_ALB*NumAlarme),NumMinute);                 // Paramètre 2 Minute
      EEPROM.write((ADRESS_AL0+3+ADRESS_ALB*NumAlarme),NumOnOff);                  // Paramètre 3 on/off;
      EEPROM.write((ADRESS_AL0+4+ADRESS_ALB*NumAlarme),NumJour);                   // Paramètre 4 Jour;
      EEPROM.write((ADRESS_AL0+5+ADRESS_ALB*NumAlarme),NumCouleur);                // Paramètre 5 Couleur;
      EEPROM.write((ADRESS_AL0+6+ADRESS_ALB*NumAlarme),NumMode);                   // Paramètre 6 Mode;
      EEPROM.commit();                                                             // Ecriture de la RAM vers EEPROM
}

void AlarmInit(){
  int ID=0,NumAlarme=0, NumHeure=0, NumMinute=0, NumOnOff=0,NumJour=0;
  for (int i=0; i <5; i++){
    NumAlarme=i;
    ID=EEPROM.read(ADRESS_AL0+ADRESS_ALB*NumAlarme);
    NumHeure=EEPROM.read(ADRESS_AL0+1+ADRESS_ALB*NumAlarme);
    NumMinute=EEPROM.read(ADRESS_AL0+2+ADRESS_ALB*NumAlarme);
    NumOnOff=EEPROM.read(ADRESS_AL0+3+ADRESS_ALB*NumAlarme);
    NumJour=EEPROM.read(ADRESS_AL0+4+ADRESS_ALB*NumAlarme);
    Serial.println("Initialisation Alarme "+String (NumAlarme)+" : Valeur ID"+ String (ID)+" H:"+ String (NumHeure)+" M:"+ String (NumMinute)+" ON OFF:"+ String (NumOnOff)+" Jour:"+ String (NumJour));
    Alarm.free(Alarme[NumAlarme]);
    Alarme[NumAlarme]=dtINVALID_ALARM_ID;
    if (ID!=255){
         Alarm_Select(NumAlarme,NumHeure,NumMinute,NumOnOff,NumJour);
         Serial.println("Alarme "+String (NumAlarme)+" : ID "+ String (Alarme[NumAlarme])+ "= Active");
    }
  }
}

void Alarm_Select(int NumAlarme,int NumHeure,int NumMinute,int NumOnOff,int NumJour){
   //dowSunday, dowMonday, dowTuesday, dowWednesday, dowThursday, dowFriday, dowSaturday 
      switch (NumJour) {
          case 0:
          // Toujours
          if (NumOnOff==1) Alarme[NumAlarme]=Alarm.alarmRepeat(NumHeure,NumMinute,0,Alarm_On);
          if (NumOnOff==0) Alarme[NumAlarme]=Alarm.alarmRepeat(NumHeure,NumMinute,0,Alarm_Off);
          break;
          case 1:
          // Dimanche
          if (NumOnOff==1) Alarme[NumAlarme]=Alarm.alarmRepeat(dowSunday,NumHeure,NumMinute,0,Alarm_On);
          if (NumOnOff==0) Alarme[NumAlarme]=Alarm.alarmRepeat(dowSunday,NumHeure,NumMinute,0,Alarm_Off);
          break;
          case 2:
          // Lundi
          if (NumOnOff==1) Alarme[NumAlarme]=Alarm.alarmRepeat(dowMonday,NumHeure,NumMinute,0,Alarm_On);
          if (NumOnOff==0) Alarme[NumAlarme]=Alarm.alarmRepeat(dowMonday,NumHeure,NumMinute,0,Alarm_Off);
          break;
          case 3:
          // Mardi
          if (NumOnOff==1) Alarme[NumAlarme]=Alarm.alarmRepeat(dowTuesday,NumHeure,NumMinute,0,Alarm_On);
          if (NumOnOff==0) Alarme[NumAlarme]=Alarm.alarmRepeat(dowTuesday,NumHeure,NumMinute,0,Alarm_Off);
          break;
          case 4:
          // Mercredi
          if (NumOnOff==1) Alarme[NumAlarme]=Alarm.alarmRepeat(dowWednesday,NumHeure,NumMinute,0,Alarm_On);
          if (NumOnOff==0) Alarme[NumAlarme]=Alarm.alarmRepeat(dowWednesday,NumHeure,NumMinute,0,Alarm_Off);
          break;
          case 5:
          // Jeudi
          if (NumOnOff==1) Alarme[NumAlarme]=Alarm.alarmRepeat(dowThursday,NumHeure,NumMinute,0,Alarm_On);
          if (NumOnOff==0) Alarme[NumAlarme]=Alarm.alarmRepeat(dowThursday,NumHeure,NumMinute,0,Alarm_Off);
          break;
          case 6:
          // Vendredi
          if (NumOnOff==1) Alarme[NumAlarme]=Alarm.alarmRepeat(dowFriday,NumHeure,NumMinute,0,Alarm_On);
          if (NumOnOff==0) Alarme[NumAlarme]=Alarm.alarmRepeat(dowFriday,NumHeure,NumMinute,0,Alarm_Off);
          break;
          case 7:
          // Samedi
          if (NumOnOff==1) Alarme[NumAlarme]=Alarm.alarmRepeat(dowSaturday,NumHeure,NumMinute,0,Alarm_On);
          if (NumOnOff==0) Alarme[NumAlarme]=Alarm.alarmRepeat(dowSaturday,NumHeure,NumMinute,0,Alarm_Off);
          break;
          default:
          //Erreur Initialisation Alarmes
          Serial.println("Erreur Initialisation Alarme "+String (NumAlarme)+" :");
     }
}

void Alarm_On() {
  // Allume le badeau LED
  Serial.println(String(Alarm.Use_ID()));
  int ID=0,NumAlarme=0,NumCouleur=0,NumMode=0;
  for (int i=0; i <5; i++){
    NumAlarme=i;
    ID=EEPROM.read(ADRESS_AL0+ADRESS_ALB*NumAlarme);
    if (ID==Alarm.Use_ID()){
    NumCouleur=EEPROM.read(ADRESS_AL0+5+ADRESS_ALB*NumAlarme);
    NumMode=EEPROM.read(ADRESS_AL0+6+ADRESS_ALB*NumAlarme);               // Lecture valeur Mémoire mode
    Serial.println("Mode alame"+ String(NumMode));
    switch (NumCouleur){
    case 0:
    ws2812fx.setColor(0x00ff00);                                          // Lecture valeur couleur verte
    break;
    case 1 :
    ws2812fx.setColor(0x0000ff);                                          // Lecture valeur couleur Bleue    
    break;
    case 2 :
    ws2812fx.setColor(0xff0000);                                          // Lecture valeur couleur Rouge   
    break;
    case 3 :
    ws2812fx.setColor(0xff00ff);                                          // Lecture valeur couleur Fuschia   
    break;
    case 4 :
    ws2812fx.setColor(0x9900ff);                                          // Lecture valeur couleur Violette   
    break;
    case 5 :
    ws2812fx.setColor(0xFF9E25);                                          // Lecture valeur couleur Blanc   
    break;
   }
   ws2812fx.setMode(NumMode);
    }
  }
  Alarm.disable(AlarmeMin);
  ws2812fx.start();
  EEPROM.write(ADRESS_ON_OFF,1);          // Sauvegarde LED en marche
  EEPROM.commit();                        // Ecriture de la RAM vers EEPROM
  
}
void Alarm_Off() {
  // Etient le badeau LED
  Serial.println(String(Alarm.Use_ID()));
  ws2812fx.stop();
  EEPROM.write(ADRESS_ON_OFF,0);          // Sauvegarde LED en marche
  EEPROM.commit();                        // Ecriture de la RAM vers EEPROM
}




