#include <USBHost_t36.h>
#include <MIDI.h>

// USB Host setup
USBHost myusb;
USBHub hub1(myusb);
KeyboardController usbKeyboard(myusb);

// MIDI setup
MIDI_CREATE_DEFAULT_INSTANCE();

// PS/2 setup
const int ps2ClockPin = 2;
const int ps2DataPin = 3;
volatile byte ps2Buffer[11];
volatile int ps2BitCount = 0;
volatile bool ps2Ready = false;

void setup() {
  Serial.begin(9600);
  while (!Serial && millis() < 4000);
  MIDI.begin(MIDI_CHANNEL_OMNI);
  myusb.begin();

  usbKeyboard.attachPress(onUSBKeyPress);

  // PS/2 pin setup
  pinMode(ps2ClockPin, INPUT_PULLUP);
  pinMode(ps2DataPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(ps2ClockPin), ps2ClockISR, FALLING);

  Serial.println("Ready to translate HID + PS/2 to MIDI");
}

void loop() {
  myusb.Task();

  if (ps2Ready) {
    byte scancode = decodePS2();
    int note = mapPS2ScancodeToNote(scancode);
    if (note >= 0) {
      MIDI.sendNoteOn(note, 100, 1);
      delay(10);
      MIDI.sendNoteOff(note, 0, 1);
      Serial.print("PS/2 key → MIDI note: ");
      Serial.println(note);
    }
    ps2Ready = false;
  }
}

// 🧠 USB HID key mapping
void onUSBKeyPress(int key) {
  int note = mapUSBKeyToNote(key);
  if (note >= 0) {
    MIDI.sendNoteOn(note, 100, 1);
    delay(10);
    MIDI.sendNoteOff(note, 0, 1);
    Serial.print("USB key → MIDI note: ");
    Serial.println(note);
  }
}

int mapUSBKeyToNote(int key) {
  switch (key) {
    case 'a': return 48; // C3
    case 's': return 50; // D3
    case 'd': return 52; // E3
    case 'f': return 53; // F3
    case 'g': return 55; // G3
    case 'h': return 57; // A3
    case 'j': return 59; // B3
    case 'k': return 60; // C4
    default: return -1;
  }
}

// 🧠 PS/2 interrupt handler
void ps2ClockISR() {
  static byte incomingByte = 0;
  static int bitIndex = 0;

  bool bit = digitalRead(ps2DataPin);
  if (bitIndex >= 1 && bitIndex <= 8) {
    incomingByte >>= 1;
    if (bit) incomingByte |= 0x80;
  }

  bitIndex++;
  if (bitIndex == 11) {
    ps2Buffer[0] = incomingByte;
    ps2Ready = true;
    bitIndex = 0;
    incomingByte = 0;
  }
}

byte decodePS2() {
  return ps2Buffer[0];
}

int mapPS2ScancodeToNote(byte code) {
  switch (code) {
    case 0x1C: return 48; // A → C3
    case 0x1B: return 50; // S → D3
    case 0x23: return 52; // D → E3
    case 0x2B: return 53; // F → F3
    case 0x34: return 55; // G → G3
    case 0x33: return 57; // H → A3
    case 0x3B: return 59; // J → B3
    case 0x42: return 60; // K → C4
    default: return -1;
  }
}
