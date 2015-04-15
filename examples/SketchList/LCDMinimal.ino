#include <util/delay.h>
#include <Arduino.h>
#include "font.c"
#include "Phoenard.h"

#define TFTLCD_TOUCH_DEBOUNCE 16

uint16_t touch_x, touch_y;
uint8_t touch_downctr;
uint8_t touch_waitup;
uint16_t touch_hor_a, touch_hor_b, touch_ver_a, touch_ver_b;

void LCD_write_font(uint16_t x, uint16_t y, uint8_t scale, unsigned char character, uint8_t color0, uint8_t color1) {
  character -= 32;
  PHNDisplay8Bit::writeImage_1bit(x, y, 8, 5, scale, font + (character * 5), DIR_DOWN, color0, color1);
}

void LCD_write_string(uint16_t x, uint16_t y, uint8_t scale, const char* text, uint8_t color0, uint8_t color1) {
  while (*text) {
    LCD_write_font(x, y, scale, *text, color0, color1);
    x += 6 * scale;
    text++;
  }
}

void LCD_write_icon(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* icon, const char* title, uint8_t colorA) {
  LCD_write_icon(x, y, w, h, icon, title, BLACK_8BIT, colorA, colorA);
}

void LCD_write_icon(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint8_t* icon, const char* title, uint8_t colorA, uint8_t colorB, uint8_t colorC) {
  PHNDisplay8Bit::writeImage_1bit(x, y, w, h, 1, icon, DIR_RIGHT, colorA, colorB);
  LCD_write_string(x + (w - strlen(title) * 6) / 2, y + h + 2, 1, title, BLACK_8BIT, colorC);
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
    if (!isNotTouched) {
      touch_waitup = 0;
    }
    touch_x = touch_y = 0xFFFF;
  } else {
    /* Touched */
    touch_downctr = TFTLCD_TOUCH_DEBOUNCE;

    /* Apply a smoothing factor to make drawing easier */
    if (old_x != 0xFFFF) {
      touch_x += 0.80 * ((int) old_x - (int) touch_x);
      touch_y += 0.80 * ((int) old_y - (int) touch_y);
    }
  }
}

void LCD_clearTouch(void) {
  touch_x = touch_y = 0xFFFF;
  touch_waitup = 1;
}

uint8_t LCD_isTouchedAny(void) {
  return touch_x != 0xFFFF && touch_y != 0xFFFF;
}

uint8_t LCD_isTouched(uint16_t x, uint16_t y, uint8_t w, uint8_t h) {
  return touch_x >= x && touch_y >= y && touch_x < (x + w) && touch_y < (y + h);
}
