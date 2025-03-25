#include <Arduino.h>
#include <HID-Project.h> 
// 40:1b:5f:6b:59:cd

// send ack
// split files
// global variables
// consider edge cases like multiple transmissions before ack etc

const char* macList = "40:1b:5f:6b:59:cd,dc:0c:2d:43:7f:51,"; // for testing
const char* passCode = "6890"; // for testing

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#define DEBUG true

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#define START_SIGNAL "START"

// enum for state?
#define NO_SIGNAL_STATE "NO_SIGNAL_STATE"
#define AWAKE_STATE "AWAKE_STATE"
#define UNLOCKED_STATE "UNLOCKED_STATE"

#define STATE_EXPIRATION 2000
#define BT_EXPIRATION 5000
#define UNLOCK_DELAY 2000
#define WAKE_DELAY 2000
#define ACK_TIMEOUT 5000 // adjust

#define SERIAL_BUFFER_SIZE 32  

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#define DEBUG_PRINTLN(x)  if (DEBUG) Serial.println(x)
#define SEND_TO_PC(x)  if (!DEBUG) Serial.println(x)
#define SEND_TO_ESP32(x)  Serial1.println(x)

enum PcState {
  NO_SIGNAL,
  AWAKE,
  UNLOCKED,
  INVALID
};

PcState pcState = NO_SIGNAL;

unsigned long lastPcContact = 0;
unsigned long lastBtContact = 0;
unsigned long lastMacSendTime = 0;
unsigned long lastAckTime = 0;

bool ackReceived = false;
bool detectedReceived = false;
String detectedMAC = "";

char serialBuffer[SERIAL_BUFFER_SIZE];

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

void typeString(const char* str) {
  for (int i = 0; str[i] != '\0' && !DEBUG; i++) {
      char c = str[i];
      Keyboard.press(c);
      delay(5);
      Keyboard.release(c);
  }
}


void initializePC() {       
  if (pcState == UNLOCKED) {
    SEND_TO_PC(START_SIGNAL);

  } else if (pcState == AWAKE) {
    typeString(passCode);
    delay(UNLOCK_DELAY);

  } else if (pcState == NO_SIGNAL) {
    Keyboard.press(0x00);
    delay(20);
    Keyboard.release(0x00);
    delay(WAKE_DELAY);
  }
}


PcState parseSignal(const char* signal) {
  if (strcmp(signal, "UNLOCKED_STATE") == 0) return UNLOCKED;
  if (strcmp(signal, "AWAKE_STATE") == 0) return AWAKE;
  return INVALID;
}


void receivePCSerial() {
  static int bufferIndex = 0;

  while (Serial.available() > 0) {
    char receivedChar = Serial.read();

    if (receivedChar == '\n' || receivedChar == '\r') {
      serialBuffer[bufferIndex] = '\0';
      bufferIndex = 0;

      PcState newState = parseSignal(serialBuffer);
      if (newState != INVALID) {
        pcState = newState;
        lastPcContact = millis();
        return;
      }

      if (strncmp(serialBuffer, "MAC:", 4) == 0) {
        macList = String(serialBuffer + 4);
        SEND_TO_ESP32(macList.c_str());
      }

      if (strchr(serialBuffer, ':') && strchr(serialBuffer, ',')) {
        DEBUG_PRINTLN("MAC list received from PC");
        SEND_TO_ESP32(serialBuffer);  // Forward to ESP32
        DEBUG_PRINTLN("MAC list forwarded to ESP32");

        ackReceived = false;
        lastMacSendTime = millis();
      }

    } else {
      if (bufferIndex < SERIAL_BUFFER_SIZE - 1) {
        serialBuffer[bufferIndex++] = receivedChar;
      }
    }
  }
}


void receiveESP32Serial() {
  while (Serial1.available() > 0) {
    String message = Serial1.readStringUntil('\n');
    message.trim();

    if (message.startsWith("ACK:")) {  
      String receivedList = message.substring(4);
      if (receivedList ==  macList) {
        DEBUG_PRINTLN("ACK received: " + message);
        ackReceived = true;
        lastAckTime = millis();
      } else {
        DEBUG_PRINTLN("Incorrect ACK received: " + message);
    }

    } else if (message.startsWith("DETECTED:")) {  
      detectedMAC = message.substring(9);
      DEBUG_PRINTLN("Device Found: " + detectedMAC);
      detectedReceived = true;
      lastBtContact = millis();
    }
  }
}


void sendMACList() {
  SEND_TO_ESP32(macList);
  DEBUG_PRINTLN("Sent MAC list, waiting for ACK...");
  lastMacSendTime = millis();
}


// TODO develop this
void retransmitMACList() {
  if (ackReceived && millis() > lastAckTime + ACK_TIMEOUT) {
    DEBUG_PRINTLN("ACK timeout reached");
    ackReceived = false;
  }

  if (!ackReceived && millis() > lastMacSendTime + 5000) {
    DEBUG_PRINTLN("Retransmitting MAC list, waiting for ACK...");
    sendMACList();
    //retryCount++; TODO use retry count to send error message to 
  }
}


void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  Keyboard.begin();
}


void loop() {     
  receivePCSerial();
  receiveESP32Serial();
  retransmitMACList();

  if (detectedReceived) {  
    DEBUG_PRINTLN("ESP32 detected device: " + detectedMAC);
    initializePC();
    detectedReceived = false;
  }
  
  if (pcState != NO_SIGNAL && millis() > STATE_EXPIRATION + lastPcContact) {
    pcState == NO_SIGNAL; 
  }
  
  if (millis() > BT_EXPIRATION + lastBtContact) {
    initializePC();
  }

  delay(5000);         // for testing
}
