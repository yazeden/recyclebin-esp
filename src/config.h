#ifndef CONFIG_H
#define CONFIG_H

// ===========================================
// WIFI & DATABASE API
// ===========================================
#define WIFI_SSID       "SmarTT"
#define WIFI_PASS       "MLYNKnapp"
#define API_HOST        "4.231.92.177"
#define API_PORT        8080
#define DEVICE_LOCATION "Heidelberglaan"  // Location name for this device

// ===========================================
// DISPLAY
// ===========================================
#define PORTRAIT_WIDTH 240
#define PORTRAIT_HEIGHT 320
#define SCREEN_WIDTH 320   // Landscape (niet gebruikt)
#define SCREEN_HEIGHT 240

#define HEADER_HEIGHT 40
#define FOOTER_HEIGHT 50
#define GRID_COLS 2
#define GRID_ROWS 3

// ===========================================
// KLEUREN (RGB565)
// ===========================================
#define COLOR_BG        0x1082  // Donker grijs/blauw achtergrond
#define COLOR_BG_LIGHT  0x2104  // Iets lichter voor items
#define COLOR_TEXT      TFT_WHITE
#define COLOR_TEXT_DIM  0x8410  // Gedimde tekst
#define COLOR_HEADER    0x000A  // Donker blauw (header/footer)
#define COLOR_ACCENT    0x03FF  // Cyan accent
#define COLOR_PLASTIC   0xFC00  // Oranje
#define COLOR_PAPER     0x03BF  // Helder blauw
#define COLOR_GREEN     0x07E0  // Groen
#define COLOR_WASTE     0x6B4D  // Donker grijs
#define COLOR_SIDEBAR   0x0841  // Zeer donker voor zijbalk
#define COLOR_SELECTED  0x03FF  // Selectie kleur (cyan)

// ===========================================
// TOUCHSCREEN (XPT2046 op VSPI)
// ===========================================
#define XPT2046_CS   33
#define XPT2046_IRQ  34
#define XPT2046_MISO 35
#define XPT2046_MOSI 32
#define XPT2046_CLK  25

// ===========================================
// LED STRIPS (APA102 - 4 strips, elk 8 LEDs)
// ===========================================
#define NUM_LEDS_PER_STRIP 8

// Strip 1: Plastic (Oranje)
#define LED1_DATA_PIN 23
#define LED1_CLK_PIN  18

// Strip 2: Papier (Blauw)
#define LED2_DATA_PIN 27
#define LED2_CLK_PIN  26

// Strip 3: GFT/Groen
#define LED3_DATA_PIN 19
#define LED3_CLK_PIN  5

// Strip 4: Restafval (Grijs)
#define LED4_DATA_PIN 17
#define LED4_CLK_PIN  16

// ===========================================
// TIMING
// ===========================================
#define SLEEP_TIMEOUT_MS 10000      // 10 sec naar sleep mode
#define ITEM_CHANGE_TIME 5000       // 5 sec per item in sleep
#define LED_BREATHING_SPEED 2000    // Adem cyclus in ms

#endif