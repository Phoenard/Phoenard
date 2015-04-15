/*
 * Sketch list - the default program that runs to display, edit and load sketches from the SD-card
 */

#include "editicons.c"
#include  <avr/eeprom.h>
#include "Phoenard.h"

extern uint16_t touch_x, touch_y;
extern uint8_t touch_downctr;
extern uint8_t touch_waitup;

/* General defines */
#define LCD_RIGHT_BORDER    32
#define HOLD_ACTIVATE_DELAY 1000

/* Sketches list settings */
#define SKETCHES_CNT_X      4
#define SKETCHES_CNT_Y      3
#define SKETCHES_CNT        (SKETCHES_CNT_X * SKETCHES_CNT_Y)
#define SKETCHES_OFF_X      5
#define SKETCHES_OFF_Y      5
#define SKETCHES_STP_X      70
#define SKETCHES_STP_Y      78
#define SKETCHES_ICON_W     64
#define SKETCHES_ICON_H     64
#define SKETCHES_PGINCR     SKETCHES_CNT
#define SKETCHES_FULLW      (SKETCHES_CNT_X * SKETCHES_STP_X)
#define SKETCHES_FULLH      (SKETCHES_CNT_Y * SKETCHES_STP_Y)
#define SKETCHES_TXT_XINCR  7

/* Edit dialog settings */
#define EDIT_ICON_SCALE     3
#define EDIT_ICON_W         (EDIT_ICON_SCALE * SKETCHES_ICON_W)
#define EDIT_ICON_H         (EDIT_ICON_SCALE * SKETCHES_ICON_H)
#define EDIT_ICON_X         ((PHNDisplayHW::WIDTH - EDIT_ICON_W) / 2)
#define EDIT_ICON_Y         5
#define EDIT_NAME_X         (EDIT_ICON_X + (EDIT_ICON_SCALE * 5))
#define EDIT_NAME_Y         (EDIT_ICON_Y + EDIT_ICON_H + EDIT_ICON_SCALE)
#define EDIT_NAME_YOFF      6
#define EDIT_NAME_SCALE     EDIT_ICON_SCALE
#define EDIT_NAME_XINCR     (EDIT_ICON_SCALE * SKETCHES_TXT_XINCR)
#define EDIT_NAME_W         (8 * EDIT_NAME_XINCR)
#define EDIT_NAME_H         (PHNDisplayHW::HEIGHT - EDIT_NAME_Y)
#define EDIT_SAVEINTER      200

#define EDIT_IDX_START      0
#define EDIT_IDX_COLOR      (EDIT_IDX_START+0)
#define EDIT_IDX_RESET      (EDIT_IDX_START+1)
#define EDIT_IDX_CANCEL     (EDIT_IDX_START+2)
#define EDIT_IDX_DELETE     (EDIT_IDX_START+3)
#define EDIT_IDX_RENAME     (EDIT_IDX_START+4)
#define EDIT_IDX_ACCEPT     (EDIT_IDX_START+5)
#define EDIT_IDX_NONE       (EDIT_IDX_START+6)

/* Special main screen icon indices */
#define IDX_START       0
#define IDX_MENU_START  SKETCHES_CNT
#define IDX_UP          (IDX_MENU_START+0)
#define IDX_ADD         (IDX_MENU_START+1)
#define IDX_DOWN        (IDX_MENU_START+2)
#define IDX_NONE        (IDX_MENU_START+3)

/* Menu icon settings */
#define MENU_STP_Y   SKETCHES_STP_Y
#define MENU_ICON_W  32
#define MENU_ICON_H  64
#define MENU_OFF_X   (PHNDisplayHW::WIDTH-MENU_ICON_W)
#define MENU_OFF_Y   5

/* The colors used when drawing UI */
#define COLOR_C1      WHITE_8BIT
#define COLOR_C2      BLACK_8BIT
#define COLOR_C1_SEL  COLOR_C2
#define COLOR_C2_SEL  COLOR_C1

void setLoadOptions(const char* sketchName, unsigned char options) {
  PHN_Settings settings;
  PHN_Settings_Load(settings);
  if (sketchName) {
    memcpy(settings.sketch_toload, sketchName, 8);
  } else {
    memcpy(settings.sketch_toload, settings.sketch_current, 8);
  }
  settings.flags &= ~(SETTINGS_LOAD | SETTINGS_LOADWIPE);
  settings.flags |= options;
  PHN_Settings_Save(settings);
}

