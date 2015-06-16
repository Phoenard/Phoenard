/*
 * Sketch list - the default program that runs to display, edit and load sketches from the SD-card
 */

#include "icons.c"
#include  <avr/eeprom.h>
#include "Phoenard.h"

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

/* Menu icon settings */
#define MENU_STP_Y   SKETCHES_STP_Y
#define MENU_ICON_W  32
#define MENU_ICON_H  64
#define MENU_OFF_X   (PHNDisplayHW::WIDTH-MENU_ICON_W)
#define MENU_OFF_Y   5

/* Menu icon indices */
#define MENU_IDX_START       0
#define MENU_IDX_UP          0
#define MENU_IDX_ADD         1
#define MENU_IDX_DOWN        2
#define MENU_IDX_SKET        3
#define MENU_IDX_NONE        (MENU_IDX_SKET + SKETCHES_CNT)
#define MENU_IDX_CNT         (MENU_IDX_NONE + 1)

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
#define EDIT_IDX_DONE       (EDIT_IDX_START+5)
#define EDIT_IDX_NONE       (EDIT_IDX_START+6)

/* The colors used when drawing the main sketch menu */
#define COLOR_MENU_BG    BLACK_8BIT
#define COLOR_MENU_SEL   WHITE_8BIT
#define COLOR_MENU_ADD   GREEN_8BIT
#define COLOR_MENU_NAV   BLUE_8BIT
#define COLOR_MENU_SKET  GRAY_LIGHT_8BIT

/* The colors used when drawing the edit menu */
#define COLOR_EDIT_BG    BLACK_8BIT
#define COLOR_EDIT_ICON  WHITE_8BIT
#define COLOR_EDIT_SEL   WHITE_8BIT

/* The colors used when drawing the sketch naming menu */
#define COLOR_NAME_BG       BLACK_8BIT /* Background color of dialog */
#define COLOR_NAME_NAM_NOR  BLUE_8BIT  /* Color for text that is not selected */
#define COLOR_NAME_NAM_SEL  WHITE_8BIT /* Color for text that is selected */
#define COLOR_NAME_KEY_BG   GRAY_8BIT  /* Background color behind keys */
#define COLOR_NAME_KEY_TXT  BLACK_8BIT /* Key text and border color */
#define COLOR_NAME_KEY_FOR  WHITE_8BIT /* Key foreground color */
#define COLOR_NAME_KEY_SEL  BLUE_8BIT  /* Key foreground color when pressed */

/* LCD touch input variables */
uint16_t touch_x, touch_y;
boolean touch_waitup;

/* Sketch information buffer */
typedef struct {
  char name[8];
  uint32_t icon;
} SketchInfo;

/*
 * The first 100 sketch entries (1.2kb) are stored in internal RAM
 * The remaining ~2730 entries are stored on the External SRAM chip
 */
SketchInfo sketches_buff[100];
int sketches_buff_cnt = 0;
const int sketches_sram_start = -sizeof(sketches_buff);
boolean sketches_reachedEnd = false;

/* Variables used by the main sketch list showing logic */
char sketch_icon_text[MENU_IDX_CNT][9];
char sketch_icon_dirty[MENU_IDX_CNT];
int sketch_offset = 0;
boolean reloadAll = true;
boolean redrawIcons;
uint8_t touchedIndex;
long pressed_time_start;

void setup() {
  /* Initialize SRAM for buffering >100 sketches */
  sram.begin();

  /* Set pin 13 (LED) to output */
  pinMode(13, OUTPUT);
}

