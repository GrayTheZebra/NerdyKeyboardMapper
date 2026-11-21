#pragma once
// Nerdy Keyboard Mapper - UsbHid
constexpr bool USB_HID_AUTO_START = true;
bool usbEnabled = USB_HID_AUTO_START;
uint32_t usbDownCount=0, usbUpCount=0, usbUnknownCount=0;
uint8_t usbPressed[MAX_KEYS] = {};

// Arduino Keyboard.h verwendet US-Zeichen als Beschreibung physischer Tasten.
// Beispielsweise DE-Ue = physische US-[ Taste, DE-Y = physische US-Z.
uint8_t usbLookup(const char *name) {
  String s(name); s.trim(); s.toLowerCase();
  while(s.startsWith("#")) s.remove(0,1);
  s.replace("_","-"); s.replace(" ","-");
  if(s=="schift-links")s="shift-links";
  if(s=="tap")s="tab";
  if(s=="entgernen")s="entfernen";
  if(s=="fn")return 0;
  if(s.length()==1) {
    char c=s[0];
    if(c>='a'&&c<='z')return c=='z'?'y':c=='y'?'z':c;
    if(c>='0'&&c<='9')return c;
  }
  if(s=="esc")return KEY_ESC;
  if(s=="enter")return KEY_RETURN;
  if(s=="space")return ' ';
  if(s=="tab")return KEY_TAB;
  if(s=="backspace")return KEY_BACKSPACE;
  if(s=="shift-links")return KEY_LEFT_SHIFT;
  if(s=="shift-rechts")return KEY_RIGHT_SHIFT;
  if(s=="strg-links")return KEY_LEFT_CTRL;
  if(s=="strg-rechts")return KEY_RIGHT_CTRL;
  if(s=="alt"||s=="alt-links")return KEY_LEFT_ALT;
  if(s=="alt-gr")return KEY_RIGHT_ALT;
  if(s=="win-links")return KEY_LEFT_GUI;
  if(s=="win-rechts")return KEY_RIGHT_GUI;
  if(s=="caps-lock"||s=="capslock")return KEY_CAPS_LOCK;
  if(s=="pfeil-links")return KEY_LEFT_ARROW;
  if(s=="pfeil-rechts")return KEY_RIGHT_ARROW;
  if(s=="pfeil-hoch"||s=="pfeil-oben")return KEY_UP_ARROW;
  if(s=="pfeil-unten")return KEY_DOWN_ARROW;
  if(s=="home"||s=="pos1")return KEY_HOME;
  if(s=="ende")return KEY_END;
  if(s=="bild-hoch"||s=="page-up")return KEY_PAGE_UP;
  if(s=="bild-runter"||s=="page-down")return KEY_PAGE_DOWN;
  if(s=="einfuegen")return KEY_INSERT;
  if(s=="entfernen")return KEY_DELETE;
  if(s=="print"||s=="druck")return KEY_PRINT_SCREEN;
  if(s=="break"||s=="pause")return KEY_PAUSE;
  if(s=="kontextmenue")return KEY_MENU;
  if(s=="num-lock"||s=="numlock")return KEY_NUM_LOCK;
  if(s=="division"||s=="numpad-division")return KEY_KP_SLASH;
  if(s=="numpad-stern")return KEY_KP_ASTERISK;
  if(s=="numpad-minus")return KEY_KP_MINUS;
  if(s=="numpad-plus")return KEY_KP_PLUS;
  if(s=="numpad-enter")return KEY_KP_ENTER;
  if(s=="numpad-komma"||s=="numpad-punkt")return KEY_KP_DOT;
  const uint8_t kp[]={KEY_KP_0,KEY_KP_1,KEY_KP_2,KEY_KP_3,KEY_KP_4,
                      KEY_KP_5,KEY_KP_6,KEY_KP_7,KEY_KP_8,KEY_KP_9};
  if(s.startsWith("numpad") && s.length()==7 && s[6]>='0' && s[6]<='9')
    return kp[s[6]-'0'];
  if(s.length()>=2 && s[0]=='f'){
    int n=s.substring(1).toInt();
    if(s==String("f")+n && n>=1 && n<=12)return KEY_F1+n-1;
  }
  // Deutsche ISO-Positionen, umgesetzt ueber US-Layout-Positionen:
  if(s=="ue")return '[';
  if(s=="plus")return ']';
  if(s=="oe")return ';';
  if(s=="ae")return '\'';
  if(s=="raute"||s=="stern")return '\\';
  if(s=="komma")return ',';
  if(s=="punkt")return '.';
  if(s=="fragezeichen"||s=="bindestrich")return '-';
  if(s=="apostroph")return '=';
  if(s=="zirkumflex")return '`';
  // ISO-102-Taste links von Y (<>) kann die Standard-Keyboard.h-API
  // nicht eindeutig ohne direkten HID-Usage-Code ausgeben.
  return 0;
}

void usbHandleEvent(uint16_t i,bool down){
  if(!usbEnabled || i>=MAX_KEYS)return;
  String label(db.keys[i].label); label.toLowerCase(); label.trim();
  if(label=="fn")return;
  uint8_t key=down ? usbLookup(db.keys[i].label) : usbPressed[i];
  if(down){
    // Fn + Pfeiltasten: Home, End, PageUp, PageDown
    bool fn=false;
    for(uint16_t j=0;j<db.count;j++){
      String n(db.keys[j].label);n.toLowerCase();n.trim();
      if(n=="fn" && testStates[j].stableDown){fn=true;break;}
    }
    if(fn){
      if(label=="pfeil-links")key=KEY_HOME;
      if(label=="pfeil-rechts")key=KEY_END;
      if(label=="pfeil-hoch"||label=="pfeil-oben")key=KEY_PAGE_UP;
      if(label=="pfeil-unten")key=KEY_PAGE_DOWN;
    }
    usbPressed[i]=key;
    if(key) {Keyboard.press(key);usbDownCount++;}
    else usbUnknownCount++;
  } else {
    usbPressed[i]=0;
    if(key) {Keyboard.release(key);usbUpCount++;}
  }
}

void usbReleaseAll(){
  Keyboard.releaseAll();
  memset(usbPressed,0,sizeof(usbPressed));
}

