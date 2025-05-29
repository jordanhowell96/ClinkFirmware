#pragma once

#ifndef DEBUG
#define DEBUG false
#endif

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

#define DEBUG_PRINTLN(x)  if (DEBUG) Serial.println(x)
#define DEBUG_PRINTF(...)  if (DEBUG) serialPrintf(Serial, __VA_ARGS__)
#define SEND_TO_PC(x)  if (!DEBUG) Serial.println(x)
#define SEND_TO_ESP(x)  Serial1.println(x)
#define SEND_TO_ESP_F(...)  Serial1.printf(Serial1, __VA_ARGS__)
