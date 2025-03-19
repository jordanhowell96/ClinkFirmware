#include <Arduino.h>
#include <HID-Project.h> // HID-Project by NicoHood
 // 40:1b:5f:6b:59:cd
 
// TODO
// refactor with method signatures?
// send invisible key press
// consider edge cases like multiple transmissions before ack etc

// debug flag

#define START_SIGNAL "START"

// enum for state?
#define NO_SIGNAL_STATE "NO_SIGNAL_STATE"
#define AWAKE_STATE "AWAKE_STATE"
#define UNLOCKED_STATE "UNLOCKED_STATE"
// #define PLAYING_STATE "PLAYING_STATE" // maybe not needed

#define STATE_EXPIRATION 2000
#define BT_EXPIRATION 5000
#define UNLOCK_DELAY 2000
#define WAKE_DELAY 2000
#define ACK_TIMEOUT 5000 // adjust

#define SERIAL_BUFFER_SIZE 32  

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

const char* pcState = NO_SIGNAL_STATE;

unsigned long lastPcContact = 0;
unsigned long lastBtContact = 0;
unsigned long lastMacSendTime = 0;
unsigned long lastAckTime = 0;

bool ackReceived = false;
bool detectedReceived = false;
String detectedMAC = "";

char serialBuffer[SERIAL_BUFFER_SIZE];

String macList = "40:1b:5f:6b:59:cd,dc:0c:2d:43:7f:51,";

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

void typeString(const char* str) {
    for (int i = 0; str[i] != '\0'; i++) {
        char c = str[i];
        Keyboard.press(c);
        delay(5);
        Keyboard.release(c);
    }
}


void initializePC() {       
  if (pcState == UNLOCKED_STATE) {
    Serial.println(START_SIGNAL);

  } else if (pcState == AWAKE_STATE) {
    typeString("6890");
    delay(UNLOCK_DELAY);

  } else if (pcState == NO_SIGNAL_STATE) {
    Keyboard.press(KEY_SPACE);
    delay(20);
    Keyboard.release(KEY_SPACE);
    delay(WAKE_DELAY);
  }
}


const char* parseSignal(const char* signal) {
  if (strcmp(signal, UNLOCKED_STATE) == 0) return UNLOCKED_STATE;
  if (strcmp(signal, AWAKE_STATE) == 0) return AWAKE_STATE;
  // if (strcmp(signal, PLAYING_STATE) == 0) return PLAYING_STATE;
  return nullptr;
}

// todo test later with only readStringUntil('\n') and check if ESP should use a similar method
void receivePCSerial() {
  static int bufferIndex = 0;

  while (Serial.available() > 0) {
    char receivedChar = Serial.read();

    if (receivedChar == '\n' || receivedChar == '\r') {
      serialBuffer[bufferIndex] = '\0';
      const char* newState = parseSignal(serialBuffer);

      if (newState != nullptr) {
        pcState = newState;
        lastPcContact = millis();
      }

      bufferIndex = 0;
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
        Serial.println("ACK received: " + message);
        ackReceived = true;
        lastAckTime = millis();
      } else {
        Serial.println("Incorrect ACK received: " + message);
    }

    } else if (message.startsWith("DETECTED:")) {  
      detectedMAC = message.substring(9);
      Serial.println("Device Found: " + detectedMAC);
      detectedReceived = true;
      lastBtContact = millis();
    }
  }
}

void sendMACList() {
  Serial1.println(macList);
  Serial.println("Sent MAC list, waiting for ACK...");
  lastMacSendTime = millis();
}


// TODO develop this
void retransmitMACList() {
  if (ackReceived && millis() > lastAckTime + ACK_TIMEOUT) {
    Serial.println("ACK timeout reached! Resetting ACK status."); // this log sucks
    ackReceived = false;
  }

  if (!ackReceived && millis() > lastMacSendTime + 5000) {
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
  sendMACList();
  retransmitMACList();

  if (detectedReceived) {  
    Serial.println("ESP32 detected device: " + detectedMAC);
    //initializePC();
    detectedReceived = false;
  }
  
  // if (pcState != NO_SIGNAL_STATE && millis() > STATE_EXPIRATION + lastPcContact) {
  //   pcState = NO_SIGNAL_STATE; 
  // }
  
  // if (millis() lastBtContact < BT_EXPIRATION + lastBtContact) {
  //   initializePC();
  // }

  delay(5000);         // todo maybe not needed
}
