// Wether station for MCU
//
// Program gets values of two sensor from server via graphql API. Values are displayed in 3s intervall.
//
// Hardware:
//   Lolin(Wemos) D1 mini pro, ESP8266
//   TM1637 4-digit display
//
// Libraries:
//   ESP8266 board: https://arduino-esp8266.readthedocs.io/en/latest/installing.html
//   ArduinoJson (version 6.19.4)
//   TM1637 Driver (by AKJ), version 2.1.1

int debug=0;

#include <Arduino.h>
#include <ArduinoJson.h>
StaticJsonDocument<400> doc;

// Wifi
#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecureBearSSL.h>
ESP8266WiFiMulti WiFiMulti;

#include <TM1637.h>

// Configs
const char* ssid = "xxx";
const char* password = "xxx";
String token = "bearer xxx";
String host = "https://xx/api/graphql";
const int8_t interval = 10; // Refresh interval in minutes


int CLK = D7;
int DIO = D6;
TM1637 tm(CLK, DIO);

double lake = 999;
double sauna = 999;
int line = 0;

void setup() {
  if (debug) Serial.begin(115200);
  if (debug) Serial.println("Started");

  tm.begin();
  tm.setBrightness(4);
  tm.display("conn");
   
  WiFi.mode(WIFI_STA);
  WiFiMulti.addAP(ssid, password);
  while (WiFiMulti.run() != WL_CONNECTED) {
    delay(100);
    if (debug) Serial.print(".");
  }
  if (debug) Serial.println("connection ok");
  if (debug) Serial.println(WiFi.localIP());
  if (debug) Serial.println("Setup complete");
}

void loop() {
  tm.display("load");
  lake = getTempFromServer("CLAK");
  if (debug) Serial.println(lake);
  displayNumber(lake);
  delay(2000);
  sauna = getTempFromServer("CSAU");
  if (debug) Serial.println(sauna);
  displayNumber(sauna);
  delay(interval*60*1000);
}

void displayNumber(double num) {
  uint8_t offset = 0;
  tm.clearScreen();
  if (num==999) {
    tm.setDp(0);
    tm.display("Err");
  } else {
    if (num>10) offset = 1;
    if (num>=1 && num<10) offset = 2;
    if (num>=0 && num<1) offset = 3;
    if (num<0 && num>-1) offset = 2;
    if (num<=-1 && num>-10) offset = 1;
    tm.setDp(4);
    tm.display((int) (num*10), true, false,offset);  
  }
}

double getTempFromServer(String sensor) {
  double value = 999;
  if ((WiFiMulti.run() == WL_CONNECTED)) {
    std::unique_ptr<BearSSL::WiFiClientSecure>client(new BearSSL::WiFiClientSecure);
    client->setInsecure();  
    HTTPClient https;
    https.useHTTP10(true);

    if (https.begin(*client, host)) {
      https.addHeader("Content-Type","application/json");
      https.addHeader("Authorization",token);
      int httpCode = https.POST("{\"query\": \"query SensorDetails($sensorName: String!) {sensorDetails(sensorName: $sensorName) {lastValue}}\", \"variables\":{\"sensorName\": \""+sensor+"\"}}");
      if (httpCode > 0) {
        if (debug) Serial.printf("[HTTPS] POST... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
          String payload = https.getString();
          if (debug) Serial.println(payload);
          DeserializationError error = deserializeJson(doc, payload);
          if (!error) {
            value = doc["data"]["sensorDetails"]["lastValue"];
          }
        }
      } else {
        if (debug) Serial.printf("[HTTPS] POST... failed, error: %s\n", https.errorToString(httpCode).c_str());
      }
      https.end();
    }
  }
  return value;
}
