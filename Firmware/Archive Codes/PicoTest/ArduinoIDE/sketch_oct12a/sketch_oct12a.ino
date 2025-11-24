#include <Arduino.h>

#define MIDI_BAUD 31250

// ───── MATRIX SETUP ─────
// 12 matrix input lines (GPIO2–13)
const uint8_t matrixPins[12] = {2,3,4,5,6,7,8,9,10,11,12,13};
// 5 group output lines (GPIO21,22,26,27,28)
const uint8_t groupPins[5]   = {21,22,26,27,28};

// ───── NOTE MAP ─────
// 27 keys total, disable 1, 2, and 27
// Button 3 = C5 (72), then chromatic up to B6 (95)
const int midiNotes[27] = {
   -1,   // Button 1 disabled
   -1,   // Button 2 disabled
   72,   // Button 3 → C5
   73,   // Button 4 → C#5
   74,   // Button 5 → D5
   75,   // Button 6 → D#5
   76,   // Button 7 → E5
   77,   // Button 8 → F5
   78,   // Button 9 → F#5
   79,   // Button 10 → G5
   80,   // Button 11 → G#5
   81,   // Button 12 → A5
   82,   // Button 13 → A#5
   83,   // Button 14 → B5
   84,   // Button 15 → C6
   85,   // Button 16 → C#6
   86,   // Button 17 → D6
   87,   // Button 18 → D#6
   88,   // Button 19 → E6
   89,   // Button 20 → F6
   90,   // Button 21 → F#6
   91,   // Button 22 → G6
   92,   // Button 23 → G#6
   93,   // Button 24 → A6
   94,   // Button 25 → A#6
   95,   // Button 26 → B6
   -1    // Button 27 disabled
};

// Track key states (27 keys)
bool keyState[27] = {false};

// ───── SETUP ─────
void setup() {
  // DIN MIDI on UART0 (GPIO0 TX, GPIO1 RX)
  Serial1.setTX(0);
  Serial1.setRX(1);
  Serial1.begin(MIDI_BAUD);

  // Configure matrix pins
  for (int i=0; i<12; i++) {
    pinMode(matrixPins[i], INPUT_PULLUP);
  }
  for (int g=0; g<5; g++) {
    pinMode(groupPins[g], OUTPUT);
    digitalWrite(groupPins[g], HIGH); // inactive
  }
}

// ───── MIDI SEND HELPERS ─────
void sendNoteOn(uint8_t note, uint8_t velocity, uint8_t channel = 0) {
  Serial1.write((uint8_t)(0x90 | (channel & 0x0F)));
  Serial1.write(note);
  Serial1.write(velocity);
}

void sendNoteOff(uint8_t note, uint8_t velocity, uint8_t channel = 0) {
  Serial1.write((uint8_t)(0x80 | (channel & 0x0F)));
  Serial1.write(note);
  Serial1.write(velocity);
}

// ───── MAIN LOOP ─────
void loop() {
  int keyIndex = 0; // 0–26 across groups × 12 lines

  for (int g=0; g<5; g++) {
    digitalWrite(groupPins[g], LOW); // activate group
    delayMicroseconds(50);

    for (int k=0; k<12; k++) {
      if (keyIndex >= 27) break; // only 27 keys total

      int note = midiNotes[keyIndex];
      if (note >= 0) {
        bool pressed = (digitalRead(matrixPins[k]) == LOW);

        if (pressed != keyState[keyIndex]) {
          delay(5); // debounce
          bool confirm = (digitalRead(matrixPins[k]) == LOW);

          if (confirm && !keyState[keyIndex]) {
            sendNoteOn(note, 127);
            keyState[keyIndex] = true;
          } else if (!confirm && keyState[keyIndex]) {
            sendNoteOff(note, 0);
            keyState[keyIndex] = false;
          }
        }
      }
      keyIndex++;
    }

    digitalWrite(groupPins[g], HIGH); // deactivate group
  }
}
