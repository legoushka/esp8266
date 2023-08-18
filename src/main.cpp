#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <ArduinoJson.h>
#include <ESP32_Supabase.h>
#include <Adafruit_Sensor.h>
#include "DHT.h"
#include <secret.h>

#define LED_1 D6
#define LED_2 D7
#define LED_0 LED_BUILTIN
#define DHT_PIN D5

#define DHTTYPE DHT11

const char *supabase_url = SUPABASE_URL;
const char *anon_key = SUPABASE_ANON_KEY;

const char *ssid = WIFI_SSID;
const char *psswd = WIFI_PASSWORD;

struct Status {
  bool led_0;
  bool led_1;
  bool led_2;
  float temperature;
  float humidity;
};

Status status = {.led_0 = false, .led_1 = false, .led_2 = false, .temperature = 0.0, .humidity = 0.0};

DHT dht(DHT_PIN, DHTTYPE);

Supabase db;


DynamicJsonDocument JSONdoc(1024);
String JSON = "";

void networkSetup(){
   WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.println("Establishing connection to wifi with SSID: " + String(WIFI_SSID));

  while (WiFi.status() != WL_CONNECTED){
    delay(1000);
    Serial.println('.');
  }
  Serial.println("Connected to wifi with IP address: ");
  Serial.println(WiFi.localIP());

}

void updateLEDs(){
  DynamicJsonDocument doc(192);
  String requestedStatus = db.from("esp8266LED").select("*").eq("mac_address", WiFi.macAddress()).limit(1).doSelect();
  Serial.println(requestedStatus);
  deserializeJson(doc, requestedStatus);
  JsonObject root_0 = doc[0];
  status.led_0 = root_0["led_0"];
  status.led_1 = root_0["led_1"];
  status.led_2 = root_0["led_2"];
  doc.clear();
  db.urlQuery_reset();
  if (status.led_0)
    digitalWrite(LED_0, LOW);
  else
    digitalWrite(LED_0, HIGH);
  if (status.led_1)
    digitalWrite(LED_1, HIGH);
  else
    digitalWrite(LED_1, LOW);
  if (status.led_2)
    digitalWrite(LED_2, HIGH);
  else
    digitalWrite(LED_2, LOW);
  
}

void setup() {
  Serial.begin(9200);
  Serial.println('\n');
  pinMode(LED_0, OUTPUT);
  pinMode(LED_1, OUTPUT);
  pinMode(LED_2, OUTPUT);
  pinMode(DHT_PIN, INPUT);
  dht.begin();
  networkSetup();
  db.begin(supabase_url, anon_key);
  updateLEDs();
  
  
}

void loop() {
  updateLEDs();
  JSONdoc["mac_address"] = WiFi.macAddress();
  Serial.println(dht.readTemperature());
  dht.readTemperature(dht.readHumidity());
  JSONdoc["temperature"] = dht.readTemperature();
  JSONdoc["humidity"] = dht.readHumidity();
  serializeJson(JSONdoc, JSON);
  
  int HTTPresponseCode = db.insert("esp8266", JSON, false);
  Serial.println(HTTPresponseCode);
  db.urlQuery_reset();
  JSON.clear();
  JSONdoc.clear();
  delay(5000);
}

