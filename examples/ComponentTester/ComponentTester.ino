/*
 * The Components Tester can test the values of common electronic components.
 *
 * This is a fairly complex program that can measure the values of resistors,
 * diodes, capacitors and transistors. This is done by controlling three
 * resistors; 1K, 10K, 100K; to measure the unknown electric properties.
 *
 * Actual measurement is done in the Measurement.h/cpp files. The initiation
 * and display of the data is done inside this main sketch.
 *
 * By default the following design schematic is used. Build this design on
 * a breadboard and plug it into the Phoenard. If something is wrongly
 * connected, the program will prompt this in your Serial Monitor.
 *
 * A total of 3 ADC pins and 9 GPIO pins are used in the design. You will need
 * a total of 3 1K, 3 10K and 3 100K resistors. The higher precision resistors,
 * the better!
 *
 * The a/b/c below denote the test points to plug components in.
 *
 * [a]------+-----------[pin A3]
 *          |--[ 1k  ]--[pin 4]
 *          |--[ 10k ]--[pin 5]
 *          \--[ 100k]--[pin 7]
 * 
 * [b]------+-----------[pin A4]
 *          |--[ 1k  ]--[pin A2]
 *          |--[ 10k ]--[pin 9]
 *          \--[ 100k]--[pin 6]
 *
 * [c]------+-----------[pin A5]
 *          |--[ 1k  ]--[pin A1]
 *          |--[ 10k ]--[pin 51]
 *          \--[ 100k]--[pin 52]
 */
#include "Phoenard.h"
#include "Measurement.h"
#include "SPI.h"

/* Probe pin constants: {ADC, 1K, 10K, 100K} */
Pins probe_a = {A3, 4, 5, 7};
Pins probe_b = {A4, A2, 9, 6};
Pins probe_c = {A5, A1, 51, 52};

// Color constants
#define BACKCOLOR   BLACK
#define BACKCOLOR2  GRAY_DARK
#define TEXTCOLOR   WHITE
#define FRAMECOLOR  RED

// Size of probe info box
#define INFO_WIDTH   70
#define INFO_HEIGHT  32
// A-C info box
#define INFO_AC_X    (INFO_AB_X + 40)
#define INFO_AC_Y    5
// A-B info box
#define INFO_AB_X    10
#define INFO_AB_Y    (INFO_AC_Y + INFO_HEIGHT + 5)
// B-C info box
#define INFO_BC_X    (INFO_AB_X + 75)
#define INFO_BC_Y    (INFO_AC_Y + INFO_HEIGHT + 5)
// Middle line
#define INFO_MN_X    (INFO_AB_X + INFO_WIDTH + (INFO_BC_X - (INFO_AB_X + INFO_WIDTH)) / 2)
// Left line
#define INFO_LN_X    (INFO_AB_X - 5)
#define INFO_LN_X2   (INFO_MN_X - 30)
// Right line
#define INFO_RN_X    (INFO_BC_X + INFO_WIDTH + 5)
#define INFO_RN_X2   (INFO_MN_X + 30)
// Probe line Y-values
#define INFO_PR_Y1   (INFO_AB_Y + INFO_HEIGHT + 5)
#define INFO_PR_Y2   (INFO_PR_Y1 + 130)
#define INFO_GR_X    INFO_RN_X + 20
#define INFO_GR_Y    INFO_AC_Y
#define INFO_GR_W    MAX_DATA_LENGTH + 3
#define INFO_GR_H    80
#define INFO_GR2_Y   INFO_GR_Y + INFO_GR_H +  5
// Probe box dimensions
#define INFO_PB_HS   9 // Hole size in probe box
#define INFO_PB_SP   5 // Hole spacing
#define INFO_PB_W    ((INFO_RN_X2 - INFO_LN_X2) + 2 * (INFO_PB_SP + (INFO_PB_HS / 2)))
#define INFO_PB_H    (INFO_PB_HS + 2 * INFO_PB_SP)
#define INFO_PB_HY   (INFO_PR_Y2 + (INFO_PB_H / 2) - (INFO_PB_HS / 2))  // Hole height offset
// Start button dimensions
#define START_TEXT   "Start"
#define START_W      INFO_GR_W
#define START_H      56
#define START_X      INFO_GR_X
#define START_Y      (INFO_GR2_Y + INFO_GR_H + 5)
// Transistor box
#define TRANS_W      (INFO_RN_X2 - INFO_LN_X2 + 60)
#define TRANS_H      70
#define TRANS_X      (INFO_MN_X - TRANS_W / 2)
#define TRANS_Y      (INFO_PR_Y1 + 20)
#define TRANS_PY     (TRANS_Y + TRANS_H - 13) // Probe circle Y-coordinate

