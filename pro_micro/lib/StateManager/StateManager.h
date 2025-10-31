#pragma once
#include <Arduino.h>
#include <Constants.h>

enum PcState : uint8_t {
  NO_SIGNAL,
  AWAKE,
  UNLOCKED,
  INVALID
};

class StateManager {
public:
  StateManager();

  void updateFromSignal(const char* signal);
  void tick();
  PcState getState() const;
  bool isStateExpired() const;

  void markContact();  // Reset timer manually

private:
  PcState currentState;
  unsigned long lastContactTime;

  PcState parseSignal(const char* signal);
};
