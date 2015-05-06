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

/**@file
 * @brief Contains PHN_Midi which can drive the decoder chip to play instruments
 */

#include <Arduino.h>
#include "PHNCore.h"

#ifndef _PHN_MIDI_H_
#define _PHN_MIDI_H_

/**@brief Play music notes using a variety of instruments using MIDI with chip VS1053B
 *
 * Construct an instance of the class, then call begin() to put the chip into
 * MIDI mode to set it ready for playing MIDI. You can then set the instrument
 * and instrument bank to use, after which notes can be played.
 *
 * To play a full note, first turn it on (press) and then off (release). For each note the
 * channel of the instrument, pitch and attach velocity can be specified. Multiple notes
 * can be turned on at the same time.
 *
 * For further implementation, the write() and talkMIDI() functions can be utilized.
 * For a full list of commands, see the MIDI specification found here: 
 * http://www.midi.org/techspecs/midimessages.php
 */
class PHN_Midi {
 public:
  /// Initializes the chip into MIDI mode
  void begin();
  /// Sets the instrument to play with
  void setInstrument(byte instrument);
  /// Sets the instrument bank to use
  void setBank(byte bank);
  /// Turns a note on (press)
  void noteOn(byte channel, byte note, byte attack_velocity);
  /// turns a note off (release)
  void noteOff(byte channel, byte note, byte release_velocity);
  /// Sets whether a note is pressed or released, similar to noteOn/noteOff
  void note(byte channel, byte note, byte velocity, boolean pressed);
  /// Main MIDI command sending routine
  void talkMIDI(byte cmd, byte data1, byte data2);
  /// Plays a note at channel 0 and releases - useful for debugging
  void playNote(byte note);
  /// Writes a single byte to the VS1053 chip
  void write(uint8_t byte);
};

#endif