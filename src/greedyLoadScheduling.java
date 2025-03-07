// Structure to hold device information (name, threshold, state)
struct Device {
  String name;         // Name of the device (Fan, Light, etc.)
  int threshold;       // Battery-off threshold (percentage)
  bool state;          // Device state: true for ON, false for OFF
};

Device devices[4];  // Array of devices, can handle up to 4 devices

// Relay pin assignments for controlling the devices
const int relayPins[] = {5, 18, 19, 21}; // Pins for 4 devices (use appropriate pins for your setup)

// Function to handle greedy load scheduling: turn off devices based on priority (threshold)
void greedyLoadScheduling(int batteryPercentage) {
  // Sort the devices based on their thresholds (lowest threshold first)
  sortDevicesByThreshold();

  // Iterate over the devices in the sorted order
  for (int i = 0; i < 4; i++) {
    // If the battery percentage is lower than the device's threshold and the device is ON
    if (batteryPercentage < devices[i].threshold && devices[i].state == true) {
      // Turn off the device (relay OFF)
      digitalWrite(relayPins[i], LOW);
      devices[i].state = false;  // Update the state to OFF
      Serial.println(devices[i].name + " turned off due to low battery");
    }
  }
}

// Function to sort devices by their battery-off thresholds (ascending order)
void sortDevicesByThreshold() {
  // Simple bubble sort algorithm to sort devices by threshold
  for (int i = 0; i < 3; i++) {
    for (int j = i + 1; j < 4; j++) {
      if (devices[i].threshold > devices[j].threshold) {
        // Swap devices[i] and devices[j] if the threshold of devices[i] > devices[j]
        Device temp = devices[i];
        devices[i] = devices[j];
        devices[j] = temp;
      }
    }
  }
}

// Function to read the battery level (this will be replaced with actual sensor reading code)
int readBatteryLevel() {
  // Simulate reading battery level (In real system, use analogRead for actual sensor data)
  return random(0, 100);  // Return a random battery level between 0 and 100 for simulation
}

// Function to publish the battery level to MQTT broker (or update display, etc.)
void publishBatteryLevel(int batteryPercentage) {
  char batteryStr[8];
  itoa(batteryPercentage, batteryStr, 10);  // Convert integer to string
  client.publish("inverter/battery", batteryStr);  // Publish battery percentage to the broker
}

void setup() {
  // Start Serial communication for debugging
  Serial.begin(115200);
  
  // Initialize relay pins as output
  for (int i = 0; i < 4; i++) {
    pinMode(relayPins[i], OUTPUT);
    digitalWrite(relayPins[i], LOW);  // Start all devices as OFF
    devices[i].state = false;  // Set initial state of all devices to OFF
  }

  // Initial setup for devices: these values will be updated dynamically from the app
  devices[0].name = "Fan";
  devices[0].threshold = 80;
  devices[1].name = "Fridge";
  devices[1].threshold = 60;
  devices[2].name = "Light";
  devices[2].threshold = 40;
  devices[3].name = "Heater";
  devices[3].threshold = 50;
}

void loop() {
  // Simulate reading the battery level
  int batteryLevel = readBatteryLevel();
  
  // Publish the battery level to MQTT broker (or update the app)
  publishBatteryLevel(batteryLevel);
  
  // Apply greedy load scheduling to turn off devices if battery is low
  greedyLoadScheduling(batteryLevel);
  
  delay(1000);  // Wait for 1 second before checking again
}
