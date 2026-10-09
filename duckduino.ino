/*
  duckduino - sd card payload runner for arduino pro micro (atmega32u4).
  reads script.txt from a microsd card and types it over usb hid.
*/

#include <SPI.h>
#include <SD.h>
#include <HID-Project.h>

#define SDCARD_CS 4
#define PAYLOAD_FILE "script.txt"
#define LED 30  // tx led

enum SymbolMode { SYM_WIN, SYM_LINUX, SYM_RAW };
SymbolMode symbolMode = SYM_WIN;

int defaultDelay = 0;
String previousLine = "";

// numpad digit keys 0-9, used by windows alt codes
const KeyboardKeycode KEYPAD[10] = {
  KEYPAD_0, KEYPAD_1, KEYPAD_2, KEYPAD_3, KEYPAD_4,
  KEYPAD_5, KEYPAD_6, KEYPAD_7, KEYPAD_8, KEYPAD_9
};

void setup() {
  pinMode(LED, OUTPUT);
  led(LED, false);

  BootKeyboard.begin();

  if (!SD.begin(SDCARD_CS)) {
    errorPulse();
  }

  File payload = SD.open(PAYLOAD_FILE);
  if (!payload) {
    errorPulse();
  }

  ensureNumLock();

  led(LED, true); // on = running
  while (payload.available()) {
    String line = readLine(payload);
    runLine(line);
  }
  payload.close();

  led(LED, false); // off = done
}

void loop() {}

// windows alt codes need numlock on
void ensureNumLock() {
  delay(500); // let the host send its first led report after enumeration
  if (!(BootKeyboard.getLeds() & LED_NUM_LOCK)) {
    BootKeyboard.write(KEY_NUM_LOCK);
    delay(50);
  }
}

String readLine(File &f) {
  String line = "";
  while (f.available()) {
    char c = f.read();
    if (c == '\n') break;
    if (c != '\r') line += c;
  }
  return line;
}

void runLine(String line) {
  if (line.length() == 0) return;
  if (line.startsWith("REM")) return;

  if (line.startsWith("STRING ")) {
    typeString(line.substring(7));
  } else if (line.startsWith("DELAY ")) {
    delay(line.substring(6).toInt());
  } else if (line.startsWith("DEFAULT_DELAY ")) {
    defaultDelay = line.substring(14).toInt();
  } else if (line.startsWith("DEFAULTDELAY ")) {
    defaultDelay = line.substring(13).toInt();
  } else if (line == "ALTCODES ON") {
    symbolMode = SYM_WIN;
  } else if (line == "ALTCODES OFF") {
    symbolMode = SYM_RAW;
  } else if (line == "ALTCODES LINUX") {
    symbolMode = SYM_LINUX;
  } else if (line.startsWith("REPEAT ")) {
    int times = line.substring(7).toInt();
    for (int i = 0; i < times; i++) {
      runLine(previousLine);
    }
    return;
  } else {
    pressCombo(line);
  }

  previousLine = line;
  if (defaultDelay) delay(defaultDelay);
}

void typeString(String s) {
  for (unsigned int i = 0; i < s.length(); i++) {
    char c = s[i];
    if (symbolMode == SYM_RAW || isBasic(c)) {
      BootKeyboard.write(c);
    } else if (symbolMode == SYM_LINUX) {
      unicodeLinux((uint8_t)c);
    } else {
      altCode((uint8_t)c);
    }
  }
}

// a-z, A-Z, 0-9 and space share positions across layouts
bool isBasic(char c) {
  return (c >= 'a' && c <= 'z') ||
         (c >= 'A' && c <= 'Z') ||
         (c >= '0' && c <= '9') ||
         c == ' ';
}

// windows alt+numpad code, eg 64 -> alt+0064 = @. leading 0 = windows-1252.
void altCode(uint8_t code) {
  BootKeyboard.press(KEY_LEFT_ALT);
  tapKeypad(0);
  char digits[4];
  itoa(code, digits, 10);
  for (byte i = 0; digits[i] != '\0'; i++) {
    tapKeypad(digits[i] - '0');
  }
  BootKeyboard.release(KEY_LEFT_ALT);
  delay(5);
}

