#include <avr/sleep.h>
#include <avr/power.h>

const unsigned long SLEEP_TIME = 20000;
unsigned long sleep_start;

/* Resets the sleep timer, waiting to sleep again */
void resetSleep() {
  sleep_start = millis();
}

/* Checks whether sufficient time has passed, and sleeps after some time */
void updateSleep() {
  if ((millis() - sleep_start) >= SLEEP_TIME) {
    sleepNow();
  }
}

/* Puts the device into sleep mode, waking up when an interrupt fires resuming code execution */
void sleepNow() {
  // First dim the display for some time, regularly checking if the user does something
  // When the user doesn't actually want it to sleep, this allows the user to quickly gain back control
  for (int i = 256; i >= 0; i--) {
    delay(2);
    display.setBacklight(i);
    display.update();
    if (display.isTouched() || isSelectPressed()) {
      display.setBacklight(256);
      resetSleep();
      return;
    }
  }

  // Put peripherals to sleep here
  display.setSleeping(true);

  // Setup pin 1 as an interrupt and attach handler.
  attachInterrupt(1,reinterpret_cast<void (*)()>(&disInt), LOW);

  // Put device to sleep and wait until an interrupt happens
  set_sleep_mode(SLEEP_MODE_PWR_DOWN);
  sleep_enable();
  sleep_mode();

  // First thing to do is disable sleep.
  sleep_disable();
  detachInterrupt(1);

  // Turn peripherals on again
  display.setSleeping(false);

  // Slowly turn the screen back on - contents were preserved during sleep
  for (int i = 0; i <= 256; i++) {
    display.setBacklight(i);
    delay(2);
  }

  // Reset sleep mode
  resetSleep();
}

void disInt(){
  /* This will bring us back from sleep. */
  
  /* We detach the interrupt to stop it from 
   * continuously firing while the interrupt pin
   * is low.
   */
  detachInterrupt(1);
}
