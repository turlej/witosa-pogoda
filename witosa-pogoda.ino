//#include <DHT.h>
#include <Wire.h>
#include <BH1750.h>
#include <Adafruit_BMP085.h>
#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <WiFiClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "hasla.h"
#include <ESP8266WebServer.h>

#define HTU21DF_I2CADDR       0x40
#define HTU21DF_READTEMP      0xE3
#define HTU21DF_READHUM       0xE5
#define HTU21DF_WRITEREG      0xE6
#define HTU21DF_READREG       0xE7
#define HTU21DF_RESET         0xFE
//#define DHTPIN 0
//#define DHTTYPE DHT22
//DHT dht(DHTPIN, DHTTYPE);
#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
WiFiClient client;
Adafruit_BMP085 bmp;
BH1750 lightMeter;
ESP8266WebServer server(80);

const char* server3 = "api.thingspeak.com";
const char* server2 = "marcin.eu.pn";


//uint8_t adres[8];
const uint8_t pole[8] = {40, 180, 28, 195, 3, 0, 0, 205}; //28-000003c31cb4
const uint8_t dom[8] = {40, 94, 91, 133, 5, 0, 0, 171}; //28-000005855b5e
const uint8_t okno[8] = {40, 128, 16, 160, 5, 0, 0, 20}; //28-000005a01080
const uint8_t grzejnik[8] = {40, 121, 76, 133, 5, 0, 0, 181}; //28-000005854c79
uint16_t lux;
float t_pole, t_dom, t_okno, t_grzejnik, cisnienie, wilgotnosc;
unsigned int czas=5, czas2;
int rssi;

void handleRoot() {
  String strona;
  String do_wyslania;
  
  do_wyslania += "<html><body>";
  strona += "<b>Witosa - Pogoda</b><br><br>";
  strona += "<b>WiFi SSID: ";
  strona += WiFi.SSID();
  strona += "</b><br>";
  strona += "<b>WiFi RSSI: ";
  strona += WiFi.RSSI();
  strona += "</b><br>";
  strona += "Rozmiar pamieci: ";
  strona += ESP.getFlashChipRealSize()/1024;
  strona += " KB";
  strona += "<br>Czas pracy: ";
  if (millis()/3600000<10) strona += "0";
  strona += millis()/3600000;
  strona += ":";
  if (millis()/60000%60<10) strona += "0";
  strona += millis()/60000%60;
  strona += ":";
  if (millis()/1000%60<10) strona += "0";
  strona += millis()/1000%60;

  strona += "<br><br>Pole: ";
  strona += t_pole;
  strona += "&deg;C<br>";
  strona += "Dom: ";
  strona += t_dom;
  strona += "&deg;C<br>";
  strona += "Okno: ";
  strona += t_okno;
  strona += "&deg;C<br>";
  strona += "Grzejnik: ";
  strona += t_grzejnik;
  strona += "&deg;C<br>";
  strona += "Cisnienie: ";
  strona += cisnienie;
  strona += "hPa<br>";
  strona += "Wilgotnosc: ";
  strona += wilgotnosc;
  strona += "%<br><br>";
  
  do_wyslania += strona;
  do_wyslania += "<a href='http://192.168.2.111/'>Odswierz</a><br><br><a href='http://192.168.2.111/reboot'>Reset ESP CPU</a></body></html>";
  do_wyslania += "</body></html>";
  server.send(200, "text/html", do_wyslania);
}

void handleNotFound(){
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET)?"GET":"POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i=0; i<server.args(); i++){
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

float pomiar_temperatury(void) {
  
  // OK lets ready!
  Wire.beginTransmission(HTU21DF_I2CADDR);
  Wire.write(HTU21DF_READTEMP);
  if (Wire.endTransmission()) return -50.0;
  
  delay(50); // add delay between request and actual read!
  
  Wire.requestFrom(HTU21DF_I2CADDR, 3);

  uint16_t t;
  if (Wire.available()) t = Wire.read(); else return -50.0;
  t <<= 8;
  if (Wire.available()) t |= Wire.read(); else return -50.0;

  uint8_t crc;
  if (Wire.available()) crc = Wire.read(); else return -50.0;
  
  float temp = t;
  temp *= 175.72;
  temp /= 65536;
  temp -= 46.85;

  return temp;
}

float pomiar_wilgotnosci(void) {
  // OK lets ready!
  Wire.beginTransmission(HTU21DF_I2CADDR);
  Wire.write(HTU21DF_READHUM);
  if (Wire.endTransmission()) return -1.0;
  
  delay(50); // add delay between request and actual read!
  
  Wire.requestFrom(HTU21DF_I2CADDR, 3);

  uint16_t h;
  if (Wire.available()) h = Wire.read(); else return -1.0;
  h <<= 8;
  if (Wire.available()) h |= Wire.read(); else return -1.0;

  uint8_t crc;
  if (Wire.available()) crc = Wire.read(); else return -1.0;

  float hum = h;
  hum *= 125;
  hum /= 65536;
  hum -= 6;

  return hum;
}

