#include <WiFi.h>           // WiFi functions
#include <PubSubClient.h>   // MQTT protocol
#include <DHTesp.h>         // DHT sensor

//* Pin Definitions
#define DHTPIN 15        // DHT22 sensor data pin
#define DHTTYPE DHT22
#define PIRPIN 12        // PIR motion sensor pin
#define LDRPIN 36        // Photoresistor (LDR) analog pin
#define LED_FAN 13       // LED simulating a fan
#define LED_LIGHT 14     // LED simulating a light

//* Sensor & Network Setup
DHTesp dht;  // DHT sensor object

const char* ssid = "Wokwi-GUEST";               // WiFi network name
const char* password = "";                      // WiFi password (empty for Wokwi)
const char* mqtt_server = "broker.hivemq.com";  // MQTT broker address

WiFiClient espClient;           // WiFi client for MQTT
PubSubClient client(espClient); // MQTT client object
unsigned long lastMsg = 0;      // Timestamp for last MQTT message

// Connects to WiFi network
void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500); // Wait until connected
  }
}

void setup() {
  // Start serial communication for debugging
  Serial.begin(115200);
  // Initialize DHT sensor
  dht.setup(DHTPIN, DHTesp::DHT22);

  pinMode(PIRPIN, INPUT);     // Set PIR pin as input
  pinMode(LED_FAN, OUTPUT);   // Set fan LED as output
  pinMode(LED_LIGHT, OUTPUT); // Set light LED as output

  // Connect to WiFi
  setup_wifi();
  // Set MQTT broker and port
  client.setServer(mqtt_server, 1883);
}

void loop() {
  // Ensure MQTT connection is active
  if (!client.connected()) {
    while (!client.connected()) {
      if (client.connect("ESP32Client")) {
        // Connected to MQTT broker
      } else {
        // Wait and retry if not connected
        delay(5000);
      }
    }
  }

  client.loop(); // Handle MQTT client background tasks

  unsigned long now = millis();
  if (now - lastMsg > 5000) {  // Every 5 seconds
    lastMsg = now;

    //* Read sensor data
    TempAndHumidity data = dht.getTempAndHumidity();
    float temp = data.temperature;
    float hum = data.humidity;
    int light = analogRead(LDRPIN);    // Read light level from LDR
    int motion = digitalRead(PIRPIN);  // Read motion from PIR

    //* Control LEDs based on sensor data
    // Turn on fan LED if temperature is above 30°C
    digitalWrite(LED_FAN, temp > 30 ? HIGH : LOW);
    Serial.println(temp > 30 ? "Fan ON" : "Fan OFF");

    // Turn on light LED if motion is detected and it's dark
    digitalWrite(LED_LIGHT, (motion == HIGH && light < 1000) ? HIGH : LOW);
    Serial.println((motion == HIGH && light < 1000) ? "Light ON" : "Light OFF");

    //* Publish sensor data to MQTT topic
    char msg[128];
    snprintf(msg, sizeof(msg),
             "{\"temperature\": %.2f, \"humidity\": %.2f, \"light\": %d, \"motion\": %d}",
             temp, hum, light, motion);
    client.publish("home/sensors", msg); // Send data to MQTT broker

    Serial.println(msg); // Print the message to serial monitor for debugging
  }
}