/* Special characters stored in FLASH */
const uint8_t ohmega_char[] PROGMEM = {92, 102, 2, 102, 92};
const uint8_t ldiode_char[] PROGMEM = {127, 8, 20, 34, 127};
const uint8_t rdiode_char[] PROGMEM = {127, 34, 20, 8, 127};
const uint8_t resistor_char[] PROGMEM = {114, 41, 82, 4, 2};
const uint8_t capacitance_char[] PROGMEM = {8, 62, 0, 62, 8};

/* Whether the big info box is shown */
boolean info_box_shown = false;

/* Widgets */
PHN_LineGraph graph_a;
PHN_LineGraph graph_b;
PHN_Button start_btn;

void setup() {
  // Start Serial at 9600 BAUD
  Serial.begin(9600);
  
  // We make use of some SPI-controlled pins
  // It is required to disable SPI first to make sure it all works
  SPI.end();

  /*
   * Perform a check to see if the test board is connected
   * This helps identify problems in your multimeter PCB design
   * See top header for information about the circuit design.
   */
  for (int i = 0; i < 3; i++) {
    // Set it all to float
    setPin(probes[i].res_1k, FLOATING);
    setPin(probes[i].res_10k, FLOATING);
    setPin(probes[i].res_100k, FLOATING);
    
    // Set ADC pin to pullup HIGH
    pinMode(probes[i].adc, INPUT);
    digitalWrite(probes[i].adc, HIGH);
    delay(1);
    
    // Verify pin is HIGH
    int adc;
    adc = analogRead(probes[i].adc);
    const int ADC_RANGE = 5;
    const int ADC_1K    = 43;
    const int ADC_10K   = 233;
    const int ADC_100K  = 755;
    if (adc < 1020) {
      Serial.print("PROBE ");
      Serial.print(i);
      Serial.println(" HAS LOW-SHORTED ADC");
    } else {
      // Pull pin low one-by-one
      setPin(probes[i].res_1k, LOW);
      delay(1);
      adc = analogRead(probes[i].adc);
      if (adc < (ADC_1K - ADC_RANGE) || adc > (ADC_1K + ADC_RANGE)) {
        Serial.print("PROBE ");
        Serial.print(i);
        Serial.print(" RESISTOR 1K ERROR: EXPECTED ADC ");
        Serial.print(ADC_1K);
        Serial.print(" BUT GOT ");
        Serial.println(adc);
      }
      setPin(probes[i].res_1k, FLOATING);
      
      setPin(probes[i].res_10k, LOW);
      delay(1);
      adc = analogRead(probes[i].adc);
      if (adc < (ADC_10K - ADC_RANGE) || adc > (ADC_10K + ADC_RANGE)) {
        Serial.print("PROBE ");
        Serial.print(i);
        Serial.print(" RESISTOR 10K ERROR: EXPECTED ADC ");
        Serial.print(ADC_10K);
        Serial.print(" BUT GOT ");
        Serial.println(adc);
      }
      setPin(probes[i].res_10k, FLOATING);
      
      setPin(probes[i].res_100k, LOW);
      delay(1);
      adc = analogRead(probes[i].adc);
      if (adc < (ADC_100K - ADC_RANGE) || adc > (ADC_100K + ADC_RANGE)) {
        Serial.print("PROBE ");
        Serial.print(i);
        Serial.print(" RESISTOR 100K ERROR: EXPECTED ADC ");
        Serial.print(ADC_100K);
        Serial.print(" BUT GOT ");
        Serial.println(adc);
      }
      setPin(probes[i].res_100k, FLOATING);
    }
    
    // Undo ADC
    digitalWrite(probes[i].adc, LOW);
  }

  // Setup start button
  start_btn.setBounds(START_X, START_Y, START_W, START_H);
  start_btn.setText("Start");
  display.addWidget(start_btn);

  // Setup first graph
  graph_a.setBounds(INFO_GR_X, INFO_GR_Y, INFO_GR_W, INFO_GR_H);
  graph_a.setRange(0.0F, 1023.0F);
  graph_a.setLineCount(1);
  graph_a.setLineColor(0, WHITE);
  display.addWidget(graph_a);

  // Setup second graph
  graph_b.setBounds(INFO_GR_X, INFO_GR2_Y, INFO_GR_W, INFO_GR_H);
  graph_b.setRange(0.0F, 1023.0F);
  graph_b.setLineCount(1);
  graph_b.setLineColor(0, WHITE);
  display.addWidget(graph_b);

  // Update the display to draw all widgets
  display.update();

  // Draw the colored wire diagram
  drawWireDiagram();

  Serial.println("BEGIN");
}

