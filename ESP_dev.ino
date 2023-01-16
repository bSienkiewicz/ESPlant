#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClientSecure.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <LittleFS.h>
#define SensorPin A0  // Moisture Sensor

const String DEVICE_NAME = "esp_1";
const bool POST_UPDATES = true;  // Does the sensor need to send changes to the database?

// defines for Ultrasound Sensor
#define SOUND_VELOCITY 0.034  // define sound velocity in cm/uS
const int trigPin = 12;       // D6
const int echoPin = 14;       // D5

const int relayPin = 5;  // Relay Pin (D1)

int refresh_time;
int g_moisture_percentage;
int max_moist;
int min_moist;
bool non_stop_pump;
int pump_time;
int cfg_v;

// WiFi authentication
const char* ssid = "TP-Link Home -Ext";
const char* password = "03Waldek70";
// const char* ssid = "iPhone (Bartek)";
// const char* password = "jpjpjpjp";

// API URL
const String serverName = "https://esplant.onrender.com";
const String POST_ENDPOINT = "/api/readings";
const String CONFIG_ENDPOINT = "/api/esp_1/config";

WiFiClientSecure client;
const char* fingerpr = "ED 14 5F CE AE 85 72 12 C2 17 42 3C 6D 61 F5 D3 F1 61 9B 8D";
HTTPClient http;
StaticJsonDocument<180> doc;
StaticJsonDocument<180> config_doc;

unsigned long time_now = 0;


/*************************************************************/
/* SETUP                                                     */
/*************************************************************/

void setup() {
  Serial.begin(9600);
  pinMode(BUILTIN_LED, OUTPUT);
  pinMode(trigPin, OUTPUT);   // Sets the trigPin as an Output
  pinMode(echoPin, INPUT);    // Sets the echoPin as an Input
  pinMode(relayPin, OUTPUT);  // Sets the relayPin as an Output
  digitalWrite(relayPin, HIGH);
  ledON(false);
  connectToWiFi();
  client.setFingerprint(fingerpr);

  // Download the config for the first time
  Serial.print("Downloading latest config... Response: ");
  // Loop until connection is set
  int httpResponseCode = 0;
  String config_get = "";
  Serial.println("Endpoint: " + serverName + CONFIG_ENDPOINT);
  do {
    http.begin(client, serverName + CONFIG_ENDPOINT);
    Serial.println(httpResponseCode);
    httpResponseCode = http.GET();
    Serial.println("Error connecting with API. Retrying in 3s...");
    BlinkLed(2, 500, true);
    delay(2000);
    config_get = http.getString();
    http.end();
  } while (httpResponseCode != 200);

  updateConfig(config_get);
}

/*************************************************************/
/* LOOP                                                      */
/*************************************************************/

void loop() {
  doc["sensor"] = "plant1";
  String json = "";

  readMoisture();
  readProximity();
  doc["max_moist"] = max_moist;
  doc["min_moist"] = min_moist;
  doc["pump_time"] = pump_time;
  doc["refresh_time"] = refresh_time;
  doc["non_stop_pump"] = non_stop_pump;
  serializeJson(doc, json);
  if (json[0] != '{' || WiFi.status() != WL_CONNECTED) return;

  if (non_stop_pump) {
    digitalWrite(relayPin, LOW);
  } else {
    digitalWrite(relayPin, HIGH);
  }

  if (millis() >= time_now + refresh_time) {  // SEND updates every X seconds.
    if ((g_moisture_percentage < min_moist + (min_moist * 0.2)) && !non_stop_pump) {     // if moist lower than min
      while (g_moisture_percentage < max_moist - (max_moist * 0.1)) {                    // pump until reaching max
        digitalWrite(relayPin, LOW);
        delay(1300);
        digitalWrite(relayPin, HIGH);
        delay(pump_time);
        readMoisture();
      }
    }

    PostUpdates(json);
    time_now = millis();
  } else manualUpdateConfig();


  Serial.println(json);
  delay(500);
}

/*************************************************************/
/* FUNCTIONS                                                 */
/*************************************************************/

