#include <Arduino.h>
#define MIDI_BAUD 31250

// ───── MATRIX + GROUP PINS ─────
// Adjust to match your wiring
const uint8_t matrixPins[12] = {
  1,2,3,4,6,7,8,9,10,11,12,13
};
const uint8_t groupPins[5] = {
  20,21,22,26,27
};

// ───── MUX PINS (TC4051BP) ─────
const uint8_t muxA = 16;
const uint8_t muxB = 17;
const uint8_t muxC = 18;
const uint8_t muxCom = 28; // ADC2

// ───── KEY STRUCTS ─────
struct Key {
  int group;
  int earlyLine;
  int lateLine;
  int midiNote;
};
struct KeyState {
  bool isDown;
  bool earlyClosed;
  uint32_t pressTime;
};

// mapping 
Key keys[27] = {
  {0, 9, 8, 46},  // A#3 underlying but not used
  {0,11,10,47},  // B3 underlying but not used
  {0, 7, 6, 48}, // C4
  {1, 7, 6, 49}, // C#4
  {1, 9, 8, 50}, // D4
  {1,11,10,51},  // D#4
  {1, 5, 4, 52}, // E4
  {1, 3, 2, 53}, // F4
  {1, 1, 0, 54}, // F#4
  {2, 1, 0, 55}, // G4
  {2, 7, 6, 56}, // G#4
  {2, 5, 4, 57}, // A4
  {2, 11, 10, 58},  // A#4
  {2, 9, 8, 59}, // B4
  {2, 3, 2, 60}, // C5
  {3, 3, 2, 61}, // C#5
  {3, 1, 0, 62}, // D5
  {3, 9, 8, 63}, // D#5
  {3, 11, 10, 64},  // E5
  {3, 5, 4, 65}, // F5
  {3, 7, 6, 66}, // F#5
  {4, 7, 6, 67}, // G5
  {4, 1, 0, 68}, // G#5
  {4, 5, 4, 69}, // A5
  {4, 9, 8, 71}, // A#5
  {4,11,10,70}, // B5
  {4, 3, 2, 72}  // C6
};

KeyState keyStates[27];

// ───── MIDI HELPERS ─────
inline void sendNoteOn(uint8_t note, uint8_t velocity) {
  Serial1.write((uint8_t)0x90);
  Serial1.write(note);
  Serial1.write(velocity);
}
inline void sendNoteOff(uint8_t note) {
  Serial1.write((uint8_t)0x80);
  Serial1.write(note);
  Serial1.write((uint8_t)0);
}

// ───── SETUP ─────
void setup() {
  // MIDI OUT on GPIO0 (UART0 TX)
  Serial1.setTX(0);
  Serial1.begin(MIDI_BAUD);

  // MIDI IN on GPIO7 (UART1 RX)
  Serial2.setRX(7);
  Serial2.begin(MIDI_BAUD);

  // Matrix IO
  for (int i=0;i<12;i++) pinMode(matrixPins[i], INPUT_PULLUP);
  for (int g=0; g<5; g++) { pinMode(groupPins[g], OUTPUT); digitalWrite(groupPins[g], HIGH); }

  // Mux address pins
  pinMode(muxA, OUTPUT);
  pinMode(muxB, OUTPUT);
  pinMode(muxC, OUTPUT);

  analogReadResolution(12); // 12‑bit ADC
}

// ───── CONTACT READER ─────
bool readContact(int group, int line) {
  digitalWrite(groupPins[group], LOW);
  delayMicroseconds(5);
  bool pressed = (digitalRead(matrixPins[line]) == LOW);
  digitalWrite(groupPins[group], HIGH);
  return pressed;
}

// ───── MUX READER ─────
int readMux(uint8_t channel) {
  digitalWrite(muxA, channel & 0x01);
  digitalWrite(muxB, (channel >> 1) & 0x01);
  digitalWrite(muxC, (channel >> 2) & 0x01);
  delayMicroseconds(5);
  return analogRead(muxCom);
}

// ───── MAIN LOOP ─────
void loop() {
  // Soft Thru: forward MIDI IN → OUT
  while (Serial2.available()) {
    Serial1.write((uint8_t)Serial2.read());
  }

  // Example: read mux channel 0 (adjust as needed)
  int muxVal = readMux(0);

  // ───── KEY SCANNING LOOP ─────
  for (int i = 0; i < 27; i++) {
    Key &k = keys[i];
    KeyState &s = keyStates[i];

    bool early = readContact(k.group, k.earlyLine);
    bool late  = readContact(k.group, k.lateLine);

    // Early contact pressed
    if (!s.earlyClosed && early) {
      s.pressTime = micros();   // timestamp in microseconds
      s.earlyClosed = true;
    }

    // Late contact pressed after early
    if (s.earlyClosed && !s.isDown && late) {
      uint32_t delta = micros() - s.pressTime;

      // Clamp delta to avoid extreme values
      if (delta < 1800)   delta = 1800;     // very fast press
      if (delta > 20000) delta = 20000;   // very slow press

      // Simple linear mapping: fast press = 127, slow press = 20
      int velocity = map((int)delta, 20000, 1800, 20, 127);
      velocity = constrain(velocity, 20, 127);

      sendNoteOn(k.midiNote, velocity);
      s.isDown = true;
    }

    // Key released (both contacts open)
    if (s.isDown && !early && !late) {
      sendNoteOff(k.midiNote);
      s.isDown = false;
      s.earlyClosed = false;
    }
  }

}
