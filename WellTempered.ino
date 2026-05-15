#include <SPI.h>

#define DAC1_CS 2
#define DAC2_CS 3
#define GATE1 16
#define GATE2 15
#define TEMP_POT 18

// Werckmeister III cents deviation from equal temperament for each semitone
// Values in cents: positive = sharper, negative = flatter
// source: https://tunableapp.com/temperaments/werckmeister-iii/
const float werckmeister3[12] = {
  0.0,    // C
  -9.775,  // C#
  -7.82,  // D
  -5.865,  // Eb
  -9.775,  // E
  -1.955,  // F
  -11.73, // F#
  -3.91,  // G
  -7.82,  // Ab
  -11.73,  // A
  -3.91,  // Bb
  -7.82   // B
};

void writeDAC(uint8_t cs, uint16_t value) {
  uint16_t cmd = value & 0x0FFF;
  cmd |= (1 << 12);

  SPI.beginTransaction(SPISettings(1000000, MSBFIRST, SPI_MODE0));
  digitalWrite(cs, LOW);
  SPI.transfer16(cmd);
  digitalWrite(cs, HIGH);
  SPI.endTransaction();
}

uint16_t noteToDac(uint8_t note, bool wellTempered) {
  int offsetNote = (int)note - 36;
  offsetNote = constrain(offsetNote, 0, 60);
  
  float dacFloat = offsetNote * 55.5f;
  
  if (wellTempered) {
    // Apply Werckmeister III correction
    int semitone = note % 12;
    // Convert cents to DAC steps
    // 100 cents = 1 semitone = 55.5 DAC steps
    float correction = werckmeister3[semitone] * 55.5f / 100.0f;
    dacFloat += correction;
  }
  
  return (uint16_t)constrain(round(dacFloat), 0, 4095);
}

void setup() {
  Serial.begin(9600);
  pinMode(DAC1_CS, OUTPUT);
  pinMode(DAC2_CS, OUTPUT);
  pinMode(GATE1, OUTPUT);
  pinMode(GATE2, OUTPUT);
  pinMode(TEMP_POT, INPUT);
  digitalWrite(DAC1_CS, HIGH);
  digitalWrite(DAC2_CS, HIGH);
  digitalWrite(GATE1, HIGH);
  digitalWrite(GATE2, HIGH);
  SPI.begin();

  writeDAC(DAC1_CS, 1332);
  writeDAC(DAC2_CS, 1332);
  Serial.println("Ready - outputting 2V until MIDI received");
}

void loop() {
  // Read pot — above 512 = well tempered, below = equal
  int potValue = analogRead(TEMP_POT);
  bool wellTempered = potValue > 512;

  if (usbMIDI.read()) {
    uint8_t type = usbMIDI.getType();
    uint8_t channel = usbMIDI.getChannel();
    uint8_t note = usbMIDI.getData1();
    uint8_t velocity = usbMIDI.getData2();

    Serial.print("Channel: "); Serial.print(channel);
    Serial.print("  Note: "); Serial.print(note);
    Serial.print("  Mode: "); Serial.print(wellTempered ? "Werckmeister III" : "Equal");
    Serial.print("  DAC: "); Serial.println(noteToDac(note, wellTempered));

    if (type == usbMIDI.NoteOn && velocity > 0) {
      uint16_t dacValue = noteToDac(note, wellTempered);

      if (channel == 1) {
        writeDAC(DAC1_CS, dacValue);
        digitalWrite(GATE1, HIGH);
        delayMicroseconds(5000);
        digitalWrite(GATE1, LOW);
      } else if (channel == 2) {
        writeDAC(DAC2_CS, dacValue);
        digitalWrite(GATE2, HIGH);
        delayMicroseconds(5000);
        digitalWrite(GATE2, LOW);
      }
    } else if (type == usbMIDI.NoteOff ||
               (type == usbMIDI.NoteOn && velocity == 0)) {
      if (channel == 1) digitalWrite(GATE1, HIGH);
      else if (channel == 2) digitalWrite(GATE2, HIGH);
    }
  }
}