void loadSketchReset(char* filename) {
  /* Clicked a sketch, load it */
  setLoadOptions(filename, SETTINGS_LOAD);

  /* Software reset using watchdog */
  WDTCSR=(1<<WDE) | (1<<WDCE);
  WDTCSR= (1<<WDE);
  for(;;);
}

void setup() {
  showSketches();
}

void loop() {
}

void showSketches() {
  /* First fill the screen with BLACK */
  PHNDisplay8Bit::fill(BLACK_8BIT);
  
  uint16_t sketch_offset = 0;
  boolean redrawAll = true;
  for (;;) {
    unsigned char sketch_count;
    char sketch_name[SKETCHES_CNT][9];
    char sketch_icon_dirty[IDX_NONE+1];
    for (uint8_t i = 0; i < sizeof(sketch_icon_dirty); i++) {
      sketch_icon_dirty[i] = 1;
    }
    
    if (redrawAll) {
      redrawAll = false;
      PHNDisplay8Bit::fill(BLACK_8BIT);
    }

    /* First wipe the sketches list from any contents */
    PHNDisplay8Bit::fillRect(SKETCHES_OFF_X, SKETCHES_OFF_Y, SKETCHES_FULLW, SKETCHES_FULLH, BLACK_8BIT);

    /* Clear touch input */
    LCD_clearTouch();

    /* List all available sketches */
    sketch_count = file_list_sketches(sketch_offset, SKETCHES_CNT, sketch_name);

    /* Proceed to draw all sketches, handle update logic */
    uint8_t oldTouchedIndex;
    uint8_t touchedIndex = IDX_NONE;
    long pressed_time_start;
    for (;;) {
      /* Update touch input */
      LCD_updateTouch();

      /* Draw/update UI */
      uint8_t i = -1;
      oldTouchedIndex = touchedIndex;
      touchedIndex = IDX_NONE;
      for (uint16_t y = 0; y < SKETCHES_CNT_Y; y++) {
        for (uint16_t x = 0; x < SKETCHES_CNT_X; x++) {
          i++;
          
          /* Slot out of range */
          if (i >= sketch_count) {
            continue;
          }

          unsigned int px = SKETCHES_OFF_X + x * SKETCHES_STP_X;
          unsigned int py = SKETCHES_OFF_Y + y * SKETCHES_STP_Y;

          /* Handle touch input */
          if (LCD_isTouched(px, py, SKETCHES_ICON_W, SKETCHES_ICON_H)) {
            touchedIndex = i;
          }

          /* Redraw the sketch if needed */
          if (sketch_icon_dirty[i]) {
            sketch_icon_dirty[i] = 0;

            /* Pressed? */
            uint8_t color1, color2;
            if (touchedIndex == i) {
              color1 = COLOR_C1_SEL;
              color2 = COLOR_C2_SEL;
            } else {
              color1 = COLOR_C1;
              color2 = COLOR_C2;
            }

            /* Draw the sketch icon */
            uint8_t* icon_data = file_open(sketch_name[i], "SKI", FILE_READ) ? volume_cacheCurrentBlock(0) : icon_sketch_default;
            LCD_write_icon(px, py, SKETCHES_ICON_W, SKETCHES_ICON_H, icon_data, sketch_name[i], color1, color2, COLOR_C1);
          }
        }
      }

      /* Draw and update all the other icons */
      uint8_t* menu_icons[] = {icon_up, icon_add, icon_down};
      for (i = IDX_MENU_START; i < IDX_NONE; i++) {
        unsigned int px = MENU_OFF_X;
        unsigned int py = MENU_OFF_Y + (MENU_STP_Y*(i-IDX_MENU_START));

        if (LCD_isTouched(px, py, MENU_ICON_W + LCD_RIGHT_BORDER, MENU_ICON_H)) {
          touchedIndex = i;
        }

        if (sketch_icon_dirty[i]) {
          sketch_icon_dirty[i] = 0;

          /* Pressed? */
          uint8_t color1, color2;
          if (touchedIndex == i) {
            color1 = COLOR_C1_SEL;
            color2 = COLOR_C2_SEL;
          } else {
            color1 = COLOR_C1;
            color2 = COLOR_C2;
          }

          /* Draw the sketch icon */
          uint8_t* icon_data = menu_icons[i-IDX_MENU_START];
          PHNDisplay8Bit::writeImage_1bit(px, py, MENU_ICON_W, MENU_ICON_H, 1, icon_data, DIR_RIGHT, color1, color2);
        }
      }

      /* Redraw icons when touched index changes */
      if (oldTouchedIndex != touchedIndex) {
        sketch_icon_dirty[oldTouchedIndex] = 1;
        sketch_icon_dirty[touchedIndex] = 1;
        pressed_time_start = millis();
      }

      if (touchedIndex == IDX_NONE) {
        /* Released the touchscreen? */
        if (!LCD_isTouchedAny() && oldTouchedIndex != IDX_NONE) {
          if (oldTouchedIndex >= IDX_MENU_START) {
            if (oldTouchedIndex == IDX_UP) {
              /* Move sketch list one page up, ignore if impossible */
              if (!sketch_offset) {
                continue;
              }
              sketch_offset -=  SKETCHES_PGINCR;
            } else if (oldTouchedIndex == IDX_DOWN) {
              /* Move sketch list one page down, ignore if impossible */
              if (sketch_count != SKETCHES_CNT) {
                continue;
              }
              sketch_offset += SKETCHES_PGINCR;
            } else if (oldTouchedIndex == IDX_ADD) {
              /* Ask for the file name, if successful, edit the icon */
              char name[8];
              memset(name, ' ' , 8);
              if (askSketchName(name)) {
                editSketch(name, true);
              } else {
                redrawAll = true;
              }
            }

            /* In menu, break out and re-initialize the sketches */
            break;
          } else {
            /* Clicked a sketch, load it */
            loadSketchReset(sketch_name[oldTouchedIndex]);
          }
        }
      } else if (touchedIndex < IDX_MENU_START && ((millis() - pressed_time_start) > HOLD_ACTIVATE_DELAY)) {
        /* Edit the selected sketch */
        editSketch(sketch_name[touchedIndex], false);

        /* In menu, break out and re-initialize the sketches */
        break;
      }
    }
  }
}