void loop() {
  /* In case the Micro-SD library fails, attempt to re-initialize */
  if (reloadAll || !volume.isInitialized) {
    digitalWrite(13, LOW);
    updateVolume();
    digitalWrite(13, HIGH);
    reloadAll = true;
  }

  /* When reloading, initialize all fields to the defaults */
  if (reloadAll) {
    redrawIcons = true;
    touchedIndex = MENU_IDX_NONE;
    LCD_clearTouch();
    sketches_buff_cnt = 0;
    sketches_reachedEnd = false;
    volume_init(1);
    reloadAll = false;
    PHNDisplay8Bit::fill(COLOR_MENU_BG);
  }

  /* Clear the sketch icon area and redraw all icons */
  if (redrawIcons) {
    redrawIcons = false;
    PHNDisplay8Bit::fillRect(SKETCHES_OFF_X, SKETCHES_OFF_Y, SKETCHES_FULLW, SKETCHES_FULLH, COLOR_MENU_BG);
    memset(sketch_icon_dirty, 1, sizeof(sketch_icon_dirty));
  }

  /* Pre-load the next block of file listing */
  if (!sketches_reachedEnd) {
    volume_cacheCurrentBlock(0);
  }

  /*
   * General sketch list update loop starts here
   * All variables used in it are declared below.
   */
  uint16_t icon_px;       /* X-coordinate of icon */
  uint16_t icon_py;       /* Y-coordinate of icon */
  uint8_t icon_w;         /* Width of the icon */
  uint8_t icon_h;         /* Height of the icon */
  uint8_t icon_idx;       /* Icon index in main sketch menu */
  int16_t sketch_index;   /* Index in the sketch buffer */
  int16_t sketch_addr;    /* SRAM address in the sketch buffer */
  SketchInfo sketch_info; /* Sketch entry information */

  /* Routinely buffer in sketch information from the Micro-SD in the background */
  for (uint8_t i = 0; i < 16 && !sketches_reachedEnd; i++) {
    /* Read next directory entry */
    SDMINFAT::dir_t* p = (SDMINFAT::dir_t*) file_read(32);

    /* Reached the end of the root directory? */
    if (p->name[0] == SDMINFAT::DIR_NAME_FREE) {
      sketches_reachedEnd = true;
      break;
    }

    /* Check if not deleted / main sketch */
    if (p->name[0] == SDMINFAT::DIR_NAME_DELETED) continue;
    if (!memcmp("SKETCHES", p->name, 8)) continue;

    /* Check file extension for SKI/HEX */
    boolean is_ski = !memcmp("SKI", p->name+8, 3);
    if (!is_ski && memcmp("HEX", p->name+8, 3)) continue;

    /* Locate this sketch name in the memory buffer */
    sketch_index = 0;
    sketch_addr = sketches_sram_start;
    boolean create_new = true;
    while (sketch_index < sketches_buff_cnt) {
      if (sketch_addr >= 0) {
        /* Reading SRAM space - turn the SD-card off */
        card_setEnabled(false);
        create_new = !sram.verifyBlock(sketch_addr, (char*) p->name, 8);
      } else {
        create_new = memcmp(sketches_buff[sketch_index].name, p->name, 8);
      }
      if (!create_new) {
        break;
      }
      sketch_index++;
      sketch_addr += sizeof(SketchInfo);
    }

    /* Create new entry if not found */
    if (create_new) {
      sketches_buff_cnt++;
      memcpy(sketch_info.name, p->name, 8);
      sketch_info.icon = 0;
      if (sketch_addr >= 0) {
        sram.writeBlock(sketch_addr, (char*) &sketch_info, sizeof(SketchInfo));
      } else {
        sketches_buff[sketch_index] = sketch_info;
      }
    }
    /* If SKI file, set icon location info */
    if (is_ski) {
      uint32_t icon_cluster = ((uint32_t) p->firstClusterHigh << 16) | p->firstClusterLow;
      uint32_t icon_block = volume_firstClusterBlock(icon_cluster);
      if (sketch_addr >= 0) {
        sram.writeBlock(sketch_addr + 8, (char*) &icon_block, sizeof(uint32_t));
      } else {
        sketches_buff[sketch_index].icon = icon_block;
      }
    }
    /* If required, mark this sketch dirty for rendering */
    icon_idx = (sketch_index - sketch_offset);
    if ((create_new || is_ski) && (sketch_index >= sketch_offset) && (icon_idx < SKETCHES_CNT)) {
      sketch_icon_dirty[MENU_IDX_SKET + icon_idx] = 1;
    }
  }

  /* Ensure SD-card is active at all times */
  card_setEnabled(true);

  /* Update touch input */
  LCD_updateTouch();
  uint8_t newTouchedIndex = MENU_IDX_NONE;

  /* Draw and update all the main menu icons */
  icon_px = MENU_OFF_X;
  icon_py = MENU_OFF_Y;
  icon_w = MENU_ICON_W;
  icon_h = MENU_ICON_H;
  icon_idx = 0;
  sketch_index = sketch_offset - MENU_IDX_SKET;
  do {
    if (LCD_isTouched(icon_px, icon_py, icon_w, icon_h)) {
      newTouchedIndex = icon_idx;
    }
    if (sketch_icon_dirty[icon_idx]) {
      sketch_icon_dirty[icon_idx] = 0;

      uint8_t icon_color;
      const uint8_t* icon_data;

      /* Update icon, icon color and title */
      memset(sketch_icon_text, 0, 9);
      switch (icon_idx) {
        case MENU_IDX_ADD:
          /* Add sketch button */
          icon_color = COLOR_MENU_ADD;
          icon_data = icon_add;
          break;
        case MENU_IDX_UP:
          /* Up navigation button */
          icon_color = COLOR_MENU_NAV;
          icon_data = icon_up;
          break;
        case MENU_IDX_DOWN:
          /* Down navigation button */
          icon_color = COLOR_MENU_NAV;
          icon_data = icon_down;
          break;
        default:
          /* Get sketch information to display */
          sketch_addr = sketches_sram_start + sketch_index * sizeof(SketchInfo);
          if (sketch_addr >= 0) {
            card_setEnabled(false);
            sram.readBlock(sketch_addr, (char*) &sketch_info, sizeof(SketchInfo));
            card_setEnabled(true);
          } else {
            sketch_info = sketches_buff[sketch_index];
          }

          /* Refresh the sketch color, name and icon */
          memcpy(sketch_icon_text[icon_idx], sketch_info.name, 8);
          icon_color = COLOR_MENU_SKET;
          icon_data = icon_sketch_default;
          if (sketch_info.icon) {
            volume_readCache(sketch_info.icon);
            icon_data = volume_cacheBuffer.data;
          }
          break;
      }
      if (newTouchedIndex == icon_idx) {
        icon_color = COLOR_MENU_SEL;
      }

      /* Draw icon */
      LCD_write_icon(icon_px, icon_py, icon_w, icon_h, icon_data, sketch_icon_text[icon_idx], COLOR_MENU_BG, icon_color);

      /* Stop here to allow for fluent updates */
      break;
    }

    /* Update indices and icon bounds */
    icon_idx++;
    sketch_index++;
    if (icon_idx < MENU_IDX_SKET) {
      icon_py += MENU_STP_Y;
    } else if (icon_idx == MENU_IDX_SKET) {
      icon_px = SKETCHES_OFF_X;
      icon_py = SKETCHES_OFF_Y;
      icon_w = SKETCHES_ICON_W;
      icon_h = SKETCHES_ICON_H;
    } else {
      icon_px += SKETCHES_STP_X;
      if (icon_px >= (SKETCHES_OFF_X + SKETCHES_CNT_X * SKETCHES_STP_X)) {
        icon_px = SKETCHES_OFF_X;
        icon_py += SKETCHES_STP_Y;
      }
    }
  } while ((icon_idx < MENU_IDX_NONE) && (sketch_index < sketches_buff_cnt));

  /* Handle button presses here */
  if (touchedIndex != MENU_IDX_NONE) {
    /* Open edit menu when holding down on a sketch icon for some time */
    if (touchedIndex >= MENU_IDX_SKET && ((millis() - pressed_time_start) > HOLD_ACTIVATE_DELAY)) {
      editSketch(sketch_icon_text[touchedIndex], false);
      reloadAll = true;

    } else if (!LCD_isTouchedAny()) {
      /* Perform actions when tapping an icon */
      switch (touchedIndex) {
        case MENU_IDX_UP:
          /* Move sketch list one page up, ignore if impossible */
          if (sketch_offset) {
            sketch_offset -=  SKETCHES_PGINCR;
            redrawIcons = true;
          }
          break;
        case MENU_IDX_DOWN:
          /* Move sketch list one page down, ignore if impossible */
          if ((sketch_offset + SKETCHES_PGINCR) < sketches_buff_cnt) {
            sketch_offset += SKETCHES_PGINCR;
            redrawIcons = true;
          }
          break;
        case MENU_IDX_ADD:
          /* Ask for the new file name, if successful, edit the icon */
          memset(sketch_icon_text[MENU_IDX_SKET], ' ' , 8);
          if (askSketchName(sketch_icon_text[MENU_IDX_SKET])) {
            editSketch(sketch_icon_text[MENU_IDX_SKET], true);
          }
          reloadAll = true;
          break;
        default:
          /* Clicked a sketch, load it */
          loadSketchReset(sketch_icon_text[touchedIndex]);
          break;
      }
    }
  }

  /* Redraw icons when touched index changes */
  if (touchedIndex != newTouchedIndex) {
    sketch_icon_dirty[touchedIndex] = 1;
    sketch_icon_dirty[newTouchedIndex] = 1;
    pressed_time_start = millis();
    touchedIndex = newTouchedIndex;
  }
}

