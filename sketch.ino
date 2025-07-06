#include <WiFi.h>
#include <PubSubClient.h>
#include <DHTesp.h>

// === Pin Definitions ===
#define DHTPIN 15        // DHT22 data pin
#define DHTTYPE DHT22
#define PIRPIN 12        // PIR motion sensor
#define LDRPIN 36        // AO from photoresistor to VP (GPIO 36)
#define LED_FAN 13       // Fan simulation LED
#define LED_LIGHT 14     // Light simulation LED

// === Sensor & Network Setup ===
DHTesp dht;

const char* ssid = "Wokwi-GUEST";
const char* password = "";
const char* mqtt_server = "broker.hivemq.com";

WiFiClient espClient;
PubSubClient client(espClient);
unsigned long lastMsg = 0;

void setup_wifi() {
  delay(10);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }
}

void setup() {
  Serial.begin(115200);
  dht.setup(DHTPIN, DHTesp::DHT22);

  pinMode(PIRPIN, INPUT);
  pinMode(LED_FAN, OUTPUT);
  pinMode(LED_LIGHT, OUTPUT);

  setup_wifi();
  client.setServer(mqtt_server, 1883);
}

void loop() {
  if (!client.connected()) {
    while (!client.connected()) {
      if (client.connect("ESP32Client")) {
        // Connected
      } else {
        delay(5000);
      }
    }
  }

  client.loop();

  unsigned long now = millis();
  if (now - lastMsg > 5000) {  // Every 5 seconds
    lastMsg = now;

    TempAndHumidity data = dht.getTempAndHumidity();
    float temp = data.temperature;
    float hum = data.humidity;
    int light = analogRead(LDRPIN);  // Photoresistor value
    int motion = digitalRead(PIRPIN);  // PIR motion state

    // === Logic to control LEDs ===
    digitalWrite(LED_FAN, temp > 30 ? HIGH : LOW); // Fan on if temp > 30
    digitalWrite(LED_LIGHT, (motion == HIGH && light < 1000) ? HIGH : LOW); // Light on if dark + motion

    // === Publish MQTT message ===
    char msg[128];
    snprintf(msg, sizeof(msg),
             "{\"temperature\": %.2f, \"humidity\": %.2f, \"light\": %d, \"motion\": %d}",
             temp, hum, light, motion);
    client.publish("home/sensors", msg);

    Serial.println(msg);
  }
}