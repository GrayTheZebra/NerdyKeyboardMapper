#pragma once
// Nerdy Keyboard Mapper - KeyScanner
bool testMode = false;
struct TestState {
  bool rawDown = false;
  bool stableDown = false;
  uint32_t changedAt = 0;
};
TestState testStates[MAX_KEYS];
int8_t side[PIN_COUNT];
uint8_t adjacency[PIN_COUNT][PIN_COUNT];
bool ghostReported = false;
constexpr unsigned long TEST_DEBOUNCE = 35;

void inferMatrixSides() {
  for (uint8_t i = 0; i < PIN_COUNT; ++i) side[i] = -1;
  // BFS pro Zusammenhangskomponente der gespeicherten Matrix
  uint8_t queue[PIN_COUNT];
  for (uint8_t root = 0; root < PIN_COUNT; ++root) {
    if (side[root] >= 0) continue;
    side[root] = 0;
    uint8_t head = 0, tail = 0;
    queue[tail++] = root;
    while (head < tail) {
      uint8_t from = queue[head++];
      for (uint16_t k = 0; k < db.count; ++k) {
        const KeyEntry &key = db.keys[k];
        uint8_t other;
        if (key.a == from) other = key.b;
        else if (key.b == from) other = key.a;
        else continue;
        if (side[other] == -1) {
          side[other] = 1 - side[from];
          queue[tail++] = other;
        } else if (side[other] == side[from]) {
          // Non-bipartite mapping: scanning still rejects ambiguous components.
        }
      }
    }
  }
}

// Liest die leitenden Verbindungen. Der Scanner treibt jeweils
// nur einen GPIO auf LOW; alle anderen sind Eingaenge mit Pull-up.
void scanConnectivity() {
  memset(adjacency, 0, sizeof(adjacency));
  releaseGPIOs();
  for (uint8_t i = 0; i < PIN_COUNT; ++i) {
    digitalWrite(gpio[i], LOW);
    pinMode(gpio[i], OUTPUT);
    delayMicroseconds(25);
    for (uint8_t j = 0; j < PIN_COUNT; ++j) {
      if (i != j && digitalRead(gpio[j]) == LOW) {
        adjacency[i][j] = 1;
        adjacency[j][i] = 1;
      }
    }
    pinMode(gpio[i], INPUT_PULLUP);
  }
}

bool hasAmbiguousComponent() {
  bool visited[PIN_COUNT] = {};
  uint8_t queue[PIN_COUNT];
  for (uint8_t root = 0; root < PIN_COUNT; ++root) {
    if (visited[root]) continue;
    uint8_t head = 0, tail = 0;
    queue[tail++] = root;
    visited[root] = true;
    uint8_t first = 0, second = 0;
    while (head < tail) {
      uint8_t x = queue[head++];
      if (side[x] == 0) ++first;
      else ++second;
      for (uint8_t y = 0; y < PIN_COUNT; ++y) {
        if (adjacency[x][y] && !visited[y]) {
          visited[y] = true;
          queue[tail++] = y;
        }
      }
    }
    if (first >= 2 && second >= 2) return true;
  }
  return false;
}


void resetTestState() {
  memset(testStates, 0, sizeof(testStates));
  ghostReported = false;
  inferMatrixSides();
}

void updateTest() {
  if (millis() - lastPoll < POLL_INTERVAL) return;
  lastPoll = millis();
  scanConnectivity();

  if (hasAmbiguousComponent()) {
    if (!ghostReported) {
      ghostReported = true;
    }
    return;
  }
  if (ghostReported) {
    ghostReported = false;
  }

  uint32_t now = millis();
  for (uint16_t i = 0; i < db.count; ++i) {
    KeyEntry &k = db.keys[i];
    bool down = adjacency[k.a][k.b] != 0;
    TestState &st = testStates[i];
    if (down != st.rawDown) {
      st.rawDown = down;
      st.changedAt = now;
    }
    if (st.stableDown != st.rawDown && now - st.changedAt >= TEST_DEBOUNCE) {
      st.stableDown = st.rawDown;
      if(ps2Mode) ps2HandleEvent(i, st.stableDown);
      usbHandleEvent(i, st.stableDown);
    }
  }
}


