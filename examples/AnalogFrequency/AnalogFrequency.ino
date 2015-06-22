/*
 * Measures the Frequency or RPM of a signal at A0.
 * Aside from showing signal information, a graph is shown on the screen.
 * A free-running ADC (interrupt-based) is used to perform measurements async.
 * This allows accurate measurements despite slowdown.
 *
 * Note that since we are occupying the ADC at all times, no LCD touch input
 * can be used. For this reason, updateWidgets() is used instead of update().
 * If touch input is needed, the only option is to disable sampling shortly
 * and read the touch, turning it back on again afterwards. This does, however,
 * result in a break during the measurements, which may not be as desired.
 *
 * 
 */
#include "Phoenard.h"

#define UPDATE_INTERVAL  50  /* How often frequency output is displayed, in ms */
#define IR_ADC_INPUT     0   /* ADC channel to capture */

// Frequency and other signal information extracted from sampling the ADC
long lastFreqUpdate = 0;
float baseLine = 0.0F;
float minimum = 0.0F;
float maximum = 0.0F;
float frequency = 0.0F;

/* These variables are used in the ADC update interrupt */
volatile uint16_t adc_slopeThres = 0;   /* Values below this threshold count as a new cycle */
volatile uint32_t adc_inputSum = 0;     /* Sum of all ADC values since last read (for graph) */
volatile uint16_t adc_inputCnt = 0;     /* Number of ADC values in the ADC sum */
volatile uint16_t adc_minimum = 1023;   /* Minimum ADC value encountered since last run */
volatile uint16_t adc_maximum = 0;      /* Maximum ADC value encountered since last run */
volatile uint8_t  adc_isTopSlope = 0;   /* Whether currently in the top part of the cycle */
volatile uint32_t adc_counterLive = 0;  /* ADC interval counter of the current frequency slope */
volatile uint32_t adc_freqCtr = 0;      /* Total ADC intervals that happened for the frequency */
volatile uint32_t adc_freqDiv = 0;      /* Total amount of frequency samples found, or the divider */

// Widgets for display
PHN_LineGraph graph;
PHN_Label freqLbl;
PHN_Label rpmLbl;
PHN_Label boundsLbl;

void setup() {
  // Initialize the ADC registers to receive input in the interrupt
  adcInit();
  
  // Initialize graph
  graph.setBounds(0, 0, display.width(), 220);
  graph.setRange(0.0F, 1023.0F);
  graph.setLineCount(2);
  graph.setLineColor(0, WHITE);
  graph.setLineColor(1, GREEN);
  graph.setAutoClear(true);
  display.addWidget(graph);

  // Initialize frequency display label
  freqLbl.setBounds(0, 220, 80, 20);
  display.addWidget(freqLbl);

  // Initialize Rpm/Bpm display label
  rpmLbl.setBounds(80, 220, 80, 20);
  display.addWidget(rpmLbl);
  
  // Initialize min/max display label
  boundsLbl.setBounds(160, 220, 160, 20);
  display.addWidget(boundsLbl);
  
  // Perform an update now
  display.updateWidgets();
}

void loop() {
  float values[4];

  // Continuously read in the buffered values
  // Make sure to turn off the interrupts during it
  cli();
  values[0] = adc_inputSum / adc_inputCnt;
  if (adc_minimum < minimum) minimum = adc_minimum;
  if (adc_maximum > maximum) maximum = adc_maximum;
  adc_inputSum = 0;
  adc_inputCnt = 0;
  adc_minimum = 1023;
  adc_maximum = 0;

  // Update frequency every now and again (more below)
  boolean doFreqUpdate = ((millis() - lastFreqUpdate) >= UPDATE_INTERVAL);
  if (doFreqUpdate) {
    // Update frequency; 9600 cycles = 1Hz.
    frequency = 9600.0F / (float) ((float) adc_freqCtr / (float) adc_freqDiv);
    adc_freqCtr = 0;
    adc_freqDiv = 0;
    lastFreqUpdate = millis();
  }

  // Enable interrupts
  sei();

  // Update and set baseline average
  maximum += 0.01 * (values[0] - maximum);
  minimum += 0.01 * (values[0] - minimum);
  baseLine = (minimum + maximum) / 2.0F;
  values[1] = baseLine;

  // Set threshold to in between the minimum and baseline
  adc_slopeThres = (uint16_t) (minimum + ((maximum - minimum) / 4.0F));

  // Add to graph
  graph.addValues(values);

  // Refresh the display when frequency is updated
  if (doFreqUpdate) {
    
    // Update Frequency
    String freqTxt;
    if (frequency >= 1000.0F) {
      freqTxt += frequency / 1000.0F;
      freqTxt += " kHz";
    } else {
      freqTxt += frequency;
      freqTxt += " Hz";
    }
    freqLbl.setText(freqTxt);
    
    // Update Rpm
    String rpmTxt;
    rpmTxt += (frequency*60.0F);
    rpmTxt += " rpm";
    rpmLbl.setText(rpmTxt);
    
    // Update bounds
    String boundsTxt;
    boundsTxt += "[";
    boundsTxt += minimum;
    boundsTxt += " to ";
    boundsTxt += maximum;
    boundsTxt += "]";
    boundsLbl.setText(boundsTxt);
  }

  // Update all widgets (note: do NOT use update(), we are using the ADC)
  display.updateWidgets();

  // You can force additional delays here to slow down the graph
  delay(10);
}

/* free running ADC updates range and raw frequency measurement data */
ISR(ADC_vect) {
  // Input ADC value
  uint16_t value = ADC;

  // Hard-update minimum/maximum
  if (value > adc_maximum) adc_maximum = value;
  if (value < adc_minimum) adc_minimum = value;

  // Whenever the value shoots from below base to above base, reset counter
  if (adc_isTopSlope) {
    // Wait until value drops below baseline
    adc_isTopSlope = (value >= adc_slopeThres);
  } else if (value >= baseLine) {
    // Value now at top part of slope; reset
    adc_isTopSlope = 1;

    // Update frequency counters
    adc_freqDiv++;
    adc_freqCtr += adc_counterLive;
    adc_counterLive = 0;
  }

  // Store sample in the summing buffer
  adc_inputSum += value;
  adc_inputCnt++;
  adc_counterLive++;
}

/* Initializes ADC registers to use an interrupt-based free-running mode */
void adcInit(){
  /*  REFS0 : VCC use as a ref, IR_AUDIO : channel selection, ADEN : ADC Enable, ADSC : ADC Start, ADATE : ADC Auto Trigger Enable, ADIE : ADC Interrupt Enable,  ADPS : ADC Prescaler  */
  // free running ADC mode, f = ( 16MHz / prescaler ) / 13 cycles per conversion 
  ADMUX = _BV(REFS0) | IR_ADC_INPUT; // | _BV(ADLAR); 
  //ADCSRA = _BV(ADSC) | _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1); //prescaler 64 : 19231 Hz - 300Hz per 64 divisions
  ADCSRA = _BV(ADSC) | _BV(ADEN) | _BV(ADATE) | _BV(ADIE) | _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0); // prescaler 128 : 9615 Hz - 150 Hz per 64 divisions, better for most music
  sei();
}
