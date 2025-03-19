#include <Arduino.h>
#include <BluetoothSerial.h>
#include <vector>

// print version and other startup info
// debug flag
// recieve ack (shared library for methods?)

#define BT_NAME "Clink"
#define DEBUG_BAUD 9600
#define SERIAL_BAUD 9600
#define BT_TIMEOUT 10000
#define BT_FAIL_TIMEOUT 1000
#define LED_TIMEOUT 1000

#define INTERNAL_LED 2
#define RX_PIN 16
#define TX_PIN 17

BluetoothSerial SerialBT;
std::vector<String> targetMACs;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

bool btFailed;
unsigned long btFailedTime = 0;
unsigned long btStartTime = 0;
unsigned long ledOnTime = 0;


void btAdvertisedDeviceFound(BTAdvertisedDevice *pDevice) {
  Serial.printf("Found device: %s\n", pDevice->toString().c_str());
  String deviceMAC = pDevice->getAddress().toString();
  deviceMAC.toLowerCase(); 

  for (String mac : targetMACs) {
    if (deviceMAC.equals(mac)) {
      Serial.printf("Detected target device with MAC: %s\n", deviceMAC.c_str());
      Serial1.println("DETECTED:" + deviceMAC);
      digitalWrite(INTERNAL_LED, HIGH);
      ledOnTime = millis();
      break;
    }
  }
}

void startAsyncDiscovery() {
  if (SerialBT.discoverAsync(btAdvertisedDeviceFound)) {
    Serial.println("Bluetooth discovery began");
    btStartTime = millis();
    btFailed = false;
  } else {
    Serial.println("Error starting Bluetooth discovery");
    btFailedTime = millis();
    btFailed = true;
  }
}

void updateMACList(String macList) {
  portENTER_CRITICAL(&mux);
  targetMACs.clear();
  int start = 0, end = 0;

  while ((end = macList.indexOf(',', start)) != -1) {
    String mac = macList.substring(start, end);
    mac.toLowerCase();
    targetMACs.push_back(mac);
    start = end + 1;
  }
  portEXIT_CRITICAL(&mux);
}


void receiveMACUpdates() {
  String latestData = "";
  while (Serial1.available()) {
    latestData = Serial1.readStringUntil('\n');
  }
  latestData.trim();

  if (!latestData.isEmpty()) {
    Serial.println("Received MAC list: " + latestData);
    Serial1.println("ACK:" + latestData);
    updateMACList(latestData);
  }
}


void checkLedOff() {
  if (millis() > ledOnTime + LED_TIMEOUT) {
    digitalWrite(INTERNAL_LED, LOW);
  }
}


void checkBTFailed() {
  if (btFailed && millis() > btFailedTime + BT_FAIL_TIMEOUT) {
    Serial.println("Retrying Bluetooth discovery...");
    startAsyncDiscovery();
  }
}


void refreshBT() {
  if (millis() > btStartTime + BT_TIMEOUT) {
    SerialBT.discoverAsyncStop();
    Serial.println("Restarting Bluetooth discovery... ");
    startAsyncDiscovery();
  }
}


void setup() {
  Serial.begin(DEBUG_BAUD);
  Serial1.begin(SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  pinMode(INTERNAL_LED, OUTPUT);
  SerialBT.begin(BT_NAME);
  Serial.println("Starting Bluetooth discovery... ");
  startAsyncDiscovery();
}


void loop() {
  receiveMACUpdates();
  checkBTFailed();
  refreshBT();
  checkLedOff();
}