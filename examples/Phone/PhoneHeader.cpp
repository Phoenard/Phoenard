#include "PhoneHeader.h"

PHN_PhoneHeader::PHN_PhoneHeader() {
  setBounds(0, 0, 320,  240);
  refreshInfo();
  updateIdx = 0;
  cardInserted = true;
  lastSync = 0;
  lastUpdate = 0;
  bigHeader = false;
  enableUpdates = true;
  
  // Setup and add cancel/accept widgets
  cancelBtn.setBounds(5, 203, 55, 32);
  cancelBtn.setVisible(false);
  cancelBtn.setColor(FOREGROUND, RED);
  cancelBtn.setColor(FRAME, RED);
  cancelBtn.setColor(CONTENT, BLACK);
  addWidget(cancelBtn);
  
  acceptBtn.setBounds(260, 203, 55, 32);
  acceptBtn.setVisible(false);
  acceptBtn.setColor(FOREGROUND, GREEN);
  acceptBtn.setColor(FRAME, GREEN);
  cancelBtn.setColor(CONTENT, BLACK);
  addWidget(acceptBtn);
}

void PHN_PhoneHeader::setUpdating(boolean updating) {
  enableUpdates = updating;
}

void PHN_PhoneHeader::setBigHeader(boolean big) {
  if (bigHeader != big) {
    bigHeader = big;
    invalidate();
  }
}

void PHN_PhoneHeader::showStatus(const char* statusText) {
  const int stat_x = 76;
  const int stat_y = 206;
  const int stat_w = 168;
  const int stat_h = 24;
  
  display.fillBorderRect(stat_x, stat_y, stat_w, stat_h, BLACK, WHITE);
  display.setTextColor(YELLOW);
  display.drawStringMiddle(stat_x, stat_y, stat_w, stat_h, statusText);
  delay(1500);
  display.fillRect(stat_x, stat_y, stat_w, stat_h, BLACK);
}

void PHN_PhoneHeader::setNavigation(const char* prevText, const char* nextText) {
  cancelBtn.setVisible(prevText != NULL);
  acceptBtn.setVisible(nextText != NULL);
  
  if (prevText) {
    cancelBtn.setText(prevText);
    //cancelBtn.setImage(PHN_Image(header_nav_button_draw, prevText));
  }
  if (nextText) {
    acceptBtn.setText(nextText);
    //acceptBtn.setImage(PHN_Image(header_nav_button_draw, nextText));
  }
}

void PHN_PhoneHeader::refreshInfo() {
  lastSync = millis() - PHONE_HEADER_SYNC_INTERVAL;
}

boolean PHN_PhoneHeader::isNavigating() {
  return cancelBtn.isClicked() || acceptBtn.isClicked();
}

boolean PHN_PhoneHeader::isAccepted() {
  return acceptBtn.isClicked();
}

boolean PHN_PhoneHeader::isCancelled() {
  return cancelBtn.isClicked();
}

void PHN_PhoneHeader::update() {
  if (!enableUpdates) return;

  // Sync width/height with display
  width = display.width();
  height = display.height();

  // Routinely read data from the sim
  float battery_new = battery;
  int signal_new = signal;
  int regStatus_new = regStatus;
  
  if ((millis() - lastUpdate) >= PHONE_HEADER_UPDATE_INTERVAL) {
    lastUpdate = millis();
    
    if (sim.isOn()) {
      switch (updateIdx) {
        case 0:
          battery_new = sim.readBatteryLevel();
        case 1:
          signal_new = sim.readSignalLevel();
        case 2:
          if (cardInserted) {
            regStatus_new = sim.getRegStatus();
          } else {
            regStatus_new = 7;
          }
      }
      if (++updateIdx >= 3) updateIdx = 0;
    } else {
      battery_new = 1.0F;
      signal_new = 0;
      regStatus_new = 6;
    }
  }

  boolean changed = (battery_new != battery) || 
                    (signal_new != signal) ||
                    (regStatus_new != regStatus);

  // Update data (check first whether it is in sleep mode or not)
  if (changed || (millis() - lastSync) >= PHONE_HEADER_SYNC_INTERVAL) {
    lastSync = millis();
    battery = battery_new;
    signal = signal_new;
    regStatus = regStatus_new;
    cardInserted = sim.isSimCardInserted();

    // Registration codes according to datasheet AT-Commands SIM908 page 77
    // Code 6 and 7 are added by us for other states
    switch(regStatus) {
      case 0: strcpy(provider, "No Provider"); break;
      case 1: sim.readProvider(provider, 30); break;
      case 2: strcpy(provider, "Registering..."); break;
      case 3: strcpy(provider, "Unable to register"); break;
      case 4: strcpy(provider, "Unknown"); break;
      case 5: sim.readProvider(provider,30); break;
      case 6: strcpy(provider, "Offline"); break;
      case 7: strcpy(provider, "No Sim Card"); break;
    }

    // Read date if possible
    if (sim.isOn()) {
      date = sim.getDate();
    }

    // Redraw the header
    if (!invalidated) {
      drawHeader();
      drawSidebars();
    }
  }
}

