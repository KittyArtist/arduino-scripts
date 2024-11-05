#include <WiFiS3.h>
#include <ArduinoMqttClient.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include <PulseSensorPlayground.h>
#include "secret.h"

#define ONE_WIRE_BUS 2  // Pin where the DS18B20 is connected
#define PULSE_PIN A0    // Pulse sensor connected to analog pin A0
#define LED_PIN 13      // On-board LED for heartbeat indication
#define THRESHOLD 550   // Threshold for heartbeat detection

// WiFi and MQTT configuration
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
const char* mqtt_server = IP_ADRESS; // located in secret.h
const int mqtt_port = 1883;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);
PulseSensorPlayground pulseSensor;  // Create PulseSensor object
OneWire oneWire(ONE_WIRE_BUS);       // Create a OneWire instance
DallasTemperature sensors(&oneWire);  // Create DallasTemperature object

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(SECRET_SSID, SECRET_PASS);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Connected to WiFi!");
}

void connectMQTT() {
  Serial.print("Connecting to MQTT broker");
  mqttClient.setId("PulseTemperatureNode");
  while (!mqttClient.connect(mqtt_server, mqtt_port)) {
    Serial.print(".");
    delay(1000);
  }
  Serial.println(" Connected to MQTT broker");
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  connectMQTT();
  
  pulseSensor.analogInput(PULSE_PIN);
  pulseSensor.blinkOnPulse(LED_PIN);
  pulseSensor.setThreshold(THRESHOLD);
  sensors.begin(); // Initialize the DS18B20 sensor
  
  if (pulseSensor.begin()) {
    Serial.println("PulseSensor object created and ready!");
  }
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqttClient.connected()) {
    connectMQTT();
  }

  // BPM
  int myBPM = pulseSensor.getBeatsPerMinute(); // Get the BPM value
  
  if (pulseSensor.sawStartOfBeat()) {
    Serial.println("♥ A HeartBeat Happened!");
    Serial.print("BPM: ");
    Serial.println(myBPM);
    
    // Publish BPM if valid
    if (myBPM > 0) {
      mqttClient.beginMessage("sensors/pulse");
      mqttClient.print(myBPM);
      mqttClient.endMessage();
      Serial.print("Published BPM: ");
      Serial.println(myBPM);
    }
  }

  // Request temperature readings
  sensors.requestTemperatures(); 
  float temperature = sensors.getTempCByIndex(0); // Get temperature from the first sensor

  // Check if the temperature reading is valid
  //If you don't add this if, it'll submit -127.0 as temperature when no temperature sensor is connected
  if (temperature == -127.0) {
    temperature = NAN; // Set to NaN (Not a Number) to indicate no valid reading
  }

  // Publish the temperature if it's valid
  if (!isnan(temperature)) {
    mqttClient.beginMessage("sensors/body_temperature");
    mqttClient.print(temperature);
    mqttClient.endMessage();
    Serial.print("Published Temperature: ");
    Serial.println(temperature);
  } else {
    Serial.println("Failed to read body temperature!");
  }

  delay(2000); // Delay between readings
}
