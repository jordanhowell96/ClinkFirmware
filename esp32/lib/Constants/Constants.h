#pragma once

#ifndef DEBUG
#define DEBUG false
#endif

#define BT_NAME "Clink"
#define DEBUG_BAUD 9600
#define SERIAL_BAUD 9600
#define BT_TIMEOUT 10000
#define BT_FAIL_TIMEOUT 1000
#define LED_TIMEOUT 1000

#define INTERNAL_LED 2
#define RX_PIN 16
#define TX_PIN 17

#define DEBUG_PRINTLN(x)  if (DEBUG) Serial.println(x)
#define DEBUG_PRINTF(...)  if (DEBUG) Serial.printf(__VA_ARGS__)
#define SEND_TO_PRO_MIC(x)  Serial1.println(x)