void editSketch(char filename[9], boolean runWhenExit) {
  /* Mark sketch for loading */
  setLoadOptions(filename, SETTINGS_LOAD | SETTINGS_LOADWIPE);

  /* First fill the screen with BLACK */
  PHNDisplay8Bit::fill(BLACK_8BIT);

  /* Open or create the .SKI file, store the FAT clusters where the (file) data resides in */
  uint32_t ski_data_block = 0;
  uint32_t hex_file_length;
  FilePtr ski_file;
  FilePtr hex_file;

  /* Open or create the .HEX file, store pointer to file directory */
  if (!file_open(filename, "HEX", FILE_READ)) {
    file_open(filename, "HEX", FILE_WRITE);
    file_flush();
  }
  hex_file = file_dir_;
  hex_file_length = file_available;

  /* Load icon data, otherwise create a new default icon */
  if (file_open(filename, "SKI", FILE_READ) && file_available) {
    /* Load the icon from SD */
    volume_cacheCurrentBlock(0);
    ski_data_block = volume_cacheBlockNumber_;
  } else {
    /* Set to the default icon */
    file_open(filename, "SKI", FILE_WRITE);
    volume_cacheCurrentBlock(1);
    memcpy(volume_cacheBuffer_.data, icon_sketch_default, 512);
    ski_data_block = volume_cacheBlockNumber_;
    file_available = 512;
    volume_cacheDirty_ = 1;

    /* Flush data to the SKI file, updating the available length */
    file_flush();
  }
  ski_file = file_dir_;

  /* Make sure the icon is cached */
  volume_readCache(ski_data_block);

  /* Wait for touch to be released */
  LCD_clearTouch();

  /* Set up the UI button icons and states */
  unsigned char* button_icon[EDIT_IDX_NONE] = {
    edit_icon_changecolor_1, edit_icon_reset,  edit_icon_cancel,
    edit_icon_delete,        edit_icon_charup, edit_icon_accept
  };
  unsigned char button_dirty[EDIT_IDX_NONE + 1] = {1, 1, 1, 1, 1, 1 ,1};
  const char* button_title[EDIT_IDX_NONE] = {
    "Color",  "Reset",  "Cancel",
    "Delete", "Rename", "Done"
  };
  const uint8_t button_color[EDIT_IDX_NONE] = {
    BLUE_8BIT, BLUE_8BIT, RED_8BIT,
    BLUE_8BIT, BLUE_8BIT, GREEN_8BIT
  };
  uint8_t oldTouchedIndex;
  uint8_t touchedIndex = EDIT_IDX_NONE;

  /* Store old icon and filename data for restoring (cancel) */
  uint8_t  data_original[512];
  char filename_original[8];
  memcpy(data_original, volume_cacheBuffer_.data, 512);
  memcpy(filename_original, filename, 8);

  boolean file_needs_saving = false;
  uint8_t draw_color = COLOR_C1;
  boolean edit_finish = false;
  long last_save = millis(); // Monitors the last time the icon data was saved
  long last_done_pressed = 0; // Monitors how long the 'done' button is pressed
  boolean needsRedraw = true;
  while (!edit_finish) {
    /* Update touch input */
    LCD_updateTouch();

    /* At all times ensure the icon data is the one in cache */
    volume_readCache(ski_data_block);

    /* If main icon needs to be redrawn, do so */
    if (needsRedraw) {
      PHNDisplay8Bit::writeImage_1bit(EDIT_ICON_X, EDIT_ICON_Y, SKETCHES_ICON_W, SKETCHES_ICON_H, EDIT_ICON_SCALE, volume_cacheBuffer_.data, DIR_RIGHT, COLOR_C1, COLOR_C2);
    }

    /* If touching paint area, update buffer and screen */
    if (LCD_isTouched(EDIT_ICON_X, EDIT_ICON_Y, EDIT_ICON_W, EDIT_ICON_H)) {
      uint8_t p_x = (touch_x - EDIT_ICON_X) / EDIT_ICON_SCALE;
      uint8_t p_y = (touch_y - EDIT_ICON_Y) / EDIT_ICON_SCALE;
      uint16_t data_idx = (p_y * (SKETCHES_ICON_W / 8)) + (p_x / 8);
      uint8_t  data_msk = (1 << (0x7 - (p_x & 0x7)));

      if (draw_color == COLOR_C1) {
        *(volume_cacheBuffer_.data + data_idx) |= data_msk;
      } else {
        *(volume_cacheBuffer_.data + data_idx) &= ~data_msk;
      }

      PHNDisplay8Bit::fillRect(EDIT_ICON_X + (p_x * EDIT_ICON_SCALE), EDIT_ICON_Y + (p_y * EDIT_ICON_SCALE), EDIT_ICON_SCALE, EDIT_ICON_SCALE, draw_color);

      // Mark cache dirty, notify it was changed
      volume_cacheDirty_ = 1;
    }

    /* If button is touched, update touched index (states) */
    oldTouchedIndex = touchedIndex;
    touchedIndex = EDIT_IDX_NONE;
    uint8_t i = -1;
    for (uint8_t x = 0; x < 2; x++) {
      for (uint8_t y = 0; y < 3; y++) {
        i++;
        
        unsigned int py = 5 + y * 65 + (y == 2) * 40;
        unsigned int px = 5 + x * (PHNDisplayHW::WIDTH - 58);
        if (LCD_isTouched(px, py, 48, 48)) {
          touchedIndex = i;
        }
        if (button_dirty[i] || needsRedraw) {
          button_dirty[i] = 0;
          
          /* Pressed? */
          uint8_t color;
          if (touchedIndex == i) {
            color = WHITE_8BIT;
          } else {
            color = button_color[i];
          }

          /* Draw the sketch icon */
          LCD_write_icon(px, py, 48, 48, button_icon[i], button_title[i], color);
        }
      }
    }

    /* Redraw text when changed */
    if (needsRedraw) {
      LCD_write_string(EDIT_NAME_X, EDIT_NAME_Y + EDIT_NAME_YOFF, EDIT_NAME_SCALE, filename, COLOR_C1, BLACK_8BIT);
    }

    /* Redraw icons when touched index changes */
    if (oldTouchedIndex != touchedIndex) {
      button_dirty[oldTouchedIndex] = 1;
      button_dirty[touchedIndex] = 1;
    }

    /* No longer needs to be redrawn */
    needsRedraw = false;

    /* Handle accept button pressed for some time */
    if (touchedIndex == EDIT_IDX_ACCEPT) {
      if (oldTouchedIndex != touchedIndex) {
        /* First press, store time */
        last_done_pressed = millis();
      } else if ((millis() - last_done_pressed) > HOLD_ACTIVATE_DELAY) {
        /* Pressed for a given amount of time, run sketch and done */
        runWhenExit = true;
        edit_finish = true;
      }
    }

    /* Handle button presses */
    if (touchedIndex == EDIT_IDX_NONE) {
      /* Released the touchscreen? */
      if (!LCD_isTouchedAny() && oldTouchedIndex != EDIT_IDX_NONE) {

        switch (oldTouchedIndex) {
          case EDIT_IDX_COLOR:
            {
              if (draw_color == COLOR_C1) {
                draw_color = COLOR_C2;
                button_icon[EDIT_IDX_COLOR] = edit_icon_changecolor_0;
              } else {
                draw_color = COLOR_C1;
                button_icon[EDIT_IDX_COLOR] = edit_icon_changecolor_1;
              }
            }
            break;

          case EDIT_IDX_RESET:
            {
              /* Resets the icon to the default icon */
              memcpy(volume_cacheBuffer_.data, icon_sketch_default, 512);
              volume_cacheDirty_ = 1;
              needsRedraw = true;
            }
            break;
            
          case EDIT_IDX_DELETE:
            {
              /* First ask the user if he is absolutely sure */
              PHNDisplay8Bit::fill(BLACK_8BIT);
              boolean cancel_down = false;
              boolean accept_down = false;
              const uint16_t text_off = 40;
              const uint16_t cancel_x = 80;
              const uint16_t cancel_y = 120;
              const uint16_t accept_x = (PHNDisplayHW::WIDTH - cancel_x - 48);
              const uint16_t accept_y = cancel_y;
              
              /* Print icons and text message to user for first draw */
              LCD_write_icon(cancel_x, cancel_y, 48, 48, edit_icon_cancel, "Cancel", RED_8BIT);
              LCD_write_icon(accept_x, accept_y, 48, 48, edit_icon_accept, "Delete", GREEN_8BIT);
              LCD_write_string(text_off, text_off, 2,      "Are you sure you want", WHITE_8BIT, BLACK_8BIT);
              LCD_write_string(text_off, text_off + 16, 2, "to permanently delete", WHITE_8BIT, BLACK_8BIT);
              LCD_write_string(text_off, text_off + 32, 2, "the sketch?",           WHITE_8BIT, BLACK_8BIT);
              
              for (;;) {
                LCD_updateTouch();
                
                if (LCD_isTouched(cancel_x, cancel_y, 48, 48) != cancel_down) {
                  cancel_down = !cancel_down;
                  LCD_write_icon(cancel_x, cancel_y, 48, 48, edit_icon_cancel, "Cancel", cancel_down ? WHITE_8BIT : RED_8BIT);
                  if (!cancel_down && !LCD_isTouchedAny()) {
                    // Just break and continue
                    needsRedraw = true;
                    break;
                  }
                }
                if (LCD_isTouched(accept_x, accept_y, 48, 48) != accept_down) {
                  accept_down = !accept_down;
                  LCD_write_icon(accept_x, accept_y, 48, 48, edit_icon_accept, "Delete", accept_down ? WHITE_8BIT : GREEN_8BIT);
                  if (!accept_down && !LCD_isTouchedAny()) {
                    // Actually perform deletion
                    /* Delete icon file */
                    file_dir_ = ski_file;
                    file_delete();

                    /* Delete HEX file */
                    file_dir_ = hex_file;
                    file_delete();

                    /* If asked to load - don't load! */
                    runWhenExit = false;

                    /* Finish */
                    edit_finish = true;
                    break;
                  }
                }
              }
              PHNDisplay8Bit::fill(BLACK_8BIT);
            }
            break;
            
          case EDIT_IDX_RENAME:
            {
              if (askSketchName(filename)) {
                /* Rename the SKI file */
                file_dir_ = ski_file;
                file_save(filename);

                /* Rename the HEX file */
                file_dir_ = hex_file;
                file_available = hex_file_length;
                file_save(filename);
              }
              needsRedraw = true;
              PHNDisplay8Bit::fill(BLACK_8BIT);
            }
            break;
            
          case EDIT_IDX_CANCEL:
            {
              /* Restore icon and filename */
              memcpy(volume_cacheBuffer_.data, data_original, 512);
              memcpy(filename, filename_original, 8);
              volume_cacheDirty_ = 1;

              /* Rename the SKI file */
              file_dir_ = ski_file;
              file_save(filename);

              /* Rename the HEX file */
              file_dir_ = hex_file;
              file_available = hex_file_length;
              file_save(filename);
              
              /* Don't load anything */
              runWhenExit = false;

              edit_finish = true;
            }
            break;
          
          case EDIT_IDX_ACCEPT:
            {
              edit_finish = true;
            }
            break;
        }
      }
    }

    /* Reset save counter when data changes */
    if (!volume_cacheDirty_) {
       file_needs_saving = 0;
    } else if (!file_needs_saving) {
       file_needs_saving = 1;
       last_save = millis();
    }

    /* Write to SD every 1 second */
    if (volume_cacheDirty_ && (((millis() - last_save) >= EDIT_SAVEINTER) || edit_finish)) {
      volume_writeCache();
    }
  }

  if (runWhenExit) {
    /* Request to load the sketch - do so */
    loadSketchReset(filename);
  } else {
    /* We ended up here, undo the setting of loading the sketch */
    setLoadOptions(NULL, 0);
  }

  /* Wipe the area next to the sketchlist */
  PHNDisplay8Bit::fillRect(SKETCHES_OFF_X, SKETCHES_OFF_Y, PHNDisplayHW::WIDTH - SKETCHES_OFF_X, SKETCHES_FULLH, BLACK_8BIT);
}

