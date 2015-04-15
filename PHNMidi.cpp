/*
The MIT License (MIT)

This file is part of the Phoenard Arduino library
Copyright (c) 2014 Phoenard

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#include "PHNMidi.h"

void PHN_Midi::begin() {
  pinMode(VS1053_PIN_PWR, OUTPUT);
  pinMode(VS1053_PIN_GPIO, OUTPUT);
  digitalWrite(VS1053_PIN_PWR, HIGH);
  digitalWrite(VS1053_PIN_GPIO, HIGH);
  pinMode(VS1053_PIN_IRX, OUTPUT);
  digitalWrite(VS1053_PIN_IRX, HIGH);

  // Reset the VS1053
  pinMode(VS1053_PIN_RESET, OUTPUT);
  digitalWrite(VS1053_PIN_RESET, LOW);
  delay(100);
  digitalWrite(VS1053_PIN_RESET, HIGH);
  delay(100);

  // Set default instrument bank and instrument ID
  talkMIDI(0xB0, 0x07, 125); //0xB0 is channel message, set channel volume to near max (127)
  setInstrument(8); // Default instrument #8 - Clavi
}

void PHN_Midi::setBank(byte bank) {
  talkMIDI(0xB0, 0, bank);
}

void PHN_Midi::setInstrument(byte instrument) {
  //Set instrument number. 0xC0 is a 1 data byte command
  talkMIDI(0xC0, instrument+1, 0);
}

//Send a MIDI note-on message.  Like pressing a piano key
//channel ranges from 0-15
void PHN_Midi::noteOn(byte channel, byte note, byte attack_velocity) {
  this->note(channel, note, attack_velocity, true);
}

//Send a MIDI note-off message.  Like releasing a piano key
void PHN_Midi::noteOff(byte channel, byte note, byte release_velocity) {
  this->note(channel, note, release_velocity, false);
}

//Send a MIDI note message specifying whether the note is pressed or released
void PHN_Midi::note(byte channel, byte note, byte velocity, boolean pressed) {
  byte cmd = channel;
  if (pressed) {
    cmd |= 0x90;
  } else {
    cmd |= 0x80;
  }
  talkMIDI(cmd, note, velocity);
}

//Plays a MIDI note. Doesn't check to see that cmd is greater than 127, or that data values are less than 127
void PHN_Midi::talkMIDI(byte cmd, byte data1, byte data2) {
  write(cmd);
  write(data1);

  //Some commands only have one data byte. All cmds less than 0xBn have 2 data bytes 
  //(sort of: http://253.ccarh.org/handout/midiprotocol/)
  if( (cmd & 0xF0) <= 0xB0)
    write(data2);
}

void PHN_Midi::write(uint8_t b) {
  // Save old interrupts, then disable them shortly
  uint8_t SREG_old = SREG;
  cli();

  // Write out 10 bits in a single loop
  // The first bit is 0, the last bit is 1
  // A delay is used to write out at 31250 baud
  uint16_t data = b;
  data |= (uint16_t) 0x1 << 10;
  data <<= 1;
  do {
    if (data & 0x1) {
      VS1053_IRX_PORT |= (1 << VS1053_IRX_PIN);
    } else {
      VS1053_IRX_PORT &= ~(1 << VS1053_IRX_PIN);
    }
    data >>= 1;
    delayMicroseconds(30);
  } while (data);

  // Restore interrupts
  SREG = SREG_old;
}

void PHN_Midi::playNote(byte note) {
  noteOn(0, note, 60);
  delay(50);
  //Turn off the note with a given off/release velocity
  noteOff(0, note, 60);
  delay(50);
}