void PostUpdates(String json) {
  if (POST_UPDATES) {
    // POST request
    http.begin(client, serverName + POST_ENDPOINT);
    http.addHeader("Content-Type", "application/json");
    int httpResponseCode = http.POST(json);
    BlinkLed(3, 100, true);
    updateConfig(http.getString());  // update config variables
    http.end();

    Serial.print("HTTP Response code: ");
    Serial.print(httpResponseCode);
    if (httpResponseCode == 200) {
      Serial.println(" // POST Request Successful.");
    } else {
      Serial.println(" // ERROR");
    }
  }
}

// Blink led x many times and decide if it stays on after blinking
void BlinkLed(int times, int del, bool stayON) {
  for (int i = 0; i < times; i++) {
    ledON(true);
    delay(del);
    ledON(false);
    delay(del);
  }
  if (stayON) ledON(true);
}

//Function to toggle the state of LED to high or low
void ledON(bool state) {
  if (state) digitalWrite(BUILTIN_LED, LOW);
  else digitalWrite(BUILTIN_LED, HIGH);
}

void readMoisture() {
  float sensorValue = analogRead(SensorPin);
  // int g_moisture_percentage = (int)(((sensorValue * -100) / 669) + (102400 / 669));
  g_moisture_percentage = map(sensorValue, 450, 1023, 100, 0);
  doc["moisture_level"] = g_moisture_percentage;
}

// Calling this function will convert the payload and transform it to config variables
void updateConfig(String payload) {
  // cleaning up JSON and deserializing
  String payload__new = payload;
  payload.remove(0, 1);                     // REMOVE [
  payload.remove(payload.length() - 1, 1);  // REMOVE ]
  char payload_arr[payload.length() + 1];
  payload.toCharArray(payload_arr, payload.length() + 1);  // convert to char array for deserialization
  deserializeJson(config_doc, payload_arr);

  if (config_doc["cfg_v"] != cfg_v) {
    refresh_time = config_doc["refresh_time"].as<long>();
    if (refresh_time < 1000) refresh_time = 1000;  // prevent setting too low
    max_moist = config_doc["max_moist"].as<long>();
    min_moist = config_doc["min_moist"].as<long>();
    pump_time = config_doc["pump_time"].as<long>();
    non_stop_pump = config_doc["non_stop_pump"].as<bool>();
    Serial.println(non_stop_pump);
    cfg_v = config_doc["cfg_v"].as<long>();
    Serial.print("\n →→→→→→→→→→ Configuration updated. REFRESHING EVERY (ms): ");
    Serial.println(refresh_time);
  }
}

// Calling this function will convert the payload and transform it to config variables
void manualUpdateConfig() {
  http.begin(client, serverName + CONFIG_ENDPOINT);
  http.addHeader("Content-Type", "application/json");
  int httpResponseCode = http.GET();

  // cleaning up JSON and deserializing
  String payload = http.getString();
  payload.remove(0, 1);                     // REMOVE [
  payload.remove(payload.length() - 1, 1);  // REMOVE ]
  char payload_arr[payload.length() + 1];
  payload.toCharArray(payload_arr, payload.length() + 1);  // convert to char array for deserialization
  deserializeJson(config_doc, payload_arr);

  if (config_doc["cfg_v"] != cfg_v) {
    refresh_time = config_doc["refresh_time"].as<long>();
    if (refresh_time < 1000) refresh_time = 1000;  // prevent setting too low
    max_moist = config_doc["max_moist"].as<long>();
    min_moist = config_doc["min_moist"].as<long>();
    pump_time = config_doc["pump_time"].as<long>();
    non_stop_pump = config_doc["non_stop_pump"].as<long>();
    cfg_v = config_doc["cfg_v"].as<long>();
    Serial.print("\n →→→→→→→→→→ Configuration updated. REFRESHING EVERY (ms): ");
    Serial.println(refresh_time);
  }
}

void readProximity() {
  long duration;
  float distanceCm;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  // Calculate the distance
  distanceCm = duration * SOUND_VELOCITY / 2;
  // doc["water_level"] = distanceCm;
  doc["water_level"] = random(30, 50);
}

void connectToWiFi() {
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  delay(1000);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    BlinkLed(1, 100, false);
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());
  BlinkLed(5, 50, true);
}