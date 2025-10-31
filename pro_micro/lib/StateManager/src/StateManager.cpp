#include <StateManager.h>

StateManager::StateManager()
  : currentState(NO_SIGNAL), lastContactTime(0) {}

void StateManager::updateFromSignal(const char* signal) {
  PcState newState = parseSignal(signal);
  if (newState != INVALID) {
    currentState = newState;
    lastContactTime = millis();
    DEBUG_PRINTF("PC State Updated: %d\n", currentState);
  }
}

void StateManager::tick() {
  if (currentState != NO_SIGNAL &&
      millis() - lastContactTime > STATE_EXPIRATION) {
    currentState = NO_SIGNAL;
    DEBUG_PRINTLN("PC state expired, reverting to NO_SIGNAL.");
  }
}

bool StateManager::isStateExpired() const {
  return (millis() - lastContactTime > STATE_EXPIRATION);
}

PcState StateManager::getState() const {
  return currentState;
}

void StateManager::markContact() {
  lastContactTime = millis();
}

PcState StateManager::parseSignal(const char* signal) {
  if (strcmp(signal, UNLOCKED_STATE) == 0) return UNLOCKED;
  if (strcmp(signal, AWAKE_STATE) == 0) return AWAKE;
  return INVALID;
}