void loop() {
  // Draw info boxes frames
  drawInfoFrame(INFO_AB_X, INFO_AB_Y);
  drawInfoFrame(INFO_BC_X, INFO_BC_Y);
  drawInfoFrame(INFO_AC_X, INFO_AC_Y);

  // Measure data and display results between probe a and c
  ADCData data_ac = measureData(probe_a, probe_c);
  ADCData data_ca = measureData(probe_c, probe_a);
  boolean ac_info = displayData(data_ac, data_ca, INFO_AC_X, INFO_AC_Y);

  // Measure data and display results between probe a and b
  ADCData data_ab = measureData(probe_a, probe_b);
  ADCData data_ba = measureData(probe_b, probe_a);
  boolean ab_info = displayData(data_ab, data_ba, INFO_AB_X, INFO_AB_Y);

  // Measure data and display results between probe b and c
  ADCData data_bc = measureData(probe_b, probe_c);
  ADCData data_cb = measureData(probe_c, probe_b);
  boolean bc_info = displayData(data_bc, data_cb, INFO_BC_X, INFO_BC_Y);

  // Perform measurements to see if there is a transistor and the kind of pin configuration
  int trans_gain = 0;
  int trans_gain_new;
  char trans_a = 'A';
  char trans_b = 'B';
  char trans_c = 'C';
  for (int i_collector = 0; i_collector < 3; i_collector++) {
    for (int i_base = 0; i_base < 3; i_base++) {
      for (int i_emitter = 0; i_emitter < 3; i_emitter++) {
        
        // Don't do any measurements where probes are the same!
        if (i_collector == i_base || i_collector == i_emitter || i_base == i_emitter) {
          continue;
        }

        // Attempt to measure transistor gain
        trans_gain_new = calcTransistorGain(probes[i_emitter], probes[i_base], probes[i_collector]);

        // Pick the first valid and highest gain configuration
        if (trans_gain_new != 0 && abs(trans_gain_new) > abs(trans_gain)) {
          trans_gain = trans_gain_new;
          trans_a = (i_collector == 0) ? 'C' : (i_base == 0) ? 'B' : 'E';
          trans_b = (i_collector == 1) ? 'C' : (i_base == 1) ? 'B' : 'E';
          trans_c = (i_collector == 2) ? 'C' : (i_base == 2) ? 'B' : 'E';
        }
      }
    }
  }

  if (trans_gain != 0) {
    info_box_shown = true;

    // Draw a box for transistor info
    display.fillRect(TRANS_X + 1, TRANS_Y + 1, TRANS_W - 2, TRANS_H - 2, BACKCOLOR2);
    display.drawRect(TRANS_X, TRANS_Y, TRANS_W, TRANS_H, FRAMECOLOR);
    display.setTextColor(TEXTCOLOR);

    // Print the transistor type
    display.setTextSize(2);
    display.setCursor(TRANS_X + 5, TRANS_Y + 5);
    if (trans_gain > 0) {
      display.print("PNP");
    } else {
      display.print("NPN");
    }
    display.setTextSize(1);
    display.setCursor(display.getCursorX() + 2, TRANS_Y + 12);
    display.print("transistor");

    // Print Hfe gain
    display.setTextSize(2);
    display.setCursorDown(TRANS_X + 5);
    int gain_y = display.getCursorY();
    display.print('H');
    display.setTextSize(1);
    display.setCursorDown(display.getCursorX());
    display.print("fe");
    display.setTextSize(2);
    display.setCursor(display.getCursorX(), gain_y);
    display.print(' ');
    display.print(abs(trans_gain));

    // Print Collector/Base/Emitter info
    drawProbe(probe_a, trans_a);
    drawProbe(probe_b, trans_b);
    drawProbe(probe_c, trans_c);
  } else {
    // No transistor, look for two disconnected probes
    // How many valid info boxes are shown?
    int info_cnt = (ac_info ? 1 : 0) + (ab_info ? 1 : 0) + (bc_info ? 1 : 0);
    if (info_cnt == 1) {
      // Very similar logic like drawInfo, but then with bigger font
      // Draw a box for other component info
      info_box_shown = true;
      display.fillRect(TRANS_X + 1, TRANS_Y + 1, TRANS_W - 2, TRANS_H - 2, BACKCOLOR2);
      display.drawRect(TRANS_X, TRANS_Y, TRANS_W, TRANS_H, FRAMECOLOR);
      if (ac_info) {
        drawInfoBox(probe_a, &data_ac, probe_c, &data_ca);
      } else if (ab_info) {
        drawInfoBox(probe_a, &data_ab, probe_b, &data_ba);
      } else if (bc_info) {
        drawInfoBox(probe_b, &data_bc, probe_c, &data_cb);
      }
    } else {
      // No special information to be shown - three-probe component
      if (info_box_shown) {
        // No transistor, wipe the area
        display.fillRect(TRANS_X, TRANS_Y, TRANS_W, TRANS_H, BLACK);
        // Redraw the wire diagram
        drawWireDiagram();
      }
      info_box_shown = false;
    }
  }

  // Select the best fit graph
  if (data_ac.capacitance > 0.0F || data_ca.capacitance > 0.0F) {
    updateGraph(graph_a, data_ac);
    updateGraph(graph_b, data_ca);
  } else if (data_ab.capacitance > 0.0F || data_ba.capacitance > 0.0F) {
    updateGraph(graph_a, data_ab);
    updateGraph(graph_b, data_ba);
  } else {
    updateGraph(graph_a, data_bc);
    updateGraph(graph_b, data_cb);
  }

  Serial.print("WAITING...");
  for (;;) {
    display.update();
    if (display.isTouchEnter(INFO_AC_X, INFO_AC_Y, INFO_WIDTH, INFO_HEIGHT)) {
      updateGraph(graph_a, data_ac);
      updateGraph(graph_b, data_ca);
    } else if (display.isTouchEnter(INFO_AB_X, INFO_AB_Y, INFO_WIDTH, INFO_HEIGHT)) {
      updateGraph(graph_a, data_ab);
      updateGraph(graph_b, data_ba);
    } else if (display.isTouchEnter(INFO_BC_X, INFO_BC_Y, INFO_WIDTH, INFO_HEIGHT)) {
      updateGraph(graph_a, data_bc);
      updateGraph(graph_b, data_cb);
    } else if (start_btn.isClicked()) {
      break;
    }
  }
  display.update();
  Serial.println("DONE");
}

