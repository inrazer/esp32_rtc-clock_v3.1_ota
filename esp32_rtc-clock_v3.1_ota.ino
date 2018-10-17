/**************************************************************
 * This is the sketch for a R 1250 M/C nixie display unit by ML, Budapest 
 * to be used as a fancy desk clock.
 * 
 * Time is update from defined NTP server(s)
 */


//#define SERIAL-ON // needs to be uncommented for ANY debug function
//#define DEBUG   // uncomment if needed
//#define TDEBUG    // uncomment for to serial output a string with actual time
#define POST      // uncomment if you wish a POST at first power-up

#include <WiFi.h>
#include <WiFiMulti.h>
#include <ESPmDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <Time.h>

WiFiMulti wifiMulti;

int hou_t = 0;
int hou_o = 0;
int min_t = 0;
int min_o = 0;
int sec_t = 0;
int sec_o = 0;

int secondsNow = 1;
int secondsBefore = 0;

int data[]={18,5,17,16};
//int data[]={15,13,12,14};                         // Array of BCD-datalines
                                                    // Info: Pins 34,35,36,39 are RO-Input-Pins
                                                    
int adress[]={12,14,27,26,25,33};                   // Array of adress lines
int times[]={sec_o,sec_t,min_o,min_t,hou_o,hou_t};  // Array of variables for time digits

char buffer [10];                                   // Buffer for auxiliary 


int relay_on  = 23;                                 // the control pins for latching relay
int relay_off = 22;                                 // for the +160V Anode supply

/*
int a0 = 2;                     // seconds ones
int a1 = 0;                     // seconds tens
int a2 = 16;                    // minutes ones
int a3 = 39;                    // minutes tens (This Pin is labeled "SVN")
int a4 = 25;                    // hours ones
int a5 = 26;                    // hours tens
*/

int xschiebe = 0;
//************************************************
#define SchlafZeit 3E7 // Mikrosekunden hier 3s
#define NTP_SERVER "de.pool.ntp.org"
#define TZ_INFO "WEST-1DWEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00" // Western European Time
RTC_DATA_ATTR int bootZaeler = 0;   // Variable in RTC Speicher bleibt erhalten nach Reset
//************************************************
void ErsteStart()
{
  #ifdef DEBUG
  Serial.println("Start nach dem Reset");
  #endif
  WiFi.mode(WIFI_STA);
  //WiFi.begin("network-name", "network-password");
    wifiMulti.addAP("ssid_from_AP_1", "your_password_for_AP_1");
    wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
    wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");
  #ifdef DEBUG
  Serial.println("");
  #endif
 
  while (wifiMulti.run() != WL_CONNECTED)
  {
    delay(200);
    pinMode(2, OUTPUT);
    digitalWrite(2, HIGH);   // Turn the LED on
    delay(200);
    digitalWrite(2, LOW);    // Turn the LED off
    #ifdef DEBUG
    Serial.print(".");
    #endif
  }
  #ifdef DEBUG
  Serial.println("");
  Serial.print("IP Addresse: ");
  Serial.println(WiFi.localIP());
  Serial.println("Hole NTP Zeit");
  #endif
  struct tm local;
  configTzTime(TZ_INFO, NTP_SERVER); // ESP32 Systemzeit mit NTP Synchronisieren
  getLocalTime(&local, 10000);      // Versuche 10 s zu Synchronisieren
  //WiFi.mode(WIFI_OFF);
  
  SetRelay();

  #ifdef POST
    PostTest();
    delay(250);
    UnsetRelay();
    delay(250);
    SetRelay();
  #endif
}

void setup()
{
  pinMode(relay_on, OUTPUT);
  pinMode(relay_off, OUTPUT);
  
  digitalWrite (relay_off, HIGH);
  delay(250);
  digitalWrite (relay_off, LOW);
  
  for (int pin = 0; pin <=3; pin++){ // set all data lines as output
   pinMode(data[pin], OUTPUT);
  }

  for (int pin = 0; pin <=5; pin++){ // set all selec lines as output
   pinMode(adress[pin], OUTPUT);
  } 
  
  esp_sleep_wakeup_cause_t wakeup_cause; // Variable für wakeup Ursache
  setenv("TZ", TZ_INFO, 1);             // Zeitzone  muss nach dem reset neu eingestellt werden
  tzset();
  #ifdef SERIAL-ON
  Serial.begin(115200);
  #endif
  #ifdef DEBUG
  Serial.println("");
  ++bootZaeler;
  Serial.println("Start Nr.: " + String(bootZaeler));
  #endif
  wakeup_cause = esp_sleep_get_wakeup_cause(); // wakeup Ursache holen
  if (wakeup_cause != 3) ErsteStart();     // Wenn wakeup durch Reset
/*
 * OTA System
 */
  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("nixie-clock");

  // No authentication by default
  // ArduinoOTA.setPassword("admin");

  // Password can be set with it's md5 value as well
  // MD5(admin) = 21232f297a57a5a743894a0e4a801fc3
  // ArduinoOTA.setPasswordHash("21232f297a57a5a743894a0e4a801fc3");

  ArduinoOTA.onStart([]() {
   String type;
   if (ArduinoOTA.getCommand() == U_FLASH)
     type = "sketch";
   else // U_SPIFFS
     type = "filesystem";

   // NOTE: if updating SPIFFS this would be the place to unmount SPIFFS using SPIFFS.end()
   #ifdef DEBUG
   Serial.println("Start updating " + type);
   #endif
  });
  ArduinoOTA.onEnd([]() {
    #ifdef DEBUG
    Serial.println("\nEnd");
    #endif
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    #ifdef DEBUG
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
    #endif
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) Serial.println("Auth Failed");
    else if (error == OTA_BEGIN_ERROR) Serial.println("Begin Failed");
    else if (error == OTA_CONNECT_ERROR) Serial.println("Connect Failed");
    else if (error == OTA_RECEIVE_ERROR) Serial.println("Receive Failed");
    else if (error == OTA_END_ERROR) Serial.println("End Failed");
  });
  ArduinoOTA.begin();
}
//**********************************************************************
void PostTest()
  {
     for (int count = 0; count <=9; count ++){
      times[0] = count;
      times[1] = count;
      times[2] = count;
      times[3] = count;
      times[4] = count;
      times[5] = count;
      WriteTime();
      delay(250);
     }
  }
