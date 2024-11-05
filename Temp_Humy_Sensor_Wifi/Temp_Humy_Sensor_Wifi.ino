#include <WiFiS3.h>
#include <ArduinoMqttClient.h>
#include "DHT.h"  // Use the appropriate DHT library for your sensor
#include "secret.h"

#define DHTPIN 7 // what pin we're connected to

// Uncomment whatever type you're using!
#define DHTTYPE DHT11 // DHT 11
//#define DHTTYPE DHT22 // DHT 22 (AM2302)
//#define DHTTYPE DHT21 // DHT 21 (AM2301)

// Initialize DHT sensor for normal 16mhz Arduino
DHT dht(DHTPIN, DHTTYPE);

// WiFi and MQTT configuration
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
const char* mqtt_server = IP_ADDRESS; // located in secret.h
const int mqtt_port = 1883;

WiFiClient wifiClient;
MqttClient mqttClient(wifiClient);

void connectWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Connected to WiFi!");
}

void connectMQTT() {
  Serial.print("Connecting to MQTT broker");
  mqttClient.setId("TempHumidityNode");
  while (!mqttClient.connect(mqtt_server, mqtt_port)) {
    delay(1000);
    Serial.print(".");
  }
  Serial.println(" Connected to MQTT broker");
}

void setup() {
  Serial.begin(115200);
  connectWiFi();
  connectMQTT();
  dht.begin();  // Initialize the DHT sensor
}

void loop() {
  if (WiFi.status() != WL_CONNECTED) {
    connectWiFi();
  }
  if (!mqttClient.connected()) {
    connectMQTT();
  }

  // Reading temperature and humidity takes about 250 milliseconds!
  float humidity = dht.readHumidity();      // Read humidity
  float temperature = dht.readTemperature(); // Read temperature

  // Publish temperature and humidity together if both readings are valid
  if (!isnan(humidity) && !isnan(temperature)) {
    mqttClient.beginMessage("sensors/temperature_humidity");
    mqttClient.print("Temperature: ");
    mqttClient.print(temperature);
    mqttClient.print(", Humidity: ");
    mqttClient.print(humidity);
    mqttClient.endMessage();
    Serial.print("Published Temperature and Humidity: ");
    Serial.print(temperature);
    Serial.print(", ");
    Serial.println(humidity);
  } else {
    // If one value is missing, publish the available value
    if (!isnan(humidity)) {
      mqttClient.beginMessage("sensors/humidity");
      mqttClient.print(humidity);
      mqttClient.endMessage();
      Serial.print("Published Humidity: ");
      Serial.println(humidity);
    }
    
    if (!isnan(temperature)) {
      mqttClient.beginMessage("sensors/temperature");
      mqttClient.print(temperature);
      mqttClient.endMessage();
      Serial.print("Published Temperature: ");
      Serial.println(temperature);
    }
  }

  // Wait a few seconds between measurements.
  delay(2000);
}