void setup(void){
  pinMode(2, INPUT);
  Serial.begin(115200);
  Wire.begin();
  Wire.beginTransmission(HTU21DF_I2CADDR);
  Wire.write(HTU21DF_RESET);
  Wire.endTransmission();
  delay(15);
  bmp.begin();
  lightMeter.begin();
  //dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  WiFi.config(IPAddress(192,168,2,111), IPAddress(192,168,2,1), IPAddress(255,255,255,0));
  Serial.println("");
  Serial.print("Rozmiar pamieci: ");
  Serial.println(ESP.getFlashChipSize());
  Serial.println(ESP.getFlashChipRealSize());

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  // Port defaults to 8266
  // ArduinoOTA.setPort(8266);

  // Hostname defaults to esp8266-[ChipID]
  ArduinoOTA.setHostname("witosa-pogoda");

  // No authentication by default
  ArduinoOTA.setPassword(haslo_ota);

  ArduinoOTA.onStart([]() {
    Serial.println("Start");
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
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

  server.on("/reboot", [](){
    server.send(200, "text/plain", "Restarting");
    delay(5000);
    ESP.restart();
  });

  server.on("/", handleRoot);

  server.onNotFound(handleNotFound);

  server.begin();
  
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
}
 
void loop(void){
  czas2 = czas;
  czas = millis()/60000;
  ArduinoOTA.handle();
  server.handleClient();
  if (czas != czas2)
  {
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Start pomiaru");
    DS18B20.begin();
    DS18B20.requestTemperatures();
    delay(300);
    //wilgotnosc = dht.readHumidity();
    wilgotnosc = pomiar_wilgotnosci();
    if (wilgotnosc < 0) wilgotnosc = pomiar_wilgotnosci();
    if (wilgotnosc>100) {wilgotnosc = 100.0;}
    delay(100);
    lux = lightMeter.readLightLevel();
    delay(100);
    cisnienie = bmp.readPressure()/100.0;
    delay(300);
    t_pole = DS18B20.getTempC(pole);
    t_dom = DS18B20.getTempC(dom);
    t_okno = DS18B20.getTempC(okno);
    t_grzejnik = DS18B20.getTempC(grzejnik);

    rssi = 2*(WiFi.RSSI()+100);
    
    Serial.print("Pole: ");
    Serial.println(t_pole);
    Serial.print("Dom: ");
    Serial.println(t_dom);
    Serial.print("Okno: ");
    Serial.println(t_okno);
    Serial.print("Grzejnik: ");
    Serial.println(t_grzejnik);
    Serial.print("Cisnienie: ");
    Serial.println(cisnienie);
    Serial.print("Wilgotnosc: ");
    Serial.println(wilgotnosc);
    Serial.print("Swiatlo: ");
    Serial.println(lux);
  
    if (client.connect(server3,80)) {
      String postStr = apiKey;
             postStr +="&field1=";
             postStr += String(t_pole);
             postStr +="&field2=";
             postStr += String(t_dom);
             postStr +="&field3=";
             postStr += String(t_grzejnik);
             postStr +="&field4=";
             postStr += String(cisnienie);
             postStr +="&field5=";
             postStr += String(wilgotnosc);
             postStr +="&field6=";
             postStr += String(t_okno);
             postStr +="&field8=";
             postStr += String(lux);
             postStr += "\r\n\r\n";
   
       client.print("POST /update HTTP/1.1\n"); 
       client.print("Host: api.thingspeak.com\n"); 
       client.print("Connection: close\n"); 
       client.print("X-THINGSPEAKAPIKEY: "+apiKey+"\n"); 
       client.print("Content-Type: application/x-www-form-urlencoded\n"); 
       client.print("Content-Length: "); 
       client.print(postStr.length()); 
       client.print("\n\n"); 
       client.print(postStr);
             
       Serial.println("send to Thingspeak");    
      }
    client.stop();
    if (client.connect(server2,80)) {
      String postStr ="&klucz=";
             postStr += klucz;
             postStr +="&pole=";
             postStr += String(t_pole);
             postStr +="&dom=";
             postStr += String(t_dom);
             postStr +="&okno=";
             postStr += String(t_okno);
             postStr +="&grzejnik=";
             postStr += String(t_grzejnik);
             postStr +="&cisnienie=";
             postStr += String(cisnienie);
             postStr +="&wilgotnosc=";
             postStr += String(wilgotnosc);
             postStr +="&naslonecznienie=";
             postStr += String(lux);
             postStr +="&rssi=";
             postStr += String(rssi);
             postStr += "\r\n\r\n";
   
       client.println("POST /wifi_pogoda.php HTTP/1.1"); 
       client.println("Host: marcin.eu.pn"); 
       client.println("Connection: close"); 
       client.println("Content-Type: application/x-www-form-urlencoded"); 
       client.print("Content-Length: "); 
       client.print(postStr.length()); 
       client.print("\n\n"); 
       client.print(postStr);
             
       Serial.println("send to Hosting");    
      }
    client.stop();
  } else
  {
    Serial.println("Brak sieci WiFi");
  }
  }
}

