#include <DHT.h>
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

#define DHTPIN 16
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
#define ONE_WIRE_BUS 2  // DS18B20 pin
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature DS18B20(&oneWire);
WiFiClient client;
Adafruit_BMP085 bmp;
BH1750 lightMeter;

const char* server = "api.thingspeak.com";
const char* server2 = "marcin.eu.pn";
const char* server3 = "marcin.myvnc.com";

//uint8_t adres[8];
const uint8_t pole[8] = {40, 180, 28, 195, 3, 0, 0, 205}; //28-000003c31cb4
const uint8_t dom[8] = {40, 94, 91, 133, 5, 0, 0, 171}; //28-000005855b5e
const uint8_t okno[8] = {40, 128, 16, 160, 5, 0, 0, 20}; //28-000005a01080
const uint8_t grzejnik[8] = {40, 121, 76, 133, 5, 0, 0, 181}; //28-000005854c79
uint16_t lux;
float t_pole, t_dom, t_okno, t_grzejnik, cisnienie, wilgotnosc;
unsigned int czas, czas2;

void setup(void){
  pinMode(2, INPUT);
  Serial.begin(115200);
  bmp.begin();
  lightMeter.begin();
  dht.begin();
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.print("Rozmiar pamieci: ");
  Serial.println(ESP.getFlashChipSize());

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
  // ArduinoOTA.setPassword((const char *)"123");

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
  if (czas != czas2)
  {
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.println("Start pomiaru");
    DS18B20.begin();
    DS18B20.requestTemperatures();
    //dht.begin();
    delay(800);
  
    t_pole = DS18B20.getTempC(pole);
    t_dom = DS18B20.getTempC(dom);
    t_okno = DS18B20.getTempC(okno);
    t_grzejnik = DS18B20.getTempC(grzejnik);
    lux = lightMeter.readLightLevel();
    cisnienie = bmp.readPressure()/100.0;
    wilgotnosc = dht.readTemperature();//dht.readHumidity();
    
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
  
    if (client.connect(server,80)) {
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
      String postStr = apiKey;
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
    if (client.connect(server3,85)) {
      String postStr = apiKey;
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
             postStr += "\r\n\r\n";
   
       client.println("POST /wifi_pogoda.php HTTP/1.1"); 
       client.println("Host: marcin.myvnc.com"); 
       client.println("Connection: close"); 
       client.println("Content-Type: application/x-www-form-urlencoded"); 
       client.print("Content-Length: "); 
       client.print(postStr.length()); 
       client.print("\n\n"); 
       client.print(postStr);
             
       Serial.println("send to Raspberry");    
      }
    client.stop();
  } else
  {
    Serial.println("Brak sieci WiFi");
  }
  }
  //delay(60000);  
}

