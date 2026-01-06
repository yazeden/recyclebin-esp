#ifndef INTERACTION_SERVICE_H
#define INTERACTION_SERVICE_H

#include <Arduino.h>
#include <queue>

struct Interaction {
  int itemId;
  unsigned long timestamp;
  bool success;
};

class InteractionService {
private:
  std::queue<Interaction> pendingInteractions;
  unsigned long lastInteractionTime = 0;
  const unsigned long DEBOUNCE_TIME = 500; // ms

  InteractionService() {}

public:
  static InteractionService& getInstance() {
    static InteractionService instance;
    return instance;
  }

  // Check of genoeg tijd is verstreken sinds laatste interactie
  bool canInteract() {
    return (millis() - lastInteractionTime) > DEBOUNCE_TIME;
  }

  // Registreer een nieuwe interactie
  void recordInteraction(int itemId) {
    if (!canInteract()) {
      Serial.println("Interaction blocked by debounce");
      return;
    }

    Interaction interaction{itemId, millis(), false};
    pendingInteractions.push(interaction);
    lastInteractionTime = millis();

    Serial.printf("Interaction recorded: item %d at %lu\n", 
                  itemId, millis());
  }

  // Haal volgende pending interactie op
  Interaction getPendingInteraction() {
    if (pendingInteractions.empty()) {
      return {-1, 0, false};
    }
    Interaction front = pendingInteractions.front();
    pendingInteractions.pop();
    return front;
  }

  // Zijn er nog pending interactions?
  bool hasPendingInteractions() const {
    return !pendingInteractions.empty();
  }

  // Verwijder alle pending interactions
  void clearPending() {
    while (!pendingInteractions.empty()) {
      pendingInteractions.pop();
    }
  }

  // Get aantal pending interactions
  int getPendingCount() const {
    return pendingInteractions.size();
  }

  // Reset debounce timer (voor testen)
  void resetDebounceTimer() {
    lastInteractionTime = 0;
  }
};

#endif