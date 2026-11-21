#pragma once
// Nerdy Keyboard Mapper - Ps2Device
// The Pico needs its own 5-V power (USB or properly isolated 5 V on VSYS).
constexpr bool PS2_AUTO_START = true;
// For keyboards with a German QWERTZ physical layout.
constexpr bool PS2_GERMAN_QWERTZ = true;
constexpr uint8_t PS2_CLOCK = 27;
constexpr uint8_t PS2_DATA = 28;
constexpr uint16_t PS2_HALF_LOW_US = 38;
constexpr uint16_t PS2_HALF_HIGH_US = 38;
constexpr uint16_t PS2_WAIT_US = 200;

bool ps2Scanning = false;
bool ps2BatPending = false;
uint32_t ps2BatAt = 0;
uint8_t ps2Param = 0; // 0 none; 1 LEDs, 2 typematic, 3 scan set
uint8_t ps2Leds = 0;
uint8_t ps2Typematic = 0x2B;
uint8_t ps2LastByte = 0xAA;
uint32_t ps2RxCount = 0, ps2TxCount = 0, ps2ErrCount = 0;

// Keyboard data and replies kept separate; host replies take priority.
uint8_t ps2Q[192], ps2ReplyQ[24];
uint16_t ps2Head=0,ps2Tail=0;
uint8_t ps2ReplyHead=0, ps2ReplyTail=0;
bool ps2QueueOverflow = false;
uint8_t pressCode[MAX_KEYS] = {};
bool pressExtended[MAX_KEYS] = {};

inline void ps2PullLow(uint8_t pin) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
}
// Pull-up on the Pico side prevents floating inputs during USB-only diagnosis.
// When connected to the MOSFET level shifter this remains open-drain:
// the MCU never actively drives HIGH.
inline void ps2Release(uint8_t pin) { pinMode(pin, INPUT_PULLUP); }
inline bool ps2IsHigh(uint8_t pin) { return digitalRead(pin) != LOW; }
void ps2BusRelease() { ps2Release(PS2_DATA); ps2Release(PS2_CLOCK); }

void ps2QueueClear() {
  ps2Head=ps2Tail=0;
  ps2ReplyHead=ps2ReplyTail=0;
  ps2QueueOverflow=false;
}
void ps2Queue(uint8_t b) {
  uint16_t next=(ps2Head+1)%sizeof(ps2Q);
  if(next==ps2Tail) {ps2QueueOverflow=true;return;}
  ps2Q[ps2Head]=b; ps2Head=next;
}
void ps2Reply(uint8_t b) {
  uint8_t next=(ps2ReplyHead+1)%sizeof(ps2ReplyQ);
  if(next==ps2ReplyTail) {ps2QueueOverflow=true;return;}
  ps2ReplyQ[ps2ReplyHead]=b; ps2ReplyHead=next;
}
void ps2ResetKeys() {
  memset(pressCode, 0, sizeof(pressCode));
  memset(pressExtended, 0, sizeof(pressExtended));
}
void ps2SetDefault() {
  ps2Param=0; ps2Leds=0; ps2Typematic=0x2B;
  ps2ResetKeys();
  resetTestState();
}

