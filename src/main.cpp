#include <Arduino.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <XPT2046_Touchscreen.h>
#include <FastLED.h>
#include "config.h"
#include "Application.h"

// Touch uses separate SPI with custom pins
SPIClass touchSPI(VSPI);
XPT2046_Touchscreen touchscreen(XPT2046_CS, XPT2046_IRQ);
TFT_eSPI tft = TFT_eSPI();

// 4 LED strips - each with NUM_LEDS_PER_STRIP LEDs
CRGB ledsStrip1[NUM_LEDS_PER_STRIP];  // Plastic
CRGB ledsStrip2[NUM_LEDS_PER_STRIP];  // Paper
CRGB ledsStrip3[NUM_LEDS_PER_STRIP];  // Green/GFT
CRGB ledsStrip4[NUM_LEDS_PER_STRIP];  // Waste/Rest

std::unique_ptr<Application> app;

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nStarting Recyclebin ESP32...");

  // Initialize TFT FIRST (it uses HSPI)
  tft.init();
  tft.setRotation(0);  // Portrait mode
  Serial.println("TFT initialized");

  // Configure touch CS pin
  pinMode(XPT2046_CS, OUTPUT);
  digitalWrite(XPT2046_CS, HIGH);
  
  // Configure touch interrupt pin  
  pinMode(XPT2046_IRQ, INPUT_PULLUP);

  // Start VSPI for touchscreen with custom pins
  // VSPI default: CLK=18, MISO=19, MOSI=23, CS=5
  // We use custom pins: CLK=25, MISO=35, MOSI=32, CS=33
  touchSPI.begin(XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI, XPT2046_CS);
  
  // Initialize touchscreen
  touchscreen.begin(touchSPI);
  touchscreen.setRotation(0);
  
  Serial.println("Touchscreen initialized");
  Serial.printf("Touch pins: CS=%d, IRQ=%d, CLK=%d, MISO=%d, MOSI=%d\n",
                XPT2046_CS, XPT2046_IRQ, XPT2046_CLK, XPT2046_MISO, XPT2046_MOSI);

  // Initialize all 4 LED strips
  FastLED.addLeds<APA102, LED1_DATA_PIN, LED1_CLK_PIN, BGR>(ledsStrip1, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<APA102, LED2_DATA_PIN, LED2_CLK_PIN, BGR>(ledsStrip2, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<APA102, LED3_DATA_PIN, LED3_CLK_PIN, BGR>(ledsStrip3, NUM_LEDS_PER_STRIP);
  FastLED.addLeds<APA102, LED4_DATA_PIN, LED4_CLK_PIN, BGR>(ledsStrip4, NUM_LEDS_PER_STRIP);
  
  Serial.println("4 LED strips initialized");

  // Application - pass all 4 strip arrays
  app = std::make_unique<Application>(&tft, &touchscreen, 
                                       ledsStrip1, ledsStrip2, 
                                       ledsStrip3, ledsStrip4);
  app->init();
}

void loop() {
  app->update();
  app->render();
  delay(16);
}