void editSketch(char filename[9], boolean runWhenExit) {
  /* Make sure to re-initialize the SD volume to prevent corruption */
  updateVolume();

  /* Mark sketch for loading */
  setLoadOptions(filename, SETTINGS_LOAD | SETTINGS_LOADWIPE);

  /* Open or create the hex/ski files and store their locations on the Micro-SD */
  uint32_t ski_data_block = 0;
  uint32_t ski_file_length = 0;
  uint32_t hex_file_length = 0;
  FilePtr ski_file;
  FilePtr hex_file;

  /* Open or create the .SKI file */
  if (file_open(filename, "SKI", SDMIN_FILE_CREATE)) {
    ski_file = file_curDir;
    file_position = 0;
    if (!file_size) {
      volume_cacheCurrentBlock(1);
      memcpy(volume_cacheBuffer.data, icon_sketch_default, 512);
      file_size = 512;
      volume_cacheDirty = 1;
      file_flush();
    }
    volume_cacheCurrentBlock(0);
    ski_data_block = volume_cacheBlockNumber;
    ski_file_length = file_size;
  }

  /* Open or create the .HEX file, store pointer to file directory */
  if (file_open(filename, "HEX", SDMIN_FILE_CREATE)) {
    hex_file = file_curDir;
    hex_file_length = file_size;
  }

  /* Wait for touch to be released */
  LCD_clearTouch();

  /* Set up the UI button icons and states */
  const unsigned char* button_icon[EDIT_IDX_NONE] = {
    edit_icon_changecolor_1, edit_icon_reset,  edit_icon_cancel,
    edit_icon_delete,        edit_icon_charup, edit_icon_accept
  };
  unsigned char button_dirty[EDIT_IDX_NONE + 1];
  const char* button_title[EDIT_IDX_NONE] = {
    "Color",  "Reset",  "Cancel",
    "Delete", "Rename", "Done"
  };
  const uint8_t button_color[EDIT_IDX_NONE] = {
    BLUE_8BIT, BLUE_8BIT, RED_8BIT,
    BLUE_8BIT, BLUE_8BIT, GREEN_8BIT
  };
  uint8_t touchedIndex = EDIT_IDX_NONE;
  uint8_t newTouchedIndex;

  /* Store old icon and filename data for restoring (cancel) */
  volume_readCache(ski_data_block);
  uint8_t icon_original[512];
  char filename_original[8];
  memcpy(icon_original, volume_cacheBuffer.data, 512);
  memcpy(filename_original, filename, 8);

  uint8_t icon_idx;
  uint8_t icon_color;
  uint8_t draw_color = COLOR_EDIT_ICON;
  boolean edit_finish = false;
  long last_save = millis(); // Monitors the last time the icon data was saved
  long last_done_pressed = 0; // Monitors how long the 'done' button is pressed
  boolean needsRedraw = true;
  boolean needsIconRedraw = true;
  boolean needsEntryUpdate = false; // Whether file entry needs to be updated
  boolean iconDirty = false; // Whether icon is dirty and needs to be re-saved at some point
  while (!edit_finish) {
    /* Update touch input */
    LCD_updateTouch();

    /* At all times ensure the icon data is the one in cache */
    volume_readCache(ski_data_block);

    /* Redraw static content when changed */
    if (needsRedraw) {
      needsRedraw = false;
      PHNDisplay8Bit::fill(COLOR_EDIT_BG);
      PHNDisplay8Bit::writeString(EDIT_NAME_X, EDIT_NAME_Y + EDIT_NAME_YOFF, EDIT_NAME_SCALE, filename, COLOR_EDIT_BG, COLOR_EDIT_ICON);
      memset(button_dirty, 1, sizeof(button_dirty));
      needsIconRedraw = true;
    }
    if (needsIconRedraw) {
      needsIconRedraw = false;
      PHNDisplay8Bit::writeImage_1bit(EDIT_ICON_X, EDIT_ICON_Y, SKETCHES_ICON_W, SKETCHES_ICON_H, EDIT_ICON_SCALE, volume_cacheBuffer.data, DIR_RIGHT, COLOR_EDIT_BG, COLOR_EDIT_ICON);
    }

    /* If touching paint area, update buffer and screen */
    if (LCD_isTouched(EDIT_ICON_X, EDIT_ICON_Y, EDIT_ICON_W, EDIT_ICON_H)) {
      uint8_t p_x = (touch_x - EDIT_ICON_X) / EDIT_ICON_SCALE;
      uint8_t p_y = (touch_y - EDIT_ICON_Y) / EDIT_ICON_SCALE;
      uint16_t data_idx = (p_y * (SKETCHES_ICON_W / 8)) + (p_x / 8);
      uint8_t  data_msk = (1 << (p_x & 0x7));

      if (draw_color == COLOR_EDIT_ICON) {
        *(volume_cacheBuffer.data + data_idx) |= data_msk;
      } else {
        *(volume_cacheBuffer.data + data_idx) &= ~data_msk;
      }

      PHNDisplay8Bit::fillRect(EDIT_ICON_X + (p_x * EDIT_ICON_SCALE), EDIT_ICON_Y + (p_y * EDIT_ICON_SCALE), EDIT_ICON_SCALE, EDIT_ICON_SCALE, draw_color);

      /* Mark cache dirty, notify it was changed */
      volume_cacheDirty = 1;
    }

    /* If button is touched, update touched index (states) */
    newTouchedIndex = EDIT_IDX_NONE;
    icon_idx = 0;
    for (uint8_t x = 0; x < 2; x++) {
     for (uint8_t y = 0; y < 3; y++) {
        unsigned int py = 5 + y * 65 + (y == 2) * 40;
        unsigned int px = 5 + x * (PHNDisplayHW::WIDTH - 58);
        if (LCD_isTouched(px, py, 48, 48)) {
          newTouchedIndex = icon_idx;
          icon_color = COLOR_EDIT_SEL;
        } else {
          icon_color = button_color[icon_idx];
        }
        if (button_dirty[icon_idx]) {
          button_dirty[icon_idx] = 0;

          /* Draw the sketch icon */
          LCD_write_icon(px, py, 48, 48, button_icon[icon_idx], button_title[icon_idx], COLOR_EDIT_BG, icon_color);
        }
        icon_idx++;
      }
    }

    /* Run sketch when the done button is pressed for some time */
    if ((touchedIndex == EDIT_IDX_DONE) && ((millis() - last_done_pressed) > HOLD_ACTIVATE_DELAY)) {
      runWhenExit = true;
      edit_finish = true;
    }

    /* Handle button press-release actions */
    if (!LCD_isTouchedAny()) {
      switch (touchedIndex) {
        case EDIT_IDX_COLOR:
          if (draw_color == COLOR_EDIT_ICON) {
            draw_color = COLOR_EDIT_BG;
            button_icon[EDIT_IDX_COLOR] = edit_icon_changecolor_0;
          } else {
            draw_color = COLOR_EDIT_ICON;
            button_icon[EDIT_IDX_COLOR] = edit_icon_changecolor_1;
          }
          break;

        case EDIT_IDX_RESET:
          /* Resets the icon to the default icon */
          memcpy(volume_cacheBuffer.data, icon_sketch_default, 512);
          volume_cacheDirty = 1;
          needsIconRedraw = true;
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
          PHNDisplay8Bit::writeString(text_off, text_off, 2, "Are you sure you want\n"
                                                             "to permanently delete\n"
                                                             "this sketch?", BLACK_8BIT, WHITE_8BIT);

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
                file_curDir = ski_file;
                file_delete();

                /* Delete HEX file */
                file_curDir = hex_file;
                file_delete();

                /* If asked to load - don't load! */
                runWhenExit = false;

                /* Finish */
                edit_finish = true;
                break;
              }
            }
          }
          needsRedraw = true;
        }
          break;

        case EDIT_IDX_RENAME:
          if (askSketchName(filename)) {
            needsEntryUpdate = true;
          }
          needsRedraw = true;
          break;
            
        case EDIT_IDX_CANCEL:
          /* Restore icon and filename */
          memcpy(volume_cacheBuffer.data, icon_original, 512);
          memcpy(filename, filename_original, 8);
          volume_cacheDirty = 1;
          needsEntryUpdate = true;

          /* Don't load anything */
          runWhenExit = false;
          edit_finish = true;
          break;
          
        case EDIT_IDX_DONE:
          edit_finish = true;
          break;
      }
    }

    /* Redraw icons when touched index changes */
    if (touchedIndex != newTouchedIndex) {
      button_dirty[touchedIndex] = 1;
      button_dirty[newTouchedIndex] = 1;
      last_done_pressed = millis();
      touchedIndex = newTouchedIndex;
    }

    /* Write file entry if needed */
    if (needsEntryUpdate) {
      needsEntryUpdate = false;

      /* Save the SKI file */
      file_curDir = ski_file;
      file_size = ski_file_length;
      file_save(filename);

      /* Save the HEX file */
      file_curDir = hex_file;
      file_size = hex_file_length;
      file_save(filename);
    }

    /* Reset save counter when data changes */
    if (!volume_cacheDirty) {
       iconDirty = false;
    } else if (!iconDirty) {
       iconDirty = true;
       last_save = millis();
    }

    /* Write icon data to SD every 1 second */
    if (iconDirty && (((millis() - last_save) >= EDIT_SAVEINTER) || edit_finish)) {
      volume_writeCache();
    }
  }

  if (runWhenExit) {
    /* Request to load the sketch - do so */
    loadSketchReset(filename);
  } else {
    /* Undo the setting of loading the sketch */
    setLoadOptions(NULL, 0);
  }
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
  PHNDisplay8Bit::writePixelLines(COLOR_NAME_KEY_BG, KEYBOARD_HEIGHT);
  PHNDisplay8Bit::writePixelLines(COLOR_NAME_BG, PHNDisplayHW::HEIGHT - KEYBOARD_HEIGHT);

  // Put a filled square underneath the spacebar
  const uint16_t GREY_SPACE_X = 65;
  const uint16_t GREY_SPACE_H = 31;
  PHNDisplay8Bit::fillRect(GREY_SPACE_X, KEYBOARD_HEIGHT, PHNDisplayHW::WIDTH - GREY_SPACE_X * 2, GREY_SPACE_H, COLOR_NAME_KEY_BG);

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
  const char *popupMessage = NULL;
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
                } else if (file_open(trimmed, "HEX", SDMIN_FILE_READ)) {
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
              if (index_selChar >= 8) {
                index_selChar = 7;
              }
              if (index_selChar && resultFilename[index_selChar] == ' ') {
                memcpy(resultFilename + index_selChar - 1, resultFilename + index_selChar, 8 - index_selChar);
                resultFilename[7] = ' ';
              } else {
                resultFilename[index_selChar] = ' ';
                index_selChar++;
              }
              index_selChar--;
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
          PHNDisplay8Bit::writeImage_1bit(x, y, w, h, 1, edit_icon_cancel, DIR_RIGHT, COLOR_NAME_BG, color);
          PHNDisplay8Bit::writeString(x + 6, y + 50, 1, "Cancel", COLOR_NAME_BG, color);
        } else if (index == KEY_ACCEPT_IDX) {
          // Draw accept button
          uint8_t color = pressed ? WHITE_8BIT : GREEN_8BIT;
          const char* title = isNewSketch ? "Add sketch" : "Rename";
          LCD_write_icon(x, y, w, h, edit_icon_accept, title, COLOR_NAME_BG, color);
        } else {
          // Draw keyboard icons
          uint8_t color = pressed ? COLOR_NAME_KEY_SEL : COLOR_NAME_KEY_FOR;
          PHNDisplay8Bit::fillRect(x, y, w, h, color);
          PHNDisplay8Bit::drawRect(x, y, w, h, COLOR_NAME_KEY_TXT);
          PHNDisplay8Bit::writeChar(x + 4, y + 3, 3, NAME_MAP[index], color, COLOR_NAME_KEY_TXT);
          if (index == KEY_BACKSPACE_IDX) {
            PHNDisplay8Bit::fillRect(x + 10, y + 12, w - 20, 3, COLOR_NAME_KEY_TXT);
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
          color_a = COLOR_NAME_NAM_NOR;
          color_b = COLOR_NAME_NAM_NOR;
        } else {
          color_a = COLOR_NAME_NAM_SEL;
          color_b = COLOR_NAME_BG;
        }
        PHNDisplay8Bit::writeChar(x, y, 4, resultFilename[index], COLOR_NAME_BG, color_a);
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
      PHNDisplay8Bit::writeString(popup_x + txt_xoff, popup_y + 10, 2, popupMessage, WHITE_8BIT, RED_8BIT);
      delay(800);
      PHNDisplay8Bit::fillRect(popup_x, popup_y, popup_w, popup_h, COLOR_NAME_KEY_BG);
      redrawAll = true;
      popupMessage = NULL;
    }
  }
}

