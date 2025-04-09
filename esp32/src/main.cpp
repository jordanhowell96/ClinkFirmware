#include <Arduino.h>
#include <BluetoothSerial.h>
#include <vector>
#include <Constants.h>

// fix jumbled characters at the start of the serial monitor
// bluetooth discovery is restarting early
// receive ack (shared library for methods?)
// receive serial method

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

BluetoothSerial SerialBT;
std::vector<String> targetMACs;
portMUX_TYPE mux = portMUX_INITIALIZER_UNLOCKED;

bool btFailed;
unsigned long btFailedTime = 0;
unsigned long btStartTime = 0;
unsigned long ledOnTime = 0;

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

void btAdvertisedDeviceFound(BTAdvertisedDevice *pDevice) {
  DEBUG_PRINTF("Found device: %s\n", pDevice->toString().c_str());
  String deviceMAC = pDevice->getAddress().toString();
  deviceMAC.toLowerCase(); 

  for (String mac : targetMACs) {
    if (deviceMAC.equals(mac)) {
      DEBUG_PRINTF("Detected target device with MAC: %s\n", deviceMAC.c_str());
      SEND_TO_PRO_MIC("DETECTED:" + deviceMAC);
      digitalWrite(INTERNAL_LED, HIGH);
      ledOnTime = millis();
      break;
    }
  }
}

void startAsyncDiscovery() {
  if (SerialBT.discoverAsync(btAdvertisedDeviceFound)) {
    DEBUG_PRINTLN("Bluetooth discovery began");
    btStartTime = millis();
    btFailed = false;
  } else {
    DEBUG_PRINTLN("Error starting Bluetooth discovery");
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
    DEBUG_PRINTF("Received MAC list: %s\n", latestData.c_str());
    SEND_TO_PRO_MIC("ACK:" + latestData);
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
    DEBUG_PRINTLN("Retrying Bluetooth discovery...");
    startAsyncDiscovery();
  }
}


void refreshBT() {
  if (millis() > btStartTime + BT_TIMEOUT) {
    SerialBT.discoverAsyncStop();
    DEBUG_PRINTLN("Restarting Bluetooth discovery... ");
    startAsyncDiscovery();
  }
}


void setup() {
  Serial.begin(DEBUG_BAUD);
  Serial1.begin(SERIAL_BAUD, SERIAL_8N1, RX_PIN, TX_PIN);
  SerialBT.begin(BT_NAME);
  DEBUG_PRINTLN("Starting Bluetooth discovery... ");
  pinMode(INTERNAL_LED, OUTPUT);
  startAsyncDiscovery();
}


void loop() {
  receiveMACUpdates();
  checkBTFailed();
  refreshBT();
  checkLedOff();
}