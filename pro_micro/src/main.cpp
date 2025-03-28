#include <Arduino.h>
#include <HID-Project.h> 
// 40:1b:5f:6b:59:cd,dc:0c:2d:43:7f:51,

// receive ack logic
// send ack
// split files
// consider edge cases like multiple transmissions before ack etc
// remove colon from mac

const char* passCode = "6890"; // for testing

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#define DEBUG true

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#define START_SIGNAL "START"

#define NO_SIGNAL_STATE "NO_SIGNAL_STATE"
#define AWAKE_STATE "AWAKE_STATE"
#define UNLOCKED_STATE "UNLOCKED_STATE"

#define STATE_EXPIRATION 2000
#define BT_EXPIRATION 5000
#define UNLOCK_DELAY 2000
#define WAKE_DELAY 2000
#define ACK_TIMEOUT 2000

// adjust buffer sizes and show max supported size
#define PC_SERIAL_BUFFER_SIZE 32  
#define ESP_SERIAL_BUFFER_SIZE 32  
#define MAC_LIST_BUFFER_SIZE 255
#define MAC_ADDR_BUFFER_SIZE 16
#define PRINTF_BUFFER_SIZE 128

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

#define DEBUG_PRINTLN(x)  if (DEBUG) Serial.println(x)
#define DEBUG_PRINTF(...)  if (DEBUG) serialPrintf(__VA_ARGS__)
#define SEND_TO_PC(x)  if (!DEBUG) Serial.println(x)
#define SEND_TO_ESP(x)  Serial1.println(x)

enum PcState : uint8_t {
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

bool awaitingAck = false;
uint8_t retryCount = 0;
bool detectedReceived = false;

char serialBuffer[SERIAL_BUFFER_SIZE];
char currentMacList[MAC_LIST_BUFFER_SIZE] = "";
char detectedMAC[MAC_ADDR_BUFFER_SIZE];

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~//

void serialPrintf(const char* fmt, ...) {
  char buf[PRINTF_BUFFER_SIZE];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, PRINTF_BUFFER_SIZE, fmt, args);
  va_end(args);
  Serial.print(buf);
}


void typeString(const char* str) {
  for (uint8_t i = 0; str[i] != '\0' && !DEBUG; i++) {
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
  if (strcmp(signal, UNLOCKED_STATE) == 0) return UNLOCKED;
  if (strcmp(signal, AWAKE_STATE) == 0) return AWAKE;
  return INVALID;
}


template<typename SerialType>
bool readSerialLine(SerialType& serial, char* buffer, size_t bufferSize, unsigned int& bufferIndex) {
  while (serial.available() > 0) {
    char c = serial.read();

    if (c == '\n' || c == '\r') {
      buffer[bufferIndex] = '\0';
      bufferIndex = 0;
      return true;
    } else if (bufferIndex < bufferSize - 1) {
      buffer[bufferIndex++] = c;
    }
  }
  return false;
}


void sendMACList() {
  if (strlen(currentMacList) == 0) return;
  SEND_TO_ESP(currentMacList);
  DEBUG_PRINTLN("Sent MAC list, waiting for ACK...");
  lastMacSendTime = millis();
  awaitingAck = true;
}


void receivePCSerial() {
  static char serialBuffer[PC_SERIAL_BUFFER_SIZE];
  static unsigned int bufferIndex = 0;

  if (readSerialLine(Serial, serialBuffer, PC_SERIAL_BUFFER_SIZE, bufferIndex)) {
    PcState newState = parseSignal(serialBuffer);

    if (newState != INVALID) {
      pcState = newState;
      DEBUG_PRINTF("PC State Updated: %d\n", pcState);
      lastPcContact = millis();
      return;
    }

    if (strncmp(serialBuffer, "MAC:", 4) == 0) {
      strncpy(currentMacList, serialBuffer + 4, MAC_LIST_BUFFER_SIZE - 1);
      currentMacList[MAC_LIST_BUFFER_SIZE - 1] = '\0';
      DEBUG_PRINTF("MAC List Received: %s\n", currentMacList);
      retryCount = 0;
      sendMACList();
    }
  }
}


void receiveESP32Serial() {
  static char espSerialBuffer[ESP_SERIAL_BUFFER_SIZE];
  static unsigned int bufferIndex = 0;

  if (readSerialLine(Serial1, espSerialBuffer, ESP_SERIAL_BUFFER_SIZE, bufferIndex)) {
    if (strncmp(espSerialBuffer, "ACK:", 4) == 0) {
      char receivedList[MAC_LIST_BUFFER_SIZE];
      strncpy(receivedList, espSerialBuffer + 4, MAC_LIST_BUFFER_SIZE - 1);
      receivedList[MAC_LIST_BUFFER_SIZE - 1] = '\0';

      if (strcmp(receivedList, currentMacList) == 0) {
        DEBUG_PRINTLN("ACK received");
        awaitingAck = false;  
        lastAckTime = millis();
      } else {
        DEBUG_PRINTLN("Incorrect ACK received");
      }

    } else if (strncmp(espSerialBuffer, "DETECTED:", 9) == 0) {
      strncpy(detectedMAC, espSerialBuffer + 9, MAC_ADDR_BUFFER_SIZE - 1);
      detectedMAC[MAC_ADDR_BUFFER_SIZE - 1] = '\0';

      DEBUG_PRINTF("Device Found: %s\n", detectedMAC);
      detectedReceived = true;
      lastBtContact = millis();
    }
  }
}


void retransmitMACList() {
  if (awaitingAck && millis() > lastMacSendTime + ACK_TIMEOUT) {
    retryCount++;
    DEBUG_PRINTF("ACK not Received, retransmitting MAC list (Retries: %s)\n", retryCount);
    sendMACList();
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
    initializePC();
    detectedReceived = false;
  }
  
  if (pcState != NO_SIGNAL && millis() > STATE_EXPIRATION + lastPcContact) {
    pcState == NO_SIGNAL; 
  }
  
  if (millis() > BT_EXPIRATION + lastBtContact) {
    initializePC();
  }
}
