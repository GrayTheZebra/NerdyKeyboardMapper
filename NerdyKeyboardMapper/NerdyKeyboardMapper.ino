// Nerdy Keyboard Mapper - Raspberry Pi Pico (Arduino-Pico / Pico SDK USB stack)
// Do not change EEPROM database structs without a migration.
#include "00_Config.h"
#include "10_MatrixAndDatabase.h"
#include "20_KeyScanner.h"
#include "30_UsbHid.h"
#include "40_Ps2Device.h"
#include "50_SerialConsole.h"

void setup() {
  Keyboard.begin(); // USB-HID + USB-Serial, Pico SDK USB Stack
  Serial.begin(115200);
  ps2BusRelease();
  releaseGPIOs();
  EEPROM.begin(sizeof(Database));
  EEPROM.get(0, db);
  databaseValid = validateDB();
  Serial.print("Nerdy Keyboard Mapper v");
  Serial.print(FIRMWARE_VERSION);
  Serial.println(" - USB-HID / PS/2 (Release Candidate)");
  if (databaseValid) {
    Serial.print("Gespeicherte Eintraege: "); Serial.println(db.count);
  } else {
    Serial.println("Datenbank unbekannt/ungueltig. Nicht ueberschrieben.");
  }
  if (!databaseValid) Serial.println("Zum Starten: :init CONFIRM (ACHTUNG: ueberschreibt ggf. alte Daten)");
  if (Serial) help();
  if(databaseValid && usbEnabled) {testMode=true;resetTestState();}
  Serial.println("USB-HID Autostart: aktiv (DE physische Tastenpositionen).");
  if(databaseValid && PS2_AUTO_START) {
    ps2Start();
    Serial.println("PS/2 Autostart: aktiv.");
  }
  if(databaseValid) {
    if(!PS2_AUTO_START)
      Serial.println("PS/2 standardmaessig AUS (Ersttest). :ps2 on aktiviert.");
    else
      Serial.println("PS/2 Autostart EIN; :ps2 off deaktiviert.");
  }
}
void loop() {
  ps2Service();
  serialInput();
  if(databaseValid){
    if(testMode)updateTest();
    else updateScanner();
  }
  ps2Service();
}
