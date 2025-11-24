#include <Arduino.h>

#define MIDI_BAUD 31250

// Adjust to your wiring
const uint8_t matrixPins[12] = {2,3,4,5,6,7,8,9,10,11,12,13};
const uint8_t groupPins[5]   = {21,22,26,27,28};

bool state[60]; // enough for 5*12 = 60 contacts

// ───── MIDI helpers ─────
void sendNoteOn(uint8_t note) {
  Serial1.write((uint8_t)0x90); // Note On, channel 1
  Serial1.write(note);
  Serial1.write((uint8_t)100);  // fixed velocity
}

void sendNoteOff(uint8_t note) {
  Serial1.write((uint8_t)0x80); // Note Off, channel 1
  Serial1.write(note);
  Serial1.write((uint8_t)0);
}

void setup() {
  Serial1.setTX(0); // GPIO0 → DIN MIDI out
  Serial1.begin(MIDI_BAUD);

  for (int i=0; i<12; i++) pinMode(matrixPins[i], INPUT_PULLUP);
  for (int g=0; g<5; g++) {
    pinMode(groupPins[g], OUTPUT);
    digitalWrite(groupPins[g], HIGH);
  }
}

void loop() {
  int idx = 0;
  for (int g=0; g<5; g++) {
    digitalWrite(groupPins[g], LOW);
    delayMicroseconds(50);

    for (int k=0; k<12; k++) {
      if (idx >= 60) break;
      bool pressed = (digitalRead(matrixPins[k]) == LOW);
      uint8_t testNote = 40 + idx; // each index = unique note number

      if (pressed && !state[idx]) {
        sendNoteOn(testNote);
        state[idx] = true;
      }
      else if (!pressed && state[idx]) {
        sendNoteOff(testNote);
        state[idx] = false;
      }

      idx++;
    }

    digitalWrite(groupPins[g], HIGH);
  }
}
