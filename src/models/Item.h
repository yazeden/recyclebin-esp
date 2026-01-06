#ifndef ITEM_H
#define ITEM_H

#include <Arduino.h>
#include <cstring>

enum class ItemCategory {
  PLASTIC,
  PAPER,
  GREEN,
  WASTE
};

struct Item {
  int id;
  String name;           
  ItemCategory category;
  uint16_t color;
  bool isDirty;          
  bool canBeDirty;       
  int ledIndex;
  String description;    

  static const char* categoryToString(ItemCategory cat) {
    switch (cat) {
      case ItemCategory::PLASTIC: return "Plastic";
      case ItemCategory::PAPER: return "Papier";
      case ItemCategory::GREEN: return "Groen";
      case ItemCategory::WASTE: return "Afval";
      default: return "Onbekend";
    }
  }

  static ItemCategory stringToCategory(const char* str) {
    if (strcmp(str, "plastic") == 0) return ItemCategory::PLASTIC;
    if (strcmp(str, "paper") == 0) return ItemCategory::PAPER;
    if (strcmp(str, "green") == 0) return ItemCategory::GREEN;
    if (strcmp(str, "waste") == 0) return ItemCategory::WASTE;
    return ItemCategory::WASTE;
  }
};

#endif