void updateGraph(PHN_LineGraph &graph, ADCData &data) {
  graph.clear();
  for (int i = 0; i < MAX_DATA_LENGTH; i++) {
    graph.addValue((float) data.data[i]);
  }
}

void drawProbe(Pins probe, char c) {
  int x;
  color_t color;
  if (probe.adc == probe_a.adc) {
    x = INFO_LN_X2;
    color = BLUE;
  } else if (probe.adc == probe_b.adc) {
    x = INFO_MN_X;
    color = GREEN;
  } else {
    x = INFO_RN_X2;
    color = YELLOW;
  }
  display.setTextSize(2);
  display.setTextColor(TEXTCOLOR);
  display.setCursor(x - 4, TRANS_PY - 4);
  display.print(c);
  display.drawCircle(x, TRANS_PY + 2, 10, color);
}

void drawInfoBox(Pins probe_a, ADCData *data_a, Pins probe_b, ADCData *data_b) {
  display.setTextColor(WHITE);
  display.setTextSize(2);
  display.setCursor(TRANS_X + 5, TRANS_Y + 5);
  
  // Figure out what is connected between probe and probe b
  // Ignore the no-connection and direct-connection checks
  boolean has_resistance = (data_a->resistance != INFINITE || data_b->resistance != INFINITE);
  boolean has_capacitance = (data_a->capacitance != 0.0F || data_b->capacitance != 0.0F);
  
  if (has_resistance) {
    float resist_avg = 0.5 * (data_a->resistance + data_b->resistance);
    if ((data_a->resistance == INFINITE) != (data_b->resistance == INFINITE)) {

      // Resistor difference too big - assume diodes
      if (data_b->resistance == INFINITE) {
        // Diode to the left
        display.print("DIODE");
        display.setCursorDown(TRANS_X + 5);
        display.print(data_a->stable_volt);
        display.print('V');
        // Mark probes
        drawProbe(probe_a, '-');
        drawProbe(probe_b, '+');
      } else {
        // Diode to the right
        display.print("DIODE");
        display.setCursorDown(TRANS_X + 5);
        display.print(data_b->stable_volt);
        display.print('V');
        // Mark probes
        drawProbe(probe_a, '+');
        drawProbe(probe_b, '-');
      }
    } else {
      // Single resistor
      display.print("RESISTOR");
      display.setCursorDown(TRANS_X + 5);
      printResistance(resist_avg);
      // Mark probes
      drawProbe(probe_a, '|');
      drawProbe(probe_b, '|');
    }
  }
  if (has_capacitance) {
    // Draw capacitance
    display.print("CAPACITOR");
    display.setCursorDown(TRANS_X + 5);
    printCapacitance(max(data_a->capacitance, data_b->capacitance));
    // Mark probes
    drawProbe(probe_a, '=');
    drawProbe(probe_b, '=');
  }
}

