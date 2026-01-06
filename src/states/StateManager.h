#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <memory>
#include "ScreenState.h"
#include "HomeScreen.h"
#include "SleepModeScreen.h"
#include "../display/DisplayManager.h"
#include "../services/LEDAnimationService.h"

class StateManager {
private:
  std::unique_ptr<ScreenState> currentState;
  std::unique_ptr<HomeScreen> homeState;
  std::unique_ptr<SleepModeScreen> sleepState;
  DisplayManager* display;
  LEDAnimationService* ledAnimation;

public:
  StateManager(DisplayManager* d, LEDAnimationService* led) 
    : display(d), ledAnimation(led) {
    
    homeState = std::make_unique<HomeScreen>(d);
    sleepState = std::make_unique<SleepModeScreen>(d, led);
    
    // START WITH SLEEPMODE (shows 6 items with circles)
    currentState = std::move(sleepState);
    currentState->onEnter();
  }

  void goToSleepMode() {
    if (currentState) {
      currentState->onExit();
    }
    
    sleepState = std::make_unique<SleepModeScreen>(display, ledAnimation);
    currentState = std::move(sleepState);
    currentState->onEnter();
  }

  void goToHomeScreen() {
    if (currentState) {
      currentState->onExit();
    }
    
    homeState = std::make_unique<HomeScreen>(display);
    currentState = std::move(homeState);
    currentState->onEnter();
  }

  void handleEvent(const Event& event) {
    if (currentState) {
      currentState->handleEvent(event);
    }
  }

  void update() {
    if (currentState) {
      currentState->update();
    }
  }

  void render() {
    if (currentState) {
      currentState->render();
    }
  }

  ScreenType getCurrentScreenType() const {
    if (currentState) {
      return currentState->getType();
    }
    return ScreenType::HOME;
  }
};

#endif