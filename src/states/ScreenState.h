#ifndef SCREEN_STATE_H
#define SCREEN_STATE_H

#include "../events/Event.h"

enum class ScreenType {
  HOME,
  SLEEP,
  CATEGORY,
  ITEM_DETAIL,
  LOADING,
  ERROR,
  SEARCH
};

class ScreenState {
public:
  virtual ~ScreenState() = default;
  
  virtual ScreenType getType() const = 0;
  virtual void onEnter() = 0;
  virtual void onExit() = 0;
  virtual void handleEvent(const Event& event) = 0;
  virtual void update() = 0;
  virtual void render() = 0;
};

#endif