/* Displays an information box about the data from/to two probes */
boolean displayData(ADCData &data_a, ADCData &data_b, int x, int y) {
  boolean has_resistance = (data_a.resistance != INFINITE || data_b.resistance != INFINITE);
  boolean has_capacitance = (data_a.capacitance != 0.0F || data_b.capacitance != 0.0F);
  boolean has_doublediode = false;

  // Zero-resistance is drawn as a straight line, indicating shorted
  if (data_a.resistance == 0.0F && data_b.resistance == 0.0F) {
    display.drawHorizontalLine(x, y + INFO_HEIGHT / 2, INFO_WIDTH, WHITE);
    return false;
  }

  // When no connection was found, draw 'N.C'
  if (!has_resistance && !has_capacitance) {
    display.setTextSize(2);
    display.setCursor(x + 13, y + 9);
    display.setTextColor(TEXTCOLOR);
    display.print("N.C.");
    return false;
  }

  // Proceed to display resistor/capacitor/diode information
  display.setTextSize(1);
  display.setCursor(x + 5, y + 5);
  display.setTextColor(TEXTCOLOR);
  if (has_resistance) {
    float resist_avg = 0.5 * (data_a.resistance + data_b.resistance);
    if ((data_a.resistance == INFINITE) != (data_b.resistance == INFINITE)) {

      // Resistor difference too big - assume diodes
      if (data_b.resistance == INFINITE) {
        // Diode to the left
        display.printMem(ldiode_char);
        display.print(' ');
        printResistance(data_a.resistance);
        display.setCursorDown(x + 5);
        display.print("  ");
        display.print(data_a.stable_volt);
        display.print('V');
      } else {
        // Diode to the right
        display.printMem(rdiode_char);
        display.print(' ');
        printResistance(data_b.resistance);
        display.setCursorDown(x + 5);
        display.print("  ");
        display.print(data_b.stable_volt);
        display.print('V');
      }
    } else if (abs(data_a.resistance - data_b.resistance) > (0.25 * resist_avg)) {
      // Two resistors/diodes, ignore voltage readout
      // Diode to the left
      has_doublediode = true;
      display.printMem(ldiode_char);
      display.print(' ');
      printResistance(data_a.resistance);
      display.setCursorDown(x + 5);
      // Diode to the right
      display.printMem(rdiode_char);
      display.print(' ');
      printResistance(data_b.resistance);
    } else {
      // Single resistor
      display.printMem(resistor_char);
      display.print(' ');
      printResistance(resist_avg);
    }
    // Newline for whatever is to come
    display.setCursorDown(x + 5);
  }
  if (has_capacitance) {
    // Draw capacitance
    display.printMem(capacitance_char);
    display.print(' ');
    printCapacitance(max(data_a.capacitance, data_b.capacitance));
  }
  // Info shown, TRUE when only resistance/capacitance and no double-diode
  return (has_capacitance != has_resistance) && !has_doublediode;
}