//********************************************************************** 
void SetRelay()
  {
    digitalWrite (relay_on, HIGH);
    delay(250);
    digitalWrite (relay_on, LOW);
  }
//********************************************************************** 
void UnsetRelay()
  {
    digitalWrite (relay_off, HIGH);
    delay(250);
    digitalWrite (relay_off, LOW);
  }
//**********************************************************************
void WriteTime()
  {
      for (int i = 0; i <= 5; i++){
  digitalWrite(adress[i], HIGH);
  delayMicroseconds(1000);
  // do something different depending on number value:
  switch (times[i]) {
    case 0:    // your hand is on the sensor
      digitalWrite(data[0], LOW);
      digitalWrite(data[1], LOW);
      digitalWrite(data[2], LOW);
      digitalWrite(data[3], LOW);
      break;
    case 1:    // your hand is close to the sensor
      digitalWrite(data[0], HIGH);
      break;
    case 2:    // your hand is a few inches from the sensor
      digitalWrite(data[1], HIGH);
      break;
    case 3:    // your hand is nowhere near the sensor
      digitalWrite(data[0], HIGH);
      digitalWrite(data[1], HIGH);
      break;
    case 4:    // your hand is nowhere near the sensor
      digitalWrite(data[2], HIGH);
      break;
    case 5:    // your hand is nowhere near the sensor
      digitalWrite(data[0], HIGH);
      digitalWrite(data[2], HIGH);
      break;
    case 6:    // your hand is nowhere near the sensor
      digitalWrite(data[1], HIGH);
      digitalWrite(data[2], HIGH);
      break;
    case 7:    // your hand is nowhere near the sensor
      digitalWrite(data[0], HIGH);
      digitalWrite(data[1], HIGH);
      digitalWrite(data[2], HIGH);
      break;
    case 8:    // your hand is nowhere near the sensor
      digitalWrite(data[3], HIGH);
      break;
    case 9:    // your hand is nowhere near the sensor
      digitalWrite(data[0], HIGH);
      digitalWrite(data[3], HIGH);
      break;  
    }  
  delayMicroseconds(1000); // time-out between A-high and D-high
  digitalWrite(adress[i], LOW);
  
  delayMicroseconds(1000); // time-out between A-low and D-low
  for (int pin = 0; pin <=3; pin++){
    digitalWrite(data[pin], LOW);
  }
  delayMicroseconds(1000); // time-out between Adress+Data Block
  }
  delayMicroseconds(5000); // delay in between parsing new time
  
  secondsBefore = times[0];
}

void loop()
  {
    // Handle OTA update requests
    ArduinoOTA.handle();
   
   tm local;
   getLocalTime(&local);
   char buffer [10];
   strftime (buffer,7,"%H%M%S",&local);
   
   #ifdef DEBUG
   Serial.println("Ausgabe des Puffers:");
   #endif
   
   int buf_index = 5;
   
   for (int i=0; i <= 5; i++){
    if (buf_index >= 0){
      times[i] = buffer[buf_index]-48;
      buf_index--;
    }
   }
  
  secondsNow = times[0];

  // cycle the tubes for lifetime reasons every hour
  if ( times[3] == 0){        // minute tens
    if ( times[2] == 0){      // minute ones
      if ( times[1] == 0){    // second tens
        if ( times[0] == 0){  // second ones
          PostTest();
          PostTest();  
        }
      }
    }
  }
  
  if ( secondsNow != secondsBefore)
    {
    WriteTime();
    #ifdef TDEBUG
    Serial.println(&local, "Datum: %d.%m.%y  Zeit: %H:%M:%S"); // Zeit Datum Print Ausgabe formatieren
    #endif
    }
   
   #ifdef DEBUG
   Serial.println(buffer);
   Serial.println("\r\n");
   Serial.println("Ausgabe der ints:");
   for (int i=5; i >= 0; i--){ Serial.println(times[i]); }
   #endif
}
