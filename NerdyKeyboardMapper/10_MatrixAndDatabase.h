#pragma once
// Nerdy Keyboard Mapper - MatrixAndDatabase
void releaseGPIOs() {
  for (uint8_t i = 0; i < PIN_COUNT; ++i) pinMode(gpio[i], INPUT_PULLUP);
}

Scan scanMatrix() {
  Scan ret = {0, 0, 0};
  bool seen[PIN_COUNT][PIN_COUNT] = {};
  releaseGPIOs();
  for (uint8_t i = 0; i < PIN_COUNT; ++i) {
    digitalWrite(gpio[i], LOW);
    pinMode(gpio[i], OUTPUT);
    delayMicroseconds(25);
    for (uint8_t j = 0; j < PIN_COUNT; ++j) {
      if (i == j || digitalRead(gpio[j]) != LOW) continue;
      uint8_t a = min(i, j), b = max(i, j);
      if (!seen[a][b]) {
        seen[a][b] = true;
        ++ret.count;
        ret.a = a;
        ret.b = b;
      }
    }
    pinMode(gpio[i], INPUT_PULLUP);
  }
  return ret;
}

bool sameScan(const Scan &a, const Scan &b) {
  return a.count == b.count && (a.count != 1 || (a.a == b.a && a.b == b.b));
}

void sortPair(uint8_t &a, uint8_t &b) {
  if (a > b) { uint8_t t = a; a = b; b = t; }
}

int findKey(uint8_t a, uint8_t b) {
  sortPair(a, b);
  for (uint16_t i = 0; i < db.count; ++i)
    if (db.keys[i].a == a && db.keys[i].b == b) return i;
  return -1;
}

bool saveDB() {
  EEPROM.put(0, db);
  if (!EEPROM.commit()) {
    Serial.println("FEHLER: Flash-Schreibvorgang fehlgeschlagen");
    return false;
  }
  return true;
}

bool validateDB() {
  if (db.magic != MAGIC || db.version != VERSION || db.count > MAX_KEYS) return false;
  for (uint16_t i = 0; i < db.count; ++i) {
    const KeyEntry &k = db.keys[i];
    if (k.a >= PIN_COUNT || k.b >= PIN_COUNT || k.a == k.b ||
        !memchr(k.label, '\0', LABEL_SIZE)) return false;
  }
  return true;
}

void initDB() {
  memset(&db, 0, sizeof(db));
  db.magic = MAGIC;
  db.version = VERSION;
  databaseValid = true;
}

void printKey(int n) {
  Serial.print(n + 1); Serial.print(": FPC ");
  Serial.print(db.keys[n].a + 1); Serial.print(" <-> ");
  Serial.print(db.keys[n].b + 1); Serial.print(" = ");
  Serial.println(db.keys[n].label);
}

void printList() {
  Serial.print("Eintraege: "); Serial.println(db.count);
  for (int i = 0; i < db.count; ++i) printKey(i);
}

void exportCSV() {
  Serial.println("FPC_A;FPC_B;NAME");
  for (int i = 0; i < db.count; ++i) {
    Serial.print(db.keys[i].a + 1); Serial.print(';');
    Serial.print(db.keys[i].b + 1); Serial.print(';');
    const char *name = db.keys[i].label;
    bool quote = strchr(name, ';') || strchr(name, '"');
    if (quote) Serial.print('"');
    for (const char *c = name; *c; ++c) {
      if (*c == '"') Serial.print('"');
      Serial.print(*c);
    }
    if (quote) Serial.print('"');
    Serial.println();
  }
  Serial.println("=== CSV ENDE ===");
}

bool validName(const String &s) {
  if (s.length() == 0 || s.length() >= LABEL_SIZE) {
    Serial.println("Name muss 1 bis 19 Bytes lang sein."); return false;
  }
  return true;
}
void setName(uint16_t idx, const String &name) {
  memset(db.keys[idx].label, 0, LABEL_SIZE);
  name.toCharArray(db.keys[idx].label, LABEL_SIZE);
}
int parseNumber(const String &v) {
  if (!v.length()) return -1;
  for (unsigned int i = 0; i < v.length(); ++i)
    if (v[i] < '0' || v[i] > '9') return -1;
  long n = v.toInt();
  return (n >= 1 && n <= db.count) ? int(n - 1) : -1;
}

// Neue/leere Datenbank wird NUR auf ausdruecklichen Befehl erstellt.
// Das verhindert einen versehentlichen Verlust bestehender Zuordnungen.
void initializeDatabase() {
  initDB();
  if (saveDB()) {
    Serial.println("Leere Datenbank initialisiert.");
    Serial.println("Jetzt jede Taste einzeln druecken und benennen.");
  }
  mode = NORMAL;
  selected = -1;
  readyForPress = false;
}

void onPress(uint8_t a, uint8_t b) {
  if (!databaseValid) return;
  int idx = findKey(a, b);
  if (mode == PICK_EDIT) {
    if (idx < 0) { Serial.println("Taste unbekannt. Andere waehlen."); return; }
    selected = idx; mode = NAME_EDIT;
    printKey(idx); Serial.println("Neuen Namen eingeben:"); Serial.print("> ");
    return;
  }
  if (mode == PICK_REMAP) {
    if (idx >= 0 && idx != selected) {
      Serial.println("Pinpaar bereits belegt:"); printKey(idx); return;
    }
    sortPair(a, b);
    db.keys[selected].a = a;
    db.keys[selected].b = b;
    saveDB(); printKey(selected);
    mode = NORMAL; selected = -1;
    return;
  }
  if (mode != NORMAL) return;
  if (idx >= 0) {
    Serial.print("Erkannt: "); Serial.println(db.keys[idx].label);
  } else {
    pendingA = a; pendingB = b;
    mode = NAME_NEW;
    Serial.print("Neue Taste FPC "); Serial.print(a + 1);
    Serial.print(" <-> "); Serial.println(b + 1);
    Serial.println("Name eingeben (:skip oder :cancel):"); Serial.print("> ");
  }
}

void updateScanner() {
  if (millis() - lastPoll < POLL_INTERVAL) return;
  lastPoll = millis();
  Scan cur = scanMatrix();
  if (!sameScan(cur, raw)) { raw = cur; rawAt = millis(); return; }
  if (millis() - rawAt < DEBOUNCE) return;
  if (cur.count == 0) { readyForPress = true; return; }
  if (cur.count != 1 || !readyForPress) return;
  readyForPress = false;
  onPress(cur.a, cur.b);
}