int PHN_PhoneHeader::getSignalLines(int cnt) {
  if (!signal || (signal < -110)) return 0;
  return map(signal, -110, -54, 1, cnt);
}

void PHN_PhoneHeader::draw() {
  // Fill the screen with a bordered frame
  display.fillBorderRect(0, 0, width, height, color(BACKGROUND), color(FRAME));

  // Draw the top header
  drawHeader();

  // For the main menu big header, also draw a nice background and then the sidebars over it
  if (bigHeader) {
    drawBackground();
    drawSidebars();
  }
}

void PHN_PhoneHeader::drawBackground(){
  // Set the viewport to the area to be filled in with colors
  display.setViewport(1, PHONE_HEADER_BIG_HEIGHT+1, width-2, height-PHONE_HEADER_BIG_HEIGHT-2);

  // We could draw an image here or something else, but this will do.
  // Feel free to implement anything you like here, the viewport will keep you safe :)
  PHNDisplay16Bit::colorTest();

  // Reset viewport back to fullscreen
  display.resetViewport();
}

void PHN_PhoneHeader::drawHeader() {
  //check if it is on mainmenu or not, then draw if yes, otherwise draw simple header
  if(bigHeader){
    // Fill background of header
    display.fillRect(1, 1, width - 2, PHONE_HEADER_BIG_HEIGHT, color(BACKGROUND));

    // Draw labeling
    display.setTextSize(1);
    display.setCursor(4, 3);
    display.setTextColor(WHITE);
    display.print("Net Sig");
    display.setCursor(display.width()-44,3);
    display.print("Bat Lev");
   
    display.setTextColor(color(FOREGROUND));
    display.setTextSize(1);
    uint8_t bat=battery*100;
    if (bat==100){
      display.setCursor(width-27, 15);
    } else {
      display.setCursor(width-21, 15);
    }
    display.setTextColor(GREEN);
    display.print(bat);
    display.print("%");
    display.setCursor(4,15);
    if (signal) {
      display.print(signal);
      display.print("dB");
    } else {
      display.print("No Signal");
    }

    // Draw provider
    display.setTextColor(RED);
    display.setCursor(display.width()/2-((strlen(provider)*5)/2), 18);
    display.print(provider);

    // Draw date
    display.setCursor(65, 10);
    display.setTextColor(CYAN);
    display.printDate(date);

    // Draw time
    display.setTextSize(2);
    display.setCursor(width/2-29, 3);
    display.printShortTime(date);
  } else {
    // Wipe old header
    display.fillRect(1, 1, width - 2, 10, color(BACKGROUND));

    // Draw top border frame
    display.drawHorizontalLine(1, 10, width-2, color(FRAME));  

    // Draw battery indicator
    int bat_width = (int) (battery * 12.0);
    display.drawRect(width-18, 2, 16, 7, color(FOREGROUND));
    display.drawVerticalLine(width-19, 4, 3, color(FOREGROUND));
    display.fillRect(width-16+(12-bat_width), 4, bat_width, 3, color(FOREGROUND));

    // Draw signal indicator icon
    display.drawVerticalLine(6/2+2, 6, 3, color(FOREGROUND));
    display.drawTriangle(2, 2, 6+2, 2, 6/2+2, 5, color(FOREGROUND));
    // Draw signal indicator lines, or X when no signal
    if (signal) {
      int signal_lines = getSignalLines(5);
      for (int i = 0; i < signal_lines; i++) {
        display.drawStraightLine(6/2+2+2 + i*2, 8, 2+i, 3, color(FOREGROUND));
      }
    } else {
      display.drawLine(9, 4, 13, 8, color(FOREGROUND));
      display.drawLine(13, 4, 9, 8, color(FOREGROUND));
    }

    // Draw time
    display.setTextSize(1);
    display.setTextColor(WHITE);
    display.setCursor(width/2-14, 3);
    display.printShortTime(date);
  }
}

void PHN_PhoneHeader::drawSidebars() {
  // Draw big signal and battery indicators for the big main menu header
  if (bigHeader) {
    int signal_lines = getSignalLines(7);
    int battery_lines = (int) (7.0F * battery);
    for (int i=0;i<7;i++) {
      color_t sig_color = (i < signal_lines) ? RED : BLACK;
      color_t bat_color = (i < battery_lines) ? RED : BLACK;
      display.fillBorderRect(4,display.height()-(32+i*30),i+6,27,sig_color,WHITE);
      display.fillBorderRect(display.width()-(4+i+6),display.height()-(32+i*30),i+6,27,bat_color,WHITE);
    }
  }
}