// Normalizes names from user's mapping; retains labels in EEPROM.
// Physical key positions for German (ISO) layout: codes do not encode glyphs.
String ps2Normalized(const char *label) {
  String n(label); n.toLowerCase(); n.trim();
  while(n.startsWith("#")) n.remove(0,1);
  n.replace("_","-"); n.replace(" ","-");
  if(n=="schift-links")n="shift-links";
  if(n=="tap")n="tab";
  if(n=="entgernen")n="entfernen";
  return n;
}
Ps2KeyCode ps2Lookup(const char *name) {
  String s=ps2Normalized(name);
  if(s.length()==1) {
    const char *alphabet="abcdefghijklmnopqrstuvwxyz";
    const uint8_t codes[]={0x1C,0x32,0x21,0x23,0x24,0x2B,0x34,0x33,0x43,
      0x3B,0x42,0x4B,0x3A,0x31,0x44,0x4D,0x15,0x2D,0x1B,
      0x2C,0x3C,0x2A,0x1D,0x22,0x35,0x1A};
    const char *p=strchr(alphabet,s[0]);
    if(p) {
      uint8_t c=codes[p-alphabet];
      if(PS2_GERMAN_QWERTZ) {
        if(s[0]=='y')c=0x1A; // ISO DE position of Y = US Z
        if(s[0]=='z')c=0x35; // ISO DE position of Z = US Y
      }
      return {c,false,false,false};
    }
    const uint8_t digits[]={0x45,0x16,0x1E,0x26,0x25,0x2E,0x36,0x3D,0x3E,0x46};
    if(s[0]>='0'&&s[0]<='9')return {digits[s[0]-'0'],false,false,false};
  }
  const char *fkeys[]={"f1","f2","f3","f4","f5","f6","f7","f8","f9","f10","f11","f12"};
  const uint8_t fcodes[]={0x05,0x06,0x04,0x0C,0x03,0x0B,0x83,0x0A,0x01,0x09,0x78,0x07};
  for(int i=0;i<12;i++)if(s==fkeys[i])return {fcodes[i],false,false,false};
  const char *np[]={"numpad0","numpad1","numpad2","numpad3","numpad4","numpad5", "numpad6","numpad7","numpad8","numpad9"};
  const uint8_t nc[]={0x70,0x69,0x72,0x7A,0x6B,0x73,0x74,0x6C,0x75,0x7D};
  for(int i=0;i<10;i++)if(s==np[i])return {nc[i],false,false,false};
  struct NamedCode {const char *n; uint8_t c; bool e;};
  static const NamedCode names[]={
    {"esc",0x76,0},{"enter",0x5A,0},{"space",0x29,0},{"tab",0x0D,0},
    {"backspace",0x66,0},{"caps-lock",0x58,0},{"capslock",0x58,0},
    {"shift-links",0x12,0},{"shift-rechts",0x59,0},
    {"strg-links",0x14,0},{"strg-rechts",0x14,1},
    {"alt",0x11,0},{"alt-links",0x11,0},{"alt-gr",0x11,1},
    {"win-links",0x1F,1},{"win-rechts",0x27,1}, {"kontextmenue",0x2F,1},
    {"pfeil-links",0x6B,1},{"pfeil-rechts",0x74,1},
    {"pfeil-hoch",0x75,1},{"pfeil-oben",0x75,1}, {"pfeil-unten",0x72,1},
    {"pos1",0x6C,1},{"home",0x6C,1},{"ende",0x69,1},
    {"bild-hoch",0x7D,1},{"page-up",0x7D,1},
    {"bild-runter",0x7A,1},{"page-down",0x7A,1},
    {"einfuegen",0x70,1},{"entfernen",0x71,1},
    {"num-lock",0x77,0},{"numlock",0x77,0},
    {"division",0x4A,1},{"numpad-division",0x4A,1},
    {"numpad-stern",0x7C,0},{"numpad-minus",0x7B,0},
    {"numpad-plus",0x79,0},{"numpad-enter",0x5A,1},
    {"numpad-komma",0x71,0},{"numpad-punkt",0x71,0},
    // German ISO physical positions
    {"ue",0x54,0},{"plus",0x5B,0},{"oe",0x4C,0},
    {"ae",0x52,0},{"raute",0x5D,0},
    {"fragezeichen",0x4E,0},{"apostroph",0x55,0},
    {"zirkumflex",0x0E,0},{"spitze-klammer",0x61,0},
    {"komma",0x41,0},{"punkt",0x49,0},{"bindestrich",0x4A,0},
    {"stern",0x5D,0}
  };
  for(const auto &c:names) if(s==c.n)return {c.c,c.e,false,false};
  if(s=="print" || s=="druck")return {0,false,true,false};
  if(s=="break" || s=="pause")return {0,false,false,true};
  return {0,false,false,false}; // fn and unknown labels never transmitted
}

// Frame output: 11 falling edges. PS/2 device controls Clock.
// NOT connected to host until voltage levels checked!
bool ps2SendByte(uint8_t b) {
  if(!ps2IsHigh(PS2_CLOCK) || !ps2IsHigh(PS2_DATA))return false;
  delayMicroseconds(60);
  if(!ps2IsHigh(PS2_CLOCK) || !ps2IsHigh(PS2_DATA))return false;
  bool parity=true;
  for(int i=0;i<8;i++)parity ^= ((b>>i)&1)!=0;
  // Keep USB CDC responsive even if the host inhibits PS/2 traffic.
  bool ok=true;
  for(int bit=0;bit<11;bit++){
    bool value=(bit==0)?false:(bit==9)?parity:(bit==10)?true:(((b>>(bit-1))&1)!=0);
    if(value) ps2Release(PS2_DATA); else ps2PullLow(PS2_DATA);
    delayMicroseconds(PS2_HALF_HIGH_US/2);
    // If host pulled clock low, it inhibited communication.
    if(!ps2IsHigh(PS2_CLOCK)){ok=false;break;}
    ps2PullLow(PS2_CLOCK);
    delayMicroseconds(PS2_HALF_LOW_US);
    ps2Release(PS2_CLOCK);
    uint32_t start=micros();
    while(!ps2IsHigh(PS2_CLOCK) && (uint32_t)(micros()-start)<PS2_WAIT_US){}
    if(!ps2IsHigh(PS2_CLOCK)){ok=false;break;}
    delayMicroseconds(PS2_HALF_HIGH_US/2);
  }
  ps2BusRelease();
  if(ok){ps2LastByte=b;ps2TxCount++;}
  else ps2ErrCount++;
  return ok;
}