/* Draws a diagram showing how the indvidual boxes are connected to the probes */
void drawWireDiagram() {
  display.drawLine(INFO_LN_X, INFO_AC_Y + INFO_HEIGHT / 2,
                   INFO_AC_X, INFO_AC_Y + INFO_HEIGHT / 2, BLUE);
  display.drawLine(INFO_AC_X + INFO_WIDTH, INFO_AC_Y + INFO_HEIGHT / 2,
                   INFO_RN_X, INFO_AC_Y + INFO_HEIGHT / 2, YELLOW);
  display.drawLine(INFO_AB_X + INFO_WIDTH, INFO_AB_Y + INFO_HEIGHT / 2, 
                   INFO_BC_X, INFO_BC_Y + INFO_HEIGHT / 2, GREEN);
  display.drawLine(INFO_LN_X, INFO_AB_Y + INFO_HEIGHT / 2,
                   INFO_AB_X, INFO_AB_Y + INFO_HEIGHT / 2, BLUE);
  display.drawLine(INFO_BC_X + INFO_WIDTH, INFO_BC_Y + INFO_HEIGHT / 2,
                   INFO_RN_X, INFO_BC_Y + INFO_HEIGHT / 2, YELLOW);
  display.drawLine(INFO_LN_X, INFO_AC_Y + INFO_HEIGHT / 2,
                   INFO_LN_X, INFO_PR_Y1, BLUE);
  display.drawLine(INFO_LN_X, INFO_PR_Y1,
                   INFO_LN_X2, INFO_PR_Y1, BLUE);
  display.drawLine(INFO_LN_X2, INFO_PR_Y1,
                   INFO_LN_X2, INFO_PR_Y2, BLUE);
  display.drawLine(INFO_MN_X, INFO_AB_Y + INFO_HEIGHT / 2,
                   INFO_MN_X, INFO_PR_Y2, GREEN);
  display.drawLine(INFO_RN_X, INFO_AC_Y + INFO_HEIGHT / 2,
                   INFO_RN_X, INFO_PR_Y1, YELLOW);
  display.drawLine(INFO_RN_X, INFO_PR_Y1,
                   INFO_RN_X2, INFO_PR_Y1, YELLOW);
  display.drawLine(INFO_RN_X2, INFO_PR_Y1,
                   INFO_RN_X2, INFO_PR_Y2, YELLOW);

  // Probe box
  display.drawRect((INFO_MN_X - (INFO_PB_W / 2)), INFO_PR_Y2, INFO_PB_W + 1, INFO_PB_H, WHITE);
  display.drawRect(INFO_LN_X2 - INFO_PB_HS / 2, INFO_PB_HY, INFO_PB_HS, INFO_PB_HS, WHITE);
  display.drawRect(INFO_MN_X - INFO_PB_HS / 2, INFO_PB_HY, INFO_PB_HS, INFO_PB_HS, WHITE);
  display.drawRect(INFO_RN_X2 - INFO_PB_HS / 2, INFO_PB_HY, INFO_PB_HS, INFO_PB_HS, WHITE);
}

/* Draws the frame in which the individual probe-to-probe info is shown */
void drawInfoFrame(int x, int y) {
  display.drawRect(x, y, INFO_WIDTH, INFO_HEIGHT, FRAMECOLOR);
  display.fillRect(x + 1, y + 1, INFO_WIDTH - 2, INFO_HEIGHT - 2, BACKCOLOR2);
}

/* Prints capacitance readings to the display */
void printCapacitance(float capacitance) {
  if (capacitance > 10) {
    display.print(capacitance);
    display.print("F");
  } else if (capacitance >= 0.001) {
    display.print(capacitance * 1000.0);
    display.print("uF");
  } else if (capacitance >= 0.000001) {
    display.print(capacitance * 1000000.0);
    display.print("nF");
  } else {
    display.print(capacitance * 1000000000.0);
    display.print("pF");
  }
}

/* Prints resistance readings to the display */
void printResistance(float resistance) {
  if (resistance == INFINITE) {
    display.print("N.C.");
    return;
  } else if (resistance >= 200000.0F) {
    display.print(resistance / 1000000.0F);
    display.print("m");
  } else if (resistance >= 2000.0F) {
    display.print(resistance / 1000.0F);
    display.print("k");
  } else {
    display.print(resistance);
  }
  display.printMem(ohmega_char);    
}
