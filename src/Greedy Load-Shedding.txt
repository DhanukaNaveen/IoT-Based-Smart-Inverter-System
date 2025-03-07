// Include necessary libraries
#include <WiFi.h>              // For Wi-Fi connection
#include <PubSubClient.h>       // For MQTT communication

// Wi-Fi and MQTT broker details
const char* ssid = "your-ssid";       // Replace with your Wi-Fi SSID
const char* password = "your-password"; // Replace with your Wi-Fi password
const char* mqttServer = "broker.hivemq.com"; // Use your MQTT broker (e.g., public HiveMQ broker)
const int mqttPort = 1883;           // Default MQTT port

// MQTT Client setup
WiFiClient espClient;
PubSubClient client(espClient);

// Define the relay pins for the 4 outputs
const int relayPins[] = {5, 18, 19, 21}; // 4 relay pins for 4 devices (use appropriate pins for your setup)

// Structure to hold device information
struct Device {
  String name;         // Name of the device (Fan, Light, etc.)
  int threshold;       // Battery-off threshold (percentage)
  bool state;          // Device state: true for ON, false for OFF
};

Device devices[4];  // Array of devices, with 4 possible devices

// Wi-Fi connection setup
void setupWiFi() {
  Serial.begin(115200);
  delay(10);
  WiFi.begin(ssid, password);  // Connect to Wi-Fi
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");
}

// MQTT client setup
void setupMQTT() {
  client.setServer(mqttServer, mqttPort); // Connect to the MQTT broker
  client.setCallback(callback); // Set callback function for MQTT messages
}

// Reconnect to the MQTT broker if the connection is lost
void reconnect() {
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Try to connect to the MQTT broker
    if (client.connect("ESP32Client")) {
      Serial.println("connected");
      client.subscribe("inverter/control");  // Subscribe to the control topic
      client.subscribe("inverter/device");   // Subscribe to the topic where device updates come from
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      delay(5000);
    }
  }
}

// Callback function to handle received MQTT messages
void callback(char* topic, byte* payload, unsigned int length) {
  String message = "";
  for (int i = 0; i < length; i++) {
    message += (char)payload[i];
  }
  
  // If a control message is received, act based on the message
  if (String(topic) == "inverter/control") {
    handleControlMessage(message);  // Handle commands like "fan_on", "light_off", etc.
  } else if (String(topic) == "inverter/device") {
    updateDeviceSettings(message);  // Update device settings (name, threshold, etc.)
  }
}

// Function to handle device control messages (e.g., fan_on, light_off)
void handleControlMessage(String message) {
  // Iterate over devices and check if the message corresponds to a device
  for (int i = 0; i < 4; i++) {
    if (message == devices[i].name + "_on") {
      digitalWrite(relayPins[i], HIGH);  // Turn device ON
      devices[i].state = true;  // Update the device state
      Serial.println(devices[i].name + " turned on");
    } else if (message == devices[i].name + "_off") {
      digitalWrite(relayPins[i], LOW);   // Turn device OFF
      devices[i].state = false;  // Update the device state
      Serial.println(devices[i].name + " turned off");
    }
  }
}

// Function to update device settings (name and threshold) from the app
void updateDeviceSettings(String settings) {
  // Example message format: "fan,80" -> "name,threshold"
  int separatorIndex = settings.indexOf(',');
  if (separatorIndex != -1) {
    String name = settings.substring(0, separatorIndex);
    int threshold = settings.substring(separatorIndex + 1).toInt();
    
    // Find the device and update its settings
    for (int i = 0; i < 4; i++) {
      if (devices[i].name == name || devices[i].name == "") {  // Match device by name or empty name
        devices[i].name = name;           // Update the name
        devices[i].threshold = threshold; // Update the threshold value
        Serial.println("Updated " + devices[i].name + " threshold to " + String(threshold) + "%");
        return;  // Exit the function after updating the device
      }
    }
  }
}

// Read the battery level and return its percentage
int readBatteryLevel() {
  // Read analog value from battery sensor
  int sensorValue = analogRead(A0); // Assuming the battery voltage is connected to A0
  // Convert sensor value to a percentage (0-1023 -> 0-100%)
  int batteryPercentage = map(sensorValue, 0, 1023, 0, 100);
  return batteryPercentage;
}

// Publish the battery level to MQTT broker
void publishBatteryLevel(int batteryPercentage) {
  char batteryStr[8];
  itoa(batteryPercentage, batteryStr, 10);  // Convert integer to string
  client.publish("inverter/battery", batteryStr);  // Publish battery percentage to the broker
}

// Check battery level and turn off devices if necessary
void checkDeviceStatus(int batteryPercentage) {
  // Iterate over devices and check if the battery level is below the threshold
  for (int i = 0; i < 4; i++) {
    if (batteryPercentage < devices[i].threshold && devices[i].state == true) {
      // If the battery is below the threshold and the device is ON, turn it OFF
      digitalWrite(relayPins[i], LOW);  // Turn device OFF
      devices[i].state = false;  // Update the device state
      Serial.println(devices[i].name + " turned off due to low battery");
    }
  }
}

void setup() {
  // Setup Wi-Fi connection and MQTT broker connection
  setupWiFi();
  setupMQTT();

  // Initialize relay pins as output
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);  // Start all devices as OFF
    devices[i].state = false;  // Set initial state of all devices to OFF
  }

  // Devices are initially empty, they will be updated dynamically via MQTT
  for (int i = 0; i < 4; i++) {
    devices[i].name = "";  // Initially empty
    devices[i].threshold = 0; // Initially no threshold
  }
}

void loop() {
  if (!client.connected()) {
    reconnect();  // Reconnect to MQTT if disconnected
  }

  client.loop();  // Handle incoming MQTT messages

  // Read battery level and publish to MQTT
  int batteryLevel = readBatteryLevel();
  publishBatteryLevel(batteryLevel);

  // Check devices and turn them off if battery is low
  checkDeviceStatus(batteryLevel);

  delay(1000);  // Wait for 1 second before checking again
}