/* Updates the Micro-SD volume, notifying the user if this fails */
void updateVolume() {
  /* Force-initialize the first time */
  volume.isInitialized = 0;

  /* Display a message to the user that no SD-card is inserted */
  for (;;) {
    volume_init(0);
    if (volume.isInitialized) {
      return;
    }

    PHNDisplay8Bit::fill(COLOR_MENU_BG);
    PHNDisplay8Bit::writeString(30, 30, 2, "Micro-SD card was not\n"
                                           "detected or is corrupt", BLACK_8BIT, RED_8BIT);
    uint8_t is_touched = 0;
    uint8_t is_dirty = 1;
    for (;;) {
      LCD_updateTouch();

      /* Run until screen is pressed and released */
      if (is_touched != LCD_isTouchedAny()) {
        is_touched = !is_touched;
        is_dirty = 1;
        if (!is_touched) break;
      }

      if (is_dirty) {
        is_dirty = 0;
        uint8_t color = is_touched ? WHITE_8BIT : GREEN_8BIT;
        LCD_write_icon(136, 96, 48, 48, edit_icon_reset, "Refresh", color);
      }
    }
  }
}

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

/*
 * Used to draw an icon with text underneath it in a portable, re-usable manner.
 * x/y/w/h specifies the location and size of the icon drawn
 * the icon data specifies the icon data used for the icon drawn
 * the title specifies the title text drawn underneath the icon
 * colorA and colorB specify the icon colors - 0/1
 * colorC specifies the foreground color of the text
 * A black background is assumed.
 */
