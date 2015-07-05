/* Live clock which allows you to adjust the current time */

// Value ranges in the date
unsigned char timeMinimums[6] = {0, 1, 1, 0, 0, 0};
unsigned char timeMaximums[6] = {99, 12, 31, 23, 59, 59};

// Constants for the clock display
const int pointerLengths[3] = {30, 45, 50};
const color_t pointerColors[3] = {BLACK, BLACK, RED};
const int clock_center_x = 160;
const int clock_center_y = 160;
const int clock_radius_inner = 60;
const int clock_radius = 75;

void showClock() {
  header.setNavigation("Cancel", "Accept");

  // Get current Date
  Date date = sim.getDate();

  // Labels displayed next to each number box
  const char* timeLabelText[6] = {"  Year:", " Month:", "   Day:", "  Hour:", "Minute:", "Second:"};

  PHN_NumberBox timeBoxes[6];
  PHN_Label timeLabels[6];
  for (int i = 0; i < 6; i++) {
    const int BOX_W = 56;
    const int BOX_H = 25;
    const int LBL_W = 47;
    int x = 2 + ((i%3) * (BOX_W+LBL_W));
    int y = 18;
    if (i >= 3) y += 10 + BOX_H;

    // Setup the widgets
    timeBoxes[i].setBounds(x+LBL_W, y, BOX_W, BOX_H);
    timeBoxes[i].setRange(timeMinimums[i], timeMaximums[i]);
    timeBoxes[i].setValue(date[i]);
    timeBoxes[i].setWrapAround(true);
    display.addWidget(timeBoxes[i]);

    // Display text labels next to the date/time
    timeLabels[i].setBounds(x, y, LBL_W, BOX_H);
    timeLabels[i].setText(timeLabelText[i]);
    display.addWidget(timeLabels[i]);
  }

  // First update to draw and initialize
  display.update();

  // Draw the clock frame
  drawClockFrame();

  unsigned long lastSecond = millis();
  Date lastDate = date;
  boolean wasDateChanged = false;
  do {
    // Update display
    display.update();

    // If the user changed any boxes, pause the automatic counting
    for (int i = 0; i < 6; i++) {
      if (timeBoxes[i].isValueChanged()) {
        lastSecond = millis() + 3000;
      }
    }

    // Increment seconds box every second
    if (millis() >= lastSecond) {
      lastSecond += 1000;
      timeBoxes[5].addValue(1);
    }

    // Automatically increment each box as the other is wrapped
    for (int i = 5; i >= 1; i--) {
      timeBoxes[i-1].addValue(timeBoxes[i].wrapAroundIncrement());
    }

    // Update the date with changes from the user interface
    // While any of the boxes are being changed, pause the time counter
    boolean timeChanged = false;
    for (int i = 0; i < 6; i++) {
      if (timeBoxes[i].isValueChanged()) {
        date[i] = timeBoxes[i].value();
        timeChanged = true;
        wasDateChanged = true;
      }
    }
    if (timeChanged) {
      timeChanged = false;
      drawClock(lastDate, date);
      lastDate = date;
    }
  } while (!header.isNavigating());

  // Remove all previously added widgets
  for (int i = 0; i < 6; i++) {
    display.removeWidget(timeBoxes[i]);
    display.removeWidget(timeLabels[i]);
  }
  
  // Write the new Date to the SIM if changed and accepted
  if (header.isAccepted() && wasDateChanged) {
    sim.setDate(date);
  }
}

void drawClockFrame() {
  const color_t BORDER = GRAY_LIGHT;
  display.fillCircle(clock_center_x, clock_center_y, clock_radius, BORDER);
  display.fillCircle(clock_center_x, clock_center_y, clock_radius_inner, WHITE);
  display.drawCircle(clock_center_x, clock_center_y, clock_radius_inner, BLACK);

  // Draw the clock 'ticks'
  for (float a = 0.0F; a < 1.9*PI; a += PI/6.0) {
    display.drawLineAngle(clock_center_x, clock_center_y, clock_radius_inner, clock_radius_inner-8, a, BLACK);
  }

  // Draw time labels in 4 places
  display.setTextColor(BLACK, BORDER);
  display.drawString(clock_center_x+clock_radius-10, clock_center_y-3, "3", 1);
  display.drawString(clock_center_x-2, clock_center_y+clock_radius-11, "6", 1);
  display.drawString(clock_center_x-clock_radius+5, clock_center_y-3, "9", 1);
  display.drawString(clock_center_x-6, clock_center_y-clock_radius+4, "12", 1);
}

void drawClock(Date oldDate, Date newDate) {
  for (int i = 3; i < 6; i++) {
    // Erase the previous pointer when the pointer changed position
    if (oldDate[i] != newDate[i] || (i==3 && oldDate[4] != newDate[4])) {
      drawClockPointer(oldDate, i, true);
    }
  }
  // Draw all pointers in the right order
  for (int i = 3; i < 6; i++) {
    drawClockPointer(newDate, i, false);
  }
}

void drawClockPointer(Date date, int dateIndex, boolean erase) {
  int pointerLength = pointerLengths[dateIndex-3];
  color_t pointerColor = erase ? WHITE : pointerColors[dateIndex-3];
  float f = getClockFact(date, dateIndex);

  // For the hour display, the angle is doubled (24hr -> 12hr)
  // As well, we add a fraction of the minute value
  if (dateIndex == 3) f = (f * 2) + getClockFact(date, 4)/12.0;

  // Draw pointer at an angle 0 - 1
  float a = f * 2.0 * PI - 0.5 * PI;
  display.drawLineAngle(clock_center_x, clock_center_y, 0, pointerLength, a, pointerColor);
}

float getClockFact(Date date, int dateIndex) {
  return (float) (date[dateIndex]-timeMinimums[dateIndex]) / (float) (timeMaximums[dateIndex]-timeMinimums[dateIndex]+1);
}