// Device samples host data on rising Clock edge, ACK at end.
// Enter only when CLK=HIGH and DATA=LOW (host request-to-send).
int ps2ReceiveByte() {
  if(!ps2IsHigh(PS2_CLOCK) || ps2IsHigh(PS2_DATA))return -1;
  delayMicroseconds(15);
  if(!ps2IsHigh(PS2_CLOCK) || ps2IsHigh(PS2_DATA))return -1;
  uint16_t frame=0;
  // Keep USB CDC responsive even if the host inhibits PS/2 traffic.
  bool ok=true;
  for(int bit=0;bit<11;bit++) {
    // Host changes DATA only while CLOCK is low.
    ps2PullLow(PS2_CLOCK);
    delayMicroseconds(PS2_HALF_LOW_US);
    ps2Release(PS2_CLOCK);
    uint32_t start=micros();
    while(!ps2IsHigh(PS2_CLOCK) && (uint32_t)(micros()-start)<PS2_WAIT_US){}
    if(!ps2IsHigh(PS2_CLOCK)){ok=false;break;}
    delayMicroseconds(20);
    if(!ps2IsHigh(PS2_DATA))frame|=(1u<<bit);
    delayMicroseconds(PS2_HALF_HIGH_US-20);
  }
  if(ok){
    // Host has released data after STOP; keyboard asserts ACK.
    ps2PullLow(PS2_DATA);
    delayMicroseconds(18);
    ps2PullLow(PS2_CLOCK);
    delayMicroseconds(PS2_HALF_LOW_US);
    ps2Release(PS2_CLOCK);
    delayMicroseconds(PS2_HALF_HIGH_US);
    ps2Release(PS2_DATA);
  }
  ps2BusRelease();
  if(!ok){ps2ErrCount++;return -1;}
  // START 0, STOP 1; odd parity across data+parity.
  if((frame&1)!=0 || ((frame>>10)&1)==0){ps2ErrCount++;return -1;}
  uint8_t b=(frame>>1)&0xFF;
  uint8_t parity=(frame>>9)&1;
  uint8_t ones=parity;
  for(int i=0;i<8;i++)ones+=(b>>i)&1;
  if((ones&1)!=1){ps2ErrCount++;return -1;}
  ps2RxCount++;
  return b;
}

void ps2RespondToHost(uint8_t b){
  ps2BatPending=false; // host is already communicating; avoid a later duplicate BAT
  if(ps2Param!=0) {
    uint8_t target=ps2Param;ps2Param=0;
    if(target==1){ps2Leds=b;ps2Reply(0xFA);return;}
    if(target==2){ps2Typematic=b;ps2Reply(0xFA);return;}
    if(target==3){
      if(b==0x00){ps2Reply(0xFA);ps2Reply(0x02);return;}
      if(b==0x02){ps2Reply(0xFA);return;}
      ps2Reply(0xFE); // Only Scan Code Set 2 implemented.
      return;
    }
  }
  switch(b){
    case 0xFF: // reset
      ps2QueueClear(); ps2SetDefault(); ps2Scanning=false;
      ps2Reply(0xFA);ps2Reply(0xAA);ps2Scanning=true;
      break;
    case 0xF2: ps2Reply(0xFA);ps2Reply(0xAB);ps2Reply(0x83);break;
    case 0xF0: ps2Reply(0xFA);ps2Param=3;break;
    case 0xED: ps2Reply(0xFA);ps2Param=1;break;
    case 0xF3: ps2Reply(0xFA);ps2Param=2;break;
    case 0xF4: ps2Reply(0xFA);ps2Scanning=true;break;
    case 0xF5: ps2Reply(0xFA);ps2Scanning=false;ps2SetDefault();ps2Head=ps2Tail=0;break;
    case 0xF6: ps2Reply(0xFA);ps2SetDefault();break;
    case 0xEE: ps2Reply(0xEE);break;
    case 0xFE: ps2Reply(ps2LastByte);break;
    default: ps2Reply(0xFE);break;
  }
}