void tapKeypad(uint8_t d) {
  BootKeyboard.press(KEYPAD[d]);
  delay(5);
  BootKeyboard.release(KEYPAD[d]);
  delay(5);
}

// linux ibus/gtk unicode entry: ctrl+shift+u, hex codepoint, enter. the byte
// value is the latin-1 codepoint, which matches unicode for 0x00-0xFF.
void unicodeLinux(uint8_t code) {
  BootKeyboard.press(KEY_LEFT_CTRL);
  BootKeyboard.press(KEY_LEFT_SHIFT);
  BootKeyboard.press('u');
  delay(5);
  BootKeyboard.releaseAll();
  char hex[3];
  itoa(code, hex, 16);
  for (byte i = 0; hex[i] != '\0'; i++) {
    BootKeyboard.write(hex[i]);
    delay(5);
  }
  BootKeyboard.write(KEY_ENTER);
  delay(5);
}

void pressCombo(String line) {
  int start = 0;
  while (start < (int)line.length()) {
    int space = line.indexOf(' ', start);
    if (space == -1) space = line.length();
    String token = line.substring(start, space);
    if (token.length() > 0) pressKey(token);
    start = space + 1;
  }
  BootKeyboard.releaseAll();
}

void pressKey(String key) {
  KeyboardKeycode code = keyCode(key);
  if (code != KEY_RESERVED) {
    BootKeyboard.press(code);
  } else if (key.length() == 1) {
    BootKeyboard.press((uint8_t)key[0]);
  }
}

KeyboardKeycode keyCode(String key) {
  if (key == "ENTER") return KEY_ENTER;
  if (key == "GUI" || key == "WINDOWS") return KEY_LEFT_GUI;
  if (key == "SHIFT") return KEY_LEFT_SHIFT;
  if (key == "ALT") return KEY_LEFT_ALT;
  if (key == "CTRL" || key == "CONTROL") return KEY_LEFT_CTRL;
  if (key == "TAB") return KEY_TAB;
  if (key == "ESC" || key == "ESCAPE") return KEY_ESC;
  if (key == "SPACE") return KEY_SPACE;
  if (key == "DELETE") return KEY_DELETE;
  if (key == "BACKSPACE") return KEY_BACKSPACE;
  if (key == "HOME") return KEY_HOME;
  if (key == "END") return KEY_END;
  if (key == "INSERT") return KEY_INSERT;
  if (key == "PAGEUP") return KEY_PAGE_UP;
  if (key == "PAGEDOWN") return KEY_PAGE_DOWN;
  if (key == "CAPSLOCK") return KEY_CAPS_LOCK;
  if (key == "UP" || key == "UPARROW") return KEY_UP_ARROW;
  if (key == "DOWN" || key == "DOWNARROW") return KEY_DOWN_ARROW;
  if (key == "LEFT" || key == "LEFTARROW") return KEY_LEFT_ARROW;
  if (key == "RIGHT" || key == "RIGHTARROW") return KEY_RIGHT_ARROW;
  if (key == "F1") return KEY_F1;
  if (key == "F2") return KEY_F2;
  if (key == "F3") return KEY_F3;
  if (key == "F4") return KEY_F4;
  if (key == "F5") return KEY_F5;
  if (key == "F6") return KEY_F6;
  if (key == "F7") return KEY_F7;
  if (key == "F8") return KEY_F8;
  if (key == "F9") return KEY_F9;
  if (key == "F10") return KEY_F10;
  if (key == "F11") return KEY_F11;
  if (key == "F12") return KEY_F12;
  return KEY_RESERVED;
}

void led(uint8_t pin, bool on) {
  digitalWrite(pin, on ? LOW : HIGH);
}

void errorPulse() {
  while (true) {
    led(LED, true);
    delay(1000);
    led(LED, false);
    delay(1000);
  }
}
