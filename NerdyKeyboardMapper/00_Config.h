#pragma once
// Nerdy Keyboard Mapper - Config
// Firmware release candidate: 1.0.0 (EEPROM schema version remains independent).
#include <Arduino.h>
#include <EEPROM.h>
#include <Keyboard.h>
#include <cstring>

// Fallback fuer Keyboard-Versionen ohne erweiterte Symbolnamen.
#ifndef KEY_MENU
#define KEY_MENU 0xED
#endif
#ifndef KEY_PRINT_SCREEN
#define KEY_PRINT_SCREEN 0xCE
#endif
#ifndef KEY_PAUSE
#define KEY_PAUSE 0xD0
#endif
#ifndef KEY_NUM_LOCK
#define KEY_NUM_LOCK 0xDB
#endif
#ifndef KEY_KP_SLASH
#define KEY_KP_SLASH 0xDC
#define KEY_KP_ASTERISK 0xDD
#define KEY_KP_MINUS 0xDE
#define KEY_KP_PLUS 0xDF
#define KEY_KP_ENTER 0xE0
#define KEY_KP_1 0xE1
#define KEY_KP_2 0xE2
#define KEY_KP_3 0xE3
#define KEY_KP_4 0xE4
#define KEY_KP_5 0xE5
#define KEY_KP_6 0xE6
#define KEY_KP_7 0xE7
#define KEY_KP_8 0xE8
#define KEY_KP_9 0xE9
#define KEY_KP_0 0xEA
#define KEY_KP_DOT 0xEB
#endif


// Nerdy Keyboard Mapper - USB HID + PS/2 - Raspberry Pi Pico / Arduino-Pico
// Reuse passive laptop keyboard matrices with USB HID and PS/2.
// Keine vorinstallierten Tastenzuordnungen.
// Bestehende v1/v2-Datenbank wird unveraendert eingelesen.
// GPIO-Reihenfolge immer auf die eigene Verdrahtung abstimmen.
constexpr uint8_t PIN_COUNT = 24;
const uint8_t gpio[PIN_COUNT] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11,
  12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 26
};

constexpr char FIRMWARE_VERSION[] = "1.0.0";

constexpr uint32_t MAGIC = 0x4B4D4150;
// Persisted EEPROM schema version; do not change when releasing firmware.
constexpr uint16_t VERSION = 1;
constexpr uint16_t MAX_KEYS = 160;
constexpr size_t LABEL_SIZE = 20;

// EEPROM-Layout absichtlich identisch zu Mapper v1.0:
// FPC-Paare INTERN nullbasiert, CSV einbasiert.
struct KeyEntry { uint8_t a, b; char label[LABEL_SIZE]; };
struct Database {
  uint32_t magic;
  uint16_t version;
  uint16_t count;
  KeyEntry keys[MAX_KEYS];
};
static_assert(sizeof(Database) <= 4096, "EEPROM zu klein");
Database db;
bool databaseValid = false;

// Forward-Deklarationen vor der Arduino-Prototypgenerierung
struct Ps2KeyCode { uint8_t code; bool extended; bool specialPrint; bool specialPause; };
Ps2KeyCode ps2Lookup(const char *name);
void ps2HandleEvent(uint16_t i, bool down);
void usbHandleEvent(uint16_t i, bool down);
bool ps2Mode = false;

// Mapping: einzelne Taste. Testmodus: Mehrfachtasten und Ghosting-Diagnose.
enum Mode { NORMAL, NAME_NEW, PICK_EDIT, NAME_EDIT, PICK_REMAP };
Mode mode = NORMAL;
int selected = -1;
uint8_t pendingA = 0, pendingB = 0;
String inputLine;

struct Scan { uint16_t count; uint8_t a, b; };
Scan raw = {0, 0, 0};
unsigned long rawAt = 0;
bool readyForPress = true;
unsigned long lastPoll = 0;
constexpr unsigned long DEBOUNCE = 40;
constexpr unsigned long POLL_INTERVAL = 12;

