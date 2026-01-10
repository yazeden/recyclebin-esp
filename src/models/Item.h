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
    // Case-insensitive vergelijking
    String s = String(str);
    s.toLowerCase();
    
    if (s == "plastic") return ItemCategory::PLASTIC;
    if (s == "paper" || s == "papier") return ItemCategory::PAPER;
    if (s == "green" || s == "groen" || s == "gft") return ItemCategory::GREEN;
    if (s == "waste" || s == "rest" || s == "restafval") return ItemCategory::WASTE;
    return ItemCategory::WASTE;
  }
};

#endif