/* Shows a dialog allowing the user to enter a (new) file name */
/* File name is validated to be free and SD-compatible before returning */
boolean askSketchName(char name[8]) {
  // Check if requesting a new file name
  boolean isNewSketch = true;
  for (uint8_t i = 0; i < 8; i++) {
    if (name[i] != ' ') {
      isNewSketch = false;
    }
  }

  // Fill with a white, then black, background
  const uint8_t KEYBOARD_HEIGHT = 167;
  PHNDisplayHW::setCursor(0, 0);
  PHNDisplay8Bit::writePixelLines(GRAY_8BIT, KEYBOARD_HEIGHT);
  PHNDisplay8Bit::writePixelLines(BLACK_8BIT, PHNDisplayHW::HEIGHT - KEYBOARD_HEIGHT);

  // Put a filled square underneath the spacebar
  const uint16_t GREY_SPACE_X = 65;
  const uint16_t GREY_SPACE_H = 31;
  PHNDisplay8Bit::fillRect(GREY_SPACE_X, KEYBOARD_HEIGHT, PHNDisplayHW::WIDTH - GREY_SPACE_X * 2, GREY_SPACE_H, GRAY_8BIT);

  // Store the new file name as a buffer; only transfer if accepted
  char resultFilename[8];
  memcpy(resultFilename, name, 8);

  const char NAME_MAP[] = {
     '!', '@', '#', '$', '%', '^', '&', '~', '(', ')', '-', '+',
     '1', '2', '3', '4', '5', '6', '7', '8', '9', '0', '_', '=',
     'Q', 'W', 'E', 'R', 'T', 'Y', 'U', 'I', 'O', 'P', '[', ']',
     'A', 'S', 'D', 'F', 'G', 'H', 'J', 'K', 'L', ';', '{', '}',
     'Z', 'X', 'C', 'V', 'B', 'N', 'M', ',', '.', '<', ' '
  };
  const uint8_t BUTTON_COUNT = sizeof(NAME_MAP) + 2;
  const uint8_t KEY_BACKSPACE_IDX = sizeof(NAME_MAP) - 2;
  const uint8_t KEY_SPACE_IDX = sizeof(NAME_MAP) - 1;
  const uint8_t KEY_CANCEL_IDX = KEY_SPACE_IDX + 1;
  const uint8_t KEY_ACCEPT_IDX = KEY_SPACE_IDX + 2;
  const uint8_t KEYBOARD_COLS = 12;
  const uint8_t KEY_WIDTH = 23;
  const uint8_t KEY_HEIGHT = 29;
  const uint8_t KEY_SPACING = 3;
  
  uint16_t x, y;
  uint16_t w, h;
  
  boolean redrawAll = true;
  uint8_t index_touched = 0xFF;
  uint8_t index_selChar = 0;
  uint8_t index_dirty_a = 0xFF;
  uint8_t index_dirty_b = 0xFF;
  uint8_t index_dirty_c = 0xFF;
  uint8_t index_dirty_d = 0xFF;
  char *popupMessage = NULL;
  for (;;) {
    uint16_t col = 0;
    uint16_t row = 0;
    LCD_updateTouch();
    
    for (uint16_t index = 0; index < BUTTON_COUNT; index++) {
      if (index == KEY_CANCEL_IDX) {
        // Cancel button
        x = 8;
        y = (PHNDisplayHW::HEIGHT - 65);
        w = 48;
        h = 48;
      } else if (index == KEY_ACCEPT_IDX) {
        // Accept button
        x = (PHNDisplayHW::WIDTH - 8 - 48);
        y = (PHNDisplayHW::HEIGHT - 65);
        w = 48;
        h = 48;
      } else {
        // Keyboard
        x = 5 + col * (KEY_WIDTH + KEY_SPACING);
        y = 4 + row * (KEY_HEIGHT + KEY_SPACING);
        h = KEY_HEIGHT;
        if (index == KEY_BACKSPACE_IDX) {
          w = (3 * KEY_WIDTH + 2 * KEY_SPACING);
          col += 2;
        } else if (index == KEY_SPACE_IDX) {
          w = (7 * KEY_WIDTH + 6 * KEY_SPACING);
          x = ((PHNDisplayHW::WIDTH - w) / 2);
        } else {
          w = KEY_WIDTH;
        }
      }
      if (LCD_isTouched(x, y, w, h) != (index_touched == index)) {
        index_dirty_a = index_touched;
        if (index_touched == index) {
          index_touched = 0xFF;
          
          // If nothing is pressed now, we completed a full press - release
          if (!LCD_isTouchedAny()) {
            if (index == KEY_CANCEL_IDX) {
              // Cancel pressed
              return false;
            } else if (index == KEY_ACCEPT_IDX) {
              // Accept pressed
              // First trim any spaces off the file name
              char trimmed[8];
              boolean nonspace = false;
              uint8_t j = 0;
              for (uint8_t i = 0; i < 8; i++) {
                if (nonspace || resultFilename[i] != ' ') {
                  nonspace = true;
                  trimmed[j++] = resultFilename[i];
                }
              }
              while (j < 8) {
                trimmed[j++] = ' ';
              }
              if (nonspace) {
                if (memcmp(resultFilename, name, 8) == 0) {
                  // If file name is left the same, allow right away
                  return true;
                } else if (file_open(trimmed, "HEX", FILE_READ)) {
                  // If file name already exists on the SD card, do not allow
                  popupMessage = "Name is already used";
                } else {
                  // (Re)naming is allowed - set the filename and return true
                  memcpy(name, trimmed, 8);
                  return true;
                }
              } else {
                // If file name is left empty (all spaces), do not allow
                popupMessage = "Please enter a name";
              }
            } else if (index == KEY_BACKSPACE_IDX) {
              // Backspace button
              if (index_selChar < 8) {
                if (index_selChar && resultFilename[index_selChar] == ' ') {
                  memcpy(resultFilename + index_selChar - 1, resultFilename + index_selChar, 8 - index_selChar);
                  resultFilename[7] = ' ';
                } else {
                  resultFilename[index_selChar] = ' ';
                  index_selChar++;
                }
              }
              if (index_selChar) {
                index_selChar--;
              }
              redrawAll = true;
            } else {
              // Keyboard pressed
              if (index_selChar < 8) {
                index_dirty_c = index_selChar;
                resultFilename[index_selChar++] = NAME_MAP[index];
                index_dirty_d = index_selChar;
              }
            }
          }
        } else {
          index_dirty_b = index_touched = index;
        }
      }
      
      boolean dirty = redrawAll;
      if (index == index_dirty_a) {
        index_dirty_a = 0xFF;
        dirty = true;
      }
      if (index == index_dirty_b) {
        index_dirty_b = 0xFF;
        dirty = true;
      }
      if (dirty) {
        boolean pressed = (index == index_touched);
        if (index == KEY_CANCEL_IDX) {
          // Draw cancel button
          uint8_t color = pressed ? WHITE_8BIT : RED_8BIT;
          PHNDisplay8Bit::writeImage_1bit(x, y, w, h, 1, edit_icon_cancel, DIR_RIGHT, color, BLACK_8BIT);
          LCD_write_string(x + 6, y + 50, 1, "Cancel", color, BLACK_8BIT);
        } else if (index == KEY_ACCEPT_IDX) {
          // Draw accept button
          uint8_t color = pressed ? WHITE_8BIT : GREEN_8BIT;
          const char* title = isNewSketch ? "Add sketch" : "Rename";
          LCD_write_icon(x, y, w, h, edit_icon_accept, title, color);
        } else {
          // Draw keyboard icons
          uint8_t color = pressed ? BLUE_8BIT : WHITE_8BIT;
          PHNDisplay8Bit::fillRect(x, y, w, h, color);
          PHNDisplay8Bit::drawRect(x, y, w, h, BLACK_8BIT);
          LCD_write_font(x + 4, y + 3, 3, NAME_MAP[index], BLACK_8BIT, color);
          if (index == KEY_BACKSPACE_IDX) {
            PHNDisplay8Bit::fillRect(x + 10, y + 12, w - 20, 3, BLACK_8BIT);
          }
        }
      }
      if (++col >= KEYBOARD_COLS) {
        col = 0;
        row++;
      }
    }
    
    // Handle changes to the name select
    for (uint8_t index = 0; index < 8; index++) {
      x = (index * 24 + 64);
      y = (PHNDisplayHW::HEIGHT - 35);
      w = 20;
      h = 30;
      
      if (LCD_isTouched(x, y, w, h) && (index_selChar != index)) {
        index_dirty_c = index_selChar;
        index_selChar = index;
        index_dirty_d = index;
      }
      boolean dirty = redrawAll;
      if (index_dirty_c == index) {
        index_dirty_c = 0xFF;
        dirty = true;
      }
      if (index_dirty_d == index) {
        index_dirty_d = 0xFF;
        dirty = true;
      }
      if (dirty) {
        uint8_t color_a, color_b;
        if (index == index_selChar) {
          color_a = WHITE_8BIT;
          color_b = WHITE_8BIT;
        } else {
          color_a = BLUE_8BIT;
          color_b = BLACK_8BIT;
        }
        LCD_write_font(x, y, 4, resultFilename[index], color_a, BLACK_8BIT);
        PHNDisplay8Bit::drawLine(x, y + h, w, DIR_RIGHT, color_b);
      }
    }
    redrawAll = false;

    // Show popup messages to the user if needed
    if (popupMessage != NULL) {
      const uint16_t popup_w = 280;
      const uint16_t popup_h = 32;
      const uint16_t popup_x = (PHNDisplayHW::WIDTH - popup_w) / 2;
      const uint16_t popup_y = (PHNDisplayHW::HEIGHT - popup_h) / 2;
      uint16_t txt_xoff = (popup_w - strlen(popupMessage) * 12) / 2;
      
      PHNDisplay8Bit::fillRect(popup_x, popup_y, popup_w, popup_h, WHITE_8BIT);
      PHNDisplay8Bit::drawRect(popup_x, popup_y, popup_w, popup_h, RED_8BIT);
      LCD_write_string(popup_x + txt_xoff, popup_y + 10, 2, popupMessage, RED_8BIT, WHITE_8BIT);
      delay(800);
      PHNDisplay8Bit::fillRect(popup_x, popup_y, popup_w, popup_h, GRAY_8BIT);
      redrawAll = true;
      popupMessage = NULL;
    }
  }
}