void LCD_write_icon(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* icon, const char* title, uint8_t color0, uint8_t color1) {
  PHNDisplay8Bit::writeImage_1bit(x, y, w, h, 1, icon, DIR_RIGHT, color0, color1);
  PHNDisplay8Bit::writeString(x + (w - strlen(title) * 6) / 2, y + h + 2, 1, title, color0, color1);
}

/*
 * Overload to make the icon drawing a little simpler to call, with the same color used for title and icon
 */
void LCD_write_icon(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint8_t* icon, const char* title, uint8_t color0) {
  LCD_write_icon(x, y, w, h, icon, title, BLACK_8BIT, color0);
}

void LCD_updateTouch(void) {
  float f_pressure;

  /* Store old x/y touched point for later */
  uint16_t old_x = touch_x;
  uint16_t old_y = touch_y;

  /* Read the touch input */
  PHNDisplayHW::readTouch(&touch_x, &touch_y, &f_pressure);

  /* Not touched */
  boolean isNotTouched = ((old_x == 0xFFFF && f_pressure <= 70.0F) || f_pressure == 0.0F);
  if (isNotTouched || touch_waitup) {
    /* Not touched or waiting for touch to engage */
    if (isNotTouched) {
      touch_waitup = false;
    }
    touch_x = touch_y = 0xFFFF;
  } else {
    /* Apply a smoothing factor to make drawing easier */
    if (old_x != 0xFFFF) {
      touch_x += 0.80 * ((int) old_x - (int) touch_x);
      touch_y += 0.80 * ((int) old_y - (int) touch_y);
    }
  }
}

void LCD_clearTouch(void) {
  touch_x = touch_y = 0xFFFF;
  touch_waitup = true;
}

uint8_t LCD_isTouchedAny(void) {
  return touch_x != 0xFFFF && touch_y != 0xFFFF;
}

uint8_t LCD_isTouched(uint16_t x, uint16_t y, uint8_t w, uint8_t h) {
  return touch_x >= x && touch_y >= y && touch_x < (x + w) && touch_y < (y + h);
}