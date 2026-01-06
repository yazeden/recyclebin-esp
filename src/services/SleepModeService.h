#ifndef SLEEP_MODE_SERVICE_H
#define SLEEP_MODE_SERVICE_H

#include <Arduino.h>
#include "../events/Event.h"

class SleepModeService {
private:
  bool isActive = false;
  unsigned long lastActivityTime = 0;
  unsigned long SLEEP_TIMEOUT = 10000; // 10 seconds inactivity
  
public:
  static SleepModeService& getInstance() {
    static SleepModeService instance;
    return instance;
  }

  void recordActivity() {
    lastActivityTime = millis();
    
    if (isActive) {
      deactivate();
    }
  }

  void update() {
    if (isActive) return; 

    unsigned long inactiveTime = millis() - lastActivityTime;
    
    if (inactiveTime > SLEEP_TIMEOUT) {
      activate();
    }
  }

  void activate() {
    if (isActive) return;
    
    isActive = true;
    Serial.println("Sleep mode: ACTIVATED");
    
    Event event;
    event.type = EventType::SCREEN_CHANGED;
    event.param1 = 1; // 1 = sleep mode
    EventBus::getInstance().dispatch(event);
  }

  void deactivate() {
    if (!isActive) return;
    
    isActive = false;
    Serial.println("Sleep mode: DEACTIVATED");
    
    Event event;
    event.type = EventType::SCREEN_CHANGED;
    event.param1 = 0; // 0 = normal mode
    EventBus::getInstance().dispatch(event);
  }

  bool getIsActive() const {
    return isActive;
  }

  void setSleepTimeout(unsigned long ms) {
    SLEEP_TIMEOUT = ms;
  }
};

#endif