void ps2SendSequence(uint8_t code,bool ext,bool down){
  if(!ps2Scanning || code==0)return;
  if(ext)ps2Queue(0xE0);
  if(!down)ps2Queue(0xF0);
  ps2Queue(code);
}
void ps2HandleEvent(uint16_t i,bool down){
  String label=ps2Normalized(db.keys[i].label);
  if(label=="fn")return;
  Ps2KeyCode key=ps2Lookup(db.keys[i].label);
  bool isFn=false;
  for(uint16_t j=0;j<db.count;j++)
    if(testStates[j].stableDown && ps2Normalized(db.keys[j].label)=="fn") {isFn=true;break;}
  if(down && isFn) {
    if(label=="pfeil-links")key={0x6C,true,false,false};  // Home
    if(label=="pfeil-rechts")key={0x69,true,false,false}; // End
    if(label=="pfeil-hoch" || label=="pfeil-oben")key={0x7D,true,false,false}; // PgUp
    if(label=="pfeil-unten")key={0x7A,true,false,false}; // PgDn
  }
  if(key.specialPrint){
    // Print Screen set2 make and break.
    if(down){ps2Queue(0xE0);ps2Queue(0x12);ps2Queue(0xE0);ps2Queue(0x7C);}
    else {ps2Queue(0xE0);ps2Queue(0xF0);ps2Queue(0x7C);ps2Queue(0xE0);ps2Queue(0xF0);ps2Queue(0x12);}
    return;
  }
  if(key.specialPause) {
    if(down){const uint8_t seq[]={0xE1,0x14,0x77,0xE1,0xF0,0x14,0xF0,0x77};for(uint8_t b:seq)ps2Queue(b);}
    return;
  }
  if(down){
    pressCode[i]=key.code;pressExtended[i]=key.extended;
  } else {
    key.code=pressCode[i];key.extended=pressExtended[i];
    pressCode[i]=0;
  }
  ps2SendSequence(key.code,key.extended,down);
}
uint32_t ps2NextTxAttempt = 0;
void ps2Start() {
  ps2Mode=true;
  ps2QueueClear();
  ps2SetDefault();
  ps2Scanning=true;
  ps2BusRelease();
  ps2BatPending=true;
  ps2BatAt=millis()+800;
  ps2NextTxAttempt=0;
  testMode=true;
}
void ps2Stop(){
  ps2Mode=false;ps2Scanning=false;ps2BatPending=false;
  ps2QueueClear();ps2BusRelease();ps2ResetKeys();
  testMode=usbEnabled;resetTestState();
  Serial.println("PS/2 deaktiviert; USB-HID bleibt aktiv, wenn eingeschaltet.");
}

uint32_t ps2RxRetryAt = 0;
void ps2Service(){
  if(!ps2Mode)return;
  // Prevent a noisy/floating DATA line from monopolising the main loop.
  if ((int32_t)(millis() - ps2RxRetryAt) < 0) return;
  // Only touch bus with voltage supply present on host side!
  if(ps2IsHigh(PS2_CLOCK) && !ps2IsHigh(PS2_DATA)){
    int rx=ps2ReceiveByte();
    if(rx>=0) {
      ps2RespondToHost((uint8_t)rx);
    } else {
      // Back off after invalid frames, but never block Serial commands.
      ps2RxRetryAt=millis()+20;
    }
    return;
  }
  if(ps2BatPending && (int32_t)(millis()-ps2BatAt)>=0){
    ps2Reply(0xAA);ps2BatPending=false;
  }
  if(!ps2IsHigh(PS2_CLOCK) || !ps2IsHigh(PS2_DATA))return;
  if ((int32_t)(millis()-ps2NextTxAttempt)<0) return;
  bool attempted=false, sent=false;
  if(ps2ReplyTail != ps2ReplyHead){
    uint8_t b=ps2ReplyQ[ps2ReplyTail];
    attempted=true;
    sent=ps2SendByte(b);
    if(sent)ps2ReplyTail=(ps2ReplyTail+1)%sizeof(ps2ReplyQ);
  } else if(ps2Scanning && ps2Tail!=ps2Head){
    uint8_t b=ps2Q[ps2Tail];
    attempted=true;
    sent=ps2SendByte(b);
    if(sent)ps2Tail=(ps2Tail+1)%sizeof(ps2Q);
  }
  if(attempted) ps2NextTxAttempt=millis()+(sent?2:50);
}

