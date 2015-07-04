/*
 * A LED chaser controller with speed and animation controlled using on-screen widgets
 */
#include "Phoenard.h"

// Runtime variables
const int mode_cnt = 5;
const int led_count = 6;
const int led_pins[led_count] = {A0,A1,A2,A3,A4,A5};
boolean   led_output_set[led_count];
boolean   led_output[led_count];

unsigned long chaser_delay = 150;
int chaser_width = 1;
int animation_mode = 0;

// Widgets
PHN_Scrollbar speedBar;
PHN_Scrollbar widthBar;
PHN_Button    modeButtons[mode_cnt];

void setup() {
  // Show title and UI border lines for fancy
  display.setTextColor(GREEN);
  display.drawStringMiddle(10, 10, 300, 50, "LED Chaser");
  display.drawHorizontalLine(0, 70, 320, GREEN);
  display.drawHorizontalLine(0, 170, 320, GREEN);

  // Initialize widgets
  // Bar to control the speed of the animation
  speedBar.setBounds(80, 80, 230, 20);
  speedBar.setRange(1, 30);
  speedBar.setValue(10);
  display.addWidget(speedBar);
  display.debugPrint(10, speedBar.getY(), 2, "Speed:");

  // Bar to change the width of the scrolling animation
  widthBar.setBounds(80, 110, 230, 20);
  widthBar.setRange(1, led_count);
  widthBar.setValue(1);
  display.addWidget(widthBar);
  display.debugPrint(10, widthBar.getY(), 2, "Size:");

  // Animation mode buttons
  for (int i = 0; i < mode_cnt; i++) {
    // Generate text for the button
    char txt[4];
    itoa(i+1, txt, 10);
    
    // Setup the button
    modeButtons[i].setBounds(80 + i * 30, 140, 25, 20);
    modeButtons[i].setText(txt);
    display.addWidget(modeButtons[i]);
  }
  display.debugPrint(10, modeButtons[0].getY(), 2, "Mode:");

  // Initialize LEDs
  for (int i = 0; i < led_count; i++) {
    pinMode(led_pins[i], OUTPUT);
    updateLed(i, false, true);
  }

  // Set default animation
  setAnimationMode(0);
}

void loop() {
  // Execute animation runs in the main loop
  switch (animation_mode) {
    case 0:
      {
        // Mode 0: Chaser effect going left/right repeatedly
        for (int i = -chaser_width; i <= led_count; i++) {
          setAllLeds(false);
          setLedRange(i, chaser_width, true);
          updateAnimation();
        }
        for (int i = led_count; i >= -chaser_width; i--) {
          setAllLeds(false);
          setLedRange(i, chaser_width, true);
          updateAnimation();
        }
      }
      break;
      
    case 1:
      {
        // Mode 1: Chaser effect going outer-middle repeatedly
        for (int i = -chaser_width; i < led_count/2; i++) {
          setAllLeds(false);
          setLedRange(i, chaser_width, true);
          setLedRange(led_count-i-chaser_width, chaser_width, true);
          updateAnimation();
        }
        for (int i = led_count/2-1; i >= -chaser_width; i--) {
          setAllLeds(false);
          setLedRange(i, chaser_width, true);
          setLedRange(led_count-i-chaser_width, chaser_width, true);
          updateAnimation();
        }
      }
      break;

    case 2:
      {
        // Mode 2: Simple left-right scrolling
        for (int i = -chaser_width; i < led_count; i++) {
          setAllLeds(false);
          setLedRange(i, chaser_width, true);
          updateAnimation();
        }
      }
      break;
      
    case 3:
      {
        // Mode 3: Simple right-left scrolling
        for (int i = led_count-1; i >= -chaser_width; i--) {
          setAllLeds(false);
          setLedRange(i, chaser_width, true);
          updateAnimation();
        }
      }
      break;
      
    case 4:
      {
        // Mode 4: Blinking
        setAllLeds(true);
        updateAnimation();
        setAllLeds(false);
        updateAnimation();
      }
      break;
  }
}

/* Sets all LEDs from at an index to a given state */
void setLedRange(int index, int count, boolean on) {
  for (int i = index; i < (index+count); i++) {
    if (i >= 0 && i < led_count) {
      led_output[i] = on;
    }
  }
}

/* Sets all LEDs to a given state */
void setAllLeds(boolean on) {
  for (int i = 0; i < led_count; i++) {
    led_output[i] = on;
  }
}

/* Changes the animation mode */
void setAnimationMode(int mode) {
  modeButtons[animation_mode].setColor(FOREGROUND, WHITE);
  animation_mode = mode;
  modeButtons[animation_mode].setColor(FOREGROUND, GREEN);
}

/* Updates the LED state and refreshes UI */
void updateAnimation() {
  // Update LED pins and perform UI for the entire animation delay
  unsigned long startTime = millis();
  for (int i = 0; i < led_count; i++) {
    boolean on = led_output[i];
    if (led_output_set[i] != on) {
      updateLed(i, on, false);
    }
  }
  do {
    // Update the widgets
    display.update();

    // Update animation settings
    chaser_delay = 1000 / speedBar.value();
    chaser_width = widthBar.value();

    // Handle mode button change clicks
    for (int i = 0; i < mode_cnt; i++) {
      if (modeButtons[i].isClicked()) {
        setAnimationMode(i);        
      }
    }
  } while ((millis() - startTime) < chaser_delay);
}

/* Updates the state of a single LED */
void updateLed(int i, boolean on, boolean drawFrame) {
  // Updates a LED digital pin state
  led_output_set[i] = on;
  digitalWrite(led_pins[i], on);
  
  // Update LED state on the screen
  const int offset = 20;
  int x = offset + i * ((320-2*offset) / (led_count-1));
  int y = 240-offset;
  if (drawFrame) {
    display.drawCircle(x, y, 6, WHITE);
  }
  display.fillCircle(x, y, 5, on ? RED : BLACK);
}
