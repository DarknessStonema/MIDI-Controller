#include <Arduino.h>
#define MIDI_BAUD 31250

//MATRIX + GROUP PINS of the XR565 
const uint8_t matrixPins[12] = { 1,2,3,4,6,7,8,9,10,11,12,13 };
const uint8_t groupPins[5]   = { 20,21,22,26,27 };

//MUX PINS (TC4051BP) 
const uint8_t muxA   = 16;
const uint8_t muxB   = 17;
const uint8_t muxC   = 18;
const uint8_t muxCom = 28; // ADC2

//KEY STRUCTS 
struct Key {
  int group;
  int earlyLine;
  int lateLine;
  int midiNote;
};
struct KeyState {
  bool isDown = false;
  bool earlyClosed = false;
  uint32_t pressTime = 0;
};

//KEY MAPPING 
Key keys[27] = {
  {0, 9, 8, 46},  {0,11,10,47},  {0, 7, 6, 48},
  {1, 7, 6, 49},  {1, 9, 8, 50},  {1,11,10,51},
  {1, 5, 4, 52},  {1, 3, 2, 53},  {1, 1, 0, 54},
  {2, 1, 0, 55},  {2, 7, 6, 56},  {2, 5, 4, 57},
  {2,11,10,58},  {2, 9, 8, 59},  {2, 3, 2, 60},
  {3, 3, 2, 61},  {3, 1, 0, 62},  {3, 9, 8, 63},
  {3,11,10,64},  {3, 5, 4, 65},  {3, 7, 6, 66},
  {4, 7, 6, 67},  {4, 1, 0, 68},  {4, 5, 4, 69},
  {4, 9, 8, 71},  {4,11,10,70},  {4, 3, 2, 72}
};

KeyState keyStates[27];

//MIDI HELPERS 
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
inline void sendCC(uint8_t ccNumber, uint8_t value) {
  Serial1.write((uint8_t)0xB0);   // Control Change on channel 1
  Serial1.write(ccNumber);
  Serial1.write(value);
}

//SOFT THRU BUFFER 
const int MIDI_BUF_SIZE = 128;
uint8_t midiInBuf[MIDI_BUF_SIZE];
volatile int midiHead = 0, midiTail = 0;

inline void readMIDIInToBuffer() {
  while (Serial2.available()) {
    uint8_t b = (uint8_t)Serial2.read();
    int next = (midiHead + 1) % MIDI_BUF_SIZE;
    if (next != midiTail) {
      midiInBuf[midiHead] = b;
      midiHead = next;
    } else {
      // buffer full: drop oldest
      midiTail = (midiTail + 1) % MIDI_BUF_SIZE;
      midiInBuf[midiHead] = b;
      midiHead = next;
    }
  }
}

inline void flushMIDIThru() {
  while (midiTail != midiHead) {
    uint8_t b = midiInBuf[midiTail];
    midiTail = (midiTail + 1) % MIDI_BUF_SIZE;
    if (b == 0xFE) continue; // filter Active Sensing if desired
    Serial1.write(b);
  }
}

//SETUP 
void setup() {
  Serial1.setTX(0);  Serial1.begin(MIDI_BAUD); // MIDI OUT
  Serial2.setRX(7);  Serial2.begin(MIDI_BAUD); // MIDI IN

  for (int i=0;i<12;i++) pinMode(matrixPins[i], INPUT_PULLUP);
  for (int g=0; g<5; g++) { pinMode(groupPins[g], OUTPUT); digitalWrite(groupPins[g], HIGH); }

  pinMode(muxA, OUTPUT); pinMode(muxB, OUTPUT); pinMode(muxC, OUTPUT);
  analogReadResolution(12);
}

//CONTACT READER 
inline bool readContact(int group, int line) {
  digitalWrite(groupPins[group], LOW);
  delayMicroseconds(2);
  bool pressed = (digitalRead(matrixPins[line]) == LOW);
  digitalWrite(groupPins[group], HIGH);
  return pressed;
}

//MUX READER
inline int readMux(uint8_t channel) {
  digitalWrite(muxA, channel & 0x01);
  digitalWrite(muxB, (channel >> 1) & 0x01);
  digitalWrite(muxC, (channel >> 2) & 0x01);
  analogRead(muxCom); // dummy read
  int sum = 0;
  for (int i = 0; i < 4; i++) sum += analogRead(muxCom);
  return sum / 4;
}

//CC UPDATE
int muxValues[8];
uint8_t lastCCValues[8] = {0};

inline void updateMuxAndSendCC() {
  for (uint8_t ch = 0; ch < 8; ch++) {
    int raw = readMux(ch);
    uint8_t ccVal = map(raw, 0, 4095, 0, 127);
    if (ccVal != lastCCValues[ch]) {
      sendCC(20 + ch, ccVal);
      lastCCValues[ch] = ccVal;
    }
  }
}

//MAIN LOOP 
void loop() {
  //Buffer incoming MIDI
  readMIDIInToBuffer();

  //Update analogue controls
  updateMuxAndSendCC();

  //scan keys
  for (int i = 0; i < 27; i++) {
    Key &k = keys[i];
    KeyState &s = keyStates[i];

    bool early = readContact(k.group, k.earlyLine);
    bool late  = readContact(k.group, k.lateLine);

    if (!s.earlyClosed && early) {
      s.pressTime = micros();
      s.earlyClosed = true;
    }

    if (s.earlyClosed && !s.isDown && late) {
      uint32_t delta = micros() - s.pressTime;
      if (delta < 1800)  delta = 1800;
      if (delta > 20000) delta = 20000;
      int velocity = map((int)delta, 20000, 1800, 20, 127);
      velocity = constrain(velocity, 20, 127);
      sendNoteOn(k.midiNote, (uint8_t)velocity);
      s.isDown = true;
    }

    if (s.isDown && !early && !late) {
      sendNoteOff(k.midiNote);
      s.isDown = false;
      s.earlyClosed = false;
    }
  }

  //Flush buffered MIDI IN → OUT
  flushMIDIThru();
}
// coded with the help of Microsoft Copilot