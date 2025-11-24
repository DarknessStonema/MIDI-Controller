#include <USBHost_t36.h>
#include <Arduino.h>
USBHost myusb;
KeyboardController keyboard(myusb);

void setup() {
  Serial.begin(115200);
  myusb.begin();
}

void loop() {
  // Continuously check for USB keyboard input
  myusb.Task();
}

// Called when a key is pressed
void keyPressed() {
  char key = keyboard.getKey();
  uint8_t note = 0;

  // QWERTZ key to MIDI mapping
  switch (key) {
    case 'y': note = 60; break;  // C4
    case 's': note = 61; break;  // C#4
    case 'x': note = 62; break;  // D4
    case 'd': note = 63; break;  // D#4
    case 'c': note = 64; break;  // E4
    case 'v': note = 65; break;  // F4
    case 'g': note = 66; break;  // F#4
    case 'b': note = 67; break;  // G4
    case 'h': note = 68; break;  // G#4
    case 'n': note = 69; break;  // A4
    case 'j': note = 70; break;  // A#4
    case 'm': note = 71; break;  // B4
    case ',': note = 72; break;  // C5
  }

  if (note > 0) {
    usbMIDI.sendNoteOn(note, 100, 1);  // note, velocity, channel
    Serial.print("Note On: "); Serial.println(note);
  }
}

// Called when a key is released
void keyReleased() {
  char key = keyboard.getKey();
  uint8_t note = 0;

  switch (key) {
    case 'y': note = 60; break;
    case 's': note = 61; break;
    case 'x': note = 62; break;
    case 'd': note = 63; break;
    case 'c': note = 64; break;
    case 'v': note = 65; break;
    case 'g': note = 66; break;
    case 'b': note = 67; break;
    case 'h': note = 68; break;
    case 'n': note = 69; break;
    case 'j': note = 70; break;
    case 'm': note = 71; break;
    case ',': note = 72; break;
  }

  if (note > 0) {
    usbMIDI.sendNoteOff(note, 0, 1);  // note, velocity, channel
    Serial.print("Note Off: "); Serial.println(note);
  }
}
