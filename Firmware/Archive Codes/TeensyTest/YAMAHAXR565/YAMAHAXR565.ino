#include <Arduino.h>

//Yamaha XR565 Buttonmatrix

usb_midi_class &midi = usbMIDI;

const uint8_t matrixPins[12] = {
  0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11
};

const uint8_t groupPins[5] = {
  28, 27, 26, 25, 24
};

// ───── BUTTON CONFIGURATION ─────
// Each group has a list of buttons defined by pin pairs (even, odd)
const uint8_t buttonCount[5] = {3, 6, 6, 6, 6};

const uint8_t buttonPins[5][6][2] = {
  { {4, 3}, {2, 1}, {6, 5} },                                // Group 0
  { {5, 6}, {4, 3}, {2, 1}, {7, 8}, {9, 10}, {12, 11} },     // Group 1
  { {12, 11}, {6, 5}, {8, 7}, {1, 2}, {4, 3}, {9, 10} },     // Group 2
  { {9, 10}, {12, 11}, {4, 3}, {2, 1}, {8, 7}, {6, 5} },     // Group 3
  { {6, 5}, {11, 12}, {8, 7}, {1, 2}, {3, 4}, {10, 9} }      // Group 4
};

// MIDI notes assigned to each button
const uint8_t midiNotes[5][6] = {
  {60, 61, 62},
  {63, 64, 65, 66, 67, 68},
  {69, 70, 71, 72, 73, 74},
  {75, 76, 77, 78, 79, 80},
  {81, 82, 83, 84, 85, 86}
};

bool keyState[5][6] = {false};

void setup() {

  // Setup all matrix pins as inputs with pullups
  for (int i = 0; i < 12; i++) {
    pinMode(matrixPins[i], INPUT_PULLUP);
  }

  // Setup group pins as outputs and disable them (set HIGH)
  for (int i = 0; i < 5; i++) {
    pinMode(groupPins[i], OUTPUT);
    digitalWrite(groupPins[i], HIGH);
  }

  delay(1000);
}

void loop() {
  for (int g = 0; g < 5; g++) {
    // Activate one group line (pull LOW)
    digitalWrite(groupPins[g], LOW);
    delayMicroseconds(50); // settling time

    for (int k = 0; k < buttonCount[g]; k++) {
      uint8_t even = buttonPins[g][k][0] - 1;
      uint8_t odd  = buttonPins[g][k][1] - 1;

      if (even >= 12 || odd >= 12) continue;

      bool pressed = digitalRead(matrixPins[odd]) == LOW;

      if (pressed && !keyState[g][k]) {
        midi.sendNoteOn(midiNotes[g][k], 127, 1);
        keyState[g][k] = true;
      } else if (!pressed && keyState[g][k]) {
        midi.sendNoteOff(midiNotes[g][k], 0, 1);
        keyState[g][k] = false;
      }
    }

    digitalWrite(groupPins[g], HIGH); // deactivate group
  }

  midi.read(); // handle incoming messages (optional)
  delay(1);
}
