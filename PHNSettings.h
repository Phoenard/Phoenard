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
 * @brief Manages the loading and saving of settings in EEPROM with the PHN_Settings struct
 *
 * Settings are stored in the back of EEPROM and are used by the bootloader to perform actions.
 * When set in EEPROM, the bootloader can be told to switch to a new sketch. The current sketch
 * and sketch size is stored, as well as calibration values for the LCD screen.
 */
 
#include  <avr/eeprom.h>

#ifndef _PHN_SETTINGS_H_
#define _PHN_SETTINGS_H_

/* Flags used for status eeprom values */
/**@name Setting flags
 * @brief The available flags stored inside PHN_Settings
 */
//@{
/// Specifies a sketch has to be loaded from Micro-SD
#define SETTINGS_LOAD           0x01
/// Specifies that loading must be skipped, with program wiped
#define SETTINGS_LOADWIPE       0x02
/// Specifies that the current sketch is modified and needs saving
#define SETTINGS_MODIFIED       0x04
/// Specifies that touch input is inverted horizontally
#define SETTINGS_TOUCH_HOR_INV  0x08
/// Specifies that touch input is inverted vertically
#define SETTINGS_TOUCH_VER_INV  0x10
/// Specifies settings have been changed - never written to EEPROM
#define SETTINGS_CHANGED        0x80
//@}

/// Structure holding the settings stored in EEPROM
typedef struct {
  unsigned char touch_hor_a;
  unsigned char touch_hor_b;
  unsigned char touch_ver_a;
  unsigned char touch_ver_b;
  char sketch_toload[8];
  char sketch_current[8];
  unsigned long sketch_size;
  unsigned char flags;
} PHN_Settings;

/* Default Phoenard settings, used to restore corrupted or wiped EEPROM */
#define SETTINGS_DEFAULT_SKETCH        {'S', 'K', 'E', 'T', 'C', 'H', 'E', 'S'}
#define SETTINGS_DEFAULT_FLAGS         (SETTINGS_LOAD | SETTINGS_CHANGED | SETTINGS_TOUCH_HOR_INV)
#define SETTINGS_DEFAULT_TOUCH_HOR_A   100
#define SETTINGS_DEFAULT_TOUCH_HOR_B   100
#define SETTINGS_DEFAULT_TOUCH_VER_A   120
#define SETTINGS_DEFAULT_TOUCH_VER_B   120

const PHN_Settings SETTINGS_DEFAULT = {
  SETTINGS_DEFAULT_TOUCH_HOR_A, SETTINGS_DEFAULT_TOUCH_HOR_B,
  SETTINGS_DEFAULT_TOUCH_VER_A, SETTINGS_DEFAULT_TOUCH_VER_B,
  SETTINGS_DEFAULT_SKETCH, SETTINGS_DEFAULT_SKETCH, 0, SETTINGS_DEFAULT_FLAGS
};

/// The total size of the device EEPROM
#define EEPROM_SIZE           4096
/// The total size of the settings stored
#define EEPROM_SETTINGS_SIZE  sizeof(PHN_Settings)
/// The address in EEPROM where the settings are stored
#define EEPROM_SETTINGS_ADDR  (EEPROM_SIZE - EEPROM_SETTINGS_SIZE)
/// Macro to obtain the EEPROM address to a particular option field
#define EEPROM_SETTINGS_ADDR_FIELD(field)  (const uint8_t*) (EEPROM_SETTINGS_ADDR+offsetof(PHN_Settings, field))

/// Macro to load settings from EEPROM
#define PHN_Settings_Load(settings)  eeprom_read_block( &settings, (void*) EEPROM_SETTINGS_ADDR, EEPROM_SETTINGS_SIZE)
/// Macro to save settings to EEPROM
#define PHN_Settings_Save(settings)  eeprom_write_block(&settings, (void*) EEPROM_SETTINGS_ADDR, EEPROM_SETTINGS_SIZE)
/// Macro to load a single or a group of setting fields from EEPROM
#define PHN_Settings_LoadField(settings, field, size)  eeprom_read_block(&settings.field, EEPROM_SETTINGS_ADDR_FIELD(field), size)

/// Reads the screen calibration information from the settings
void PHN_Settings_ReadCali(PHN_Settings settings, int *hor_a, int *hor_b, int *ver_a, int *ver_b);
/// Writes the screen calibration information to the settings
void PHN_Settings_WriteCali(PHN_Settings *settings, int hor_a, int hor_b, int ver_a, int ver_b);
/// Requests the bootloader to load a particular sketch
void PHN_loadSketch(const char* sketchName, bool loadNow = true);

#endif