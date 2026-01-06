#ifndef EVENT_H
#define EVENT_H

#include <Arduino.h>
#include <functional>
#include <vector>

// Event types
enum class EventType {
  TOUCH_PRESSED,
  SWIPE_LEFT,
  SWIPE_RIGHT,
  ITEM_SELECTED,
  CATEGORY_CHANGED,
  LED_ANIMATION_DONE,
  SCREEN_CHANGED,
  WIFI_CONNECTED,
  WIFI_DISCONNECTED,
  DATA_RECEIVED
};

struct Event {
  EventType type;
  int param1 = -1;
  int param2 = -1;
  void* data = nullptr;
};

// Event listener callback
using EventCallback = std::function<void(const Event&)>;

class EventBus {
private:
  std::vector<std::pair<EventType, EventCallback>> listeners;

public:
  static EventBus& getInstance() {
    static EventBus instance;
    return instance;
  }

  void subscribe(EventType type, EventCallback callback) {
    listeners.push_back({type, callback});
  }

  void dispatch(const Event& event) {
    for (const auto& listener : listeners) {
      if (listener.first == event.type) {
        listener.second(event);
      }
    }
  }

  void clear() {
    listeners.clear();
  }
};

#endif