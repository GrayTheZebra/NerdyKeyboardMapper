#pragma once
// Nerdy Keyboard Mapper - SerialConsole
void help() {
  Serial.println("Befehle:");
  Serial.println(":list           Auflisten mit Nummern");
  Serial.println(":test           Mehrtasten-Test EIN/AUS (nur bekannte Tasten)");
  Serial.println(":usb on         USB-HID-Tasten senden");
  Serial.println(":usb off        USB-HID-Tasten stoppen");
  Serial.println(":usb status     USB-HID-Zaehler anzeigen");
  Serial.println(":ps2 on         PS/2-Ausgabe einschalten");
  Serial.println(":ps2 off        PS/2-Ausgabe ausschalten");
  Serial.println(":ps2 status     PS/2-Status/Fehlerzaehler");
  Serial.println(":test off       Mehrtasten-Test ausschalten");
  Serial.println(":export         CSV ausgeben");
  Serial.println(":rename N NAME  Eintrag N umbenennen");
  Serial.println(":delete N       Eintrag N loeschen");
  Serial.println(":remap N        Eintrag N neu anlernen");
  Serial.println(":edit           Taste druecken und Namen aendern");
  Serial.println(":skip           Unbekannte Taste ueberspringen");
  Serial.println(":init CONFIRM     Neue leere Datenbank anlegen (ACHTUNG: ueberschreibt!)");
  Serial.println(":erase CONFIRM   Alle Zuordnungen loeschen");
  Serial.println(":cancel         Eingabe abbrechen");
}

void command(String s) {
  s.trim();
  if (!s.length()) return;
  if (s == ":cancel") { mode = NORMAL; selected = -1; Serial.println("Abgebrochen."); return; }
  if (s == ":init CONFIRM") { initializeDatabase(); return; }
  if (s == ":help") { help(); return; }
  if (s == ":usb on") {usbReleaseAll();usbEnabled=true;testMode=true;resetTestState();Serial.println("USB-HID aktiv.");return;}
  if (s == ":usb off") {usbEnabled=false;usbReleaseAll();if(!ps2Mode)testMode=false;Serial.println("USB-HID-Ausgabe deaktiviert.");return;}
  if (s == ":usb status") {
    Serial.print("USB-HID aktiv: ");Serial.println(usbEnabled?"ja":"nein");
    Serial.print("DOWN: ");Serial.print(usbDownCount);
    Serial.print(" UP: ");Serial.print(usbUpCount);
    Serial.print(" Unbekannte: ");Serial.println(usbUnknownCount);
    return;
  }
  if (s == ":ps2 on") { if(databaseValid)ps2Start();else Serial.println("Keine gueltige Datenbank.");return; }
  if (s == ":ps2 off") { ps2Stop();return; }
  if (s == ":ps2 status") {
    Serial.print("PS/2 aktiv: ");Serial.println(ps2Mode?"ja":"nein");
    Serial.print("Scanning: ");Serial.println(ps2Scanning?"ja":"nein");
    Serial.print("RX: ");Serial.print(ps2RxCount);
    Serial.print(" TX: ");Serial.print(ps2TxCount);
    Serial.print(" Fehler: ");Serial.println(ps2ErrCount);
    Serial.print("LED Bits: ");Serial.println(ps2Leds,BIN);
    if(ps2QueueOverflow)Serial.println("WARNUNG: PS/2-Warteschlange uebergelaufen!");
    return;
  }
  if (!databaseValid) {
    Serial.println("Datenbank ungueltig: Keine automatische Loeschung!");
    Serial.println("Bestehende Daten werden nicht automatisch ueberschrieben.");
    Serial.println("Nur :init CONFIRM erstellt bewusst eine leere Datenbank.");
    return;
  }
  if(ps2Mode){
    if(s==":list"){printList();return;}
    if(s==":export"){exportCSV();return;}
    Serial.println("Im PS/2-Modus: :ps2 off, :ps2 status, :help, :list, :export.");
    return;
  }
  if (s == ":test" || s == ":test on" || s == ":test off") {
    testMode = (s == ":test off") ? false : ((s == ":test") ? !testMode : true);
    mode = NORMAL; selected = -1; readyForPress = false;
    if (testMode) {
      resetTestState();
      Serial.println("MEHRTASTEN-TEST AKTIV: DOWN/UP + Ghosting-Warnung.");
      Serial.println("Nur bekannte Tasten; keine Speicherung.");
    } else {
      Serial.println("MEHRTASTEN-TEST AUS. Alle Tasten loslassen.");
      raw = {0, 0, 0}; rawAt = millis();
    }
    return;
  }
  if (testMode) {
    Serial.println("Im Testmodus nur :test, :test off, :help, :list, :export und :cancel.");
    if (s == ":list") printList();
    if (s == ":export") exportCSV();
    return;
  }
  if (mode == NAME_NEW) {
    if (s == ":skip") { mode = NORMAL; Serial.println("Uebersprungen."); return; }
    if (!validName(s)) return;
    if (db.count >= MAX_KEYS) { Serial.println("Datenbank voll"); mode = NORMAL; return; }
    KeyEntry &k = db.keys[db.count];
    k.a = pendingA; k.b = pendingB; setName(db.count, s);
    ++db.count; saveDB(); printKey(db.count - 1);
    mode = NORMAL; return;
  }
  if (mode == NAME_EDIT) {
    if (!validName(s)) return;
    setName(selected, s); saveDB(); printKey(selected);
    mode = NORMAL; selected = -1; return;
  }
  if (mode == PICK_EDIT || mode == PICK_REMAP) {
    Serial.println("Bitte Tastaturtaste druecken oder :cancel."); return;
  }
  if (s == ":list") { printList(); return; }
  if (s == ":export") { exportCSV(); return; }
  if (s == ":edit") {
    mode = PICK_EDIT; Serial.println("Bekannte Taste druecken:"); return;
  }
  if (s == ":erase CONFIRM") {
    initDB(); saveDB(); Serial.println("Datenbank geloescht."); return;
  }
  if (s.startsWith(":rename ")) {
    String args = s.substring(8); args.trim();
    int space = args.indexOf(' ');
    if (space < 0) { Serial.println("Syntax: :rename N NAME"); return; }
    int idx = parseNumber(args.substring(0, space));
    String name = args.substring(space + 1); name.trim();
    if (idx < 0 || !validName(name)) { Serial.println("Ungueltige Angabe."); return; }
    setName(idx, name); saveDB(); printKey(idx); return;
  }
  if (s.startsWith(":delete ")) {
    int idx = parseNumber(s.substring(8));
    if (idx < 0) { Serial.println("Ungueltige Nummer."); return; }
    Serial.print("Geloescht: "); printKey(idx);
    for (int i = idx; i + 1 < db.count; ++i) db.keys[i] = db.keys[i + 1];
    --db.count; memset(&db.keys[db.count], 0, sizeof(KeyEntry));
    saveDB(); return;
  }
  if (s.startsWith(":remap ")) {
    int idx = parseNumber(s.substring(7));
    if (idx < 0) { Serial.println("Ungueltige Nummer."); return; }
    selected = idx; mode = PICK_REMAP;
    Serial.println("Neue Taste fuer folgenden Eintrag druecken:");
    printKey(idx); return;
  }
  Serial.println("Unbekannter Befehl (:help)");
}

void serialInput() {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r') {
      if (inputLine.length()) { command(inputLine); inputLine = ""; }
    } else if ((uint8_t)c >= 32 && inputLine.length() < 120) {
      inputLine += c;
    }
  }
}

