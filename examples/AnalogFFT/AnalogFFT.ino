/*
 * This is a Phoenard display implementation for an Arduino FFT example.
 * The analog intensity in time-domain and the FFT in frequency-domain are graphed on the screen.
 * By default analog channel 0 (A0) is used as input. Attach an amplified microphone signal to this pin to analyze sounds!
 *
 * This sketch requires the FFFT library to be installed. For convenience, we provide a mirror to it here:
 * http://phoenard.com/media/ffft.zip
 * 
 * Original Fixed point FFT library is from ELM Chan: http://elm-chan.org/works/akilcd/report_e.html
 * A way to port it to the Arduino library and most demo codes are from AMurchick
 * Forum post: http://arduino.cc/forum/index.php/topic,37751.0.html
 *
 * This example acquires an analog signal from A0 and displays it on the screen
 * Analog signal is captured at 9.6 KHz, 64 spectrum bands each 150Hz which can be changed in adcInit()
 */

// Definition needed to make sure ffft library works
#define prog_int16_t int16_t

#include "Phoenard.h"
#include <ffft.h>

#define IR_ADC_INPUT  0   // ADC channel to capture
#define RANDOM_NOISE  0   // Set to non-zero to add random() noise

/* Time-domain graph settings */
#define TD_GRAPH_X     6
#define TD_GRAPH_Y     21
#define TD_GRAPH_W     306
#define TD_GRAPH_H     90

/* Frequency-domain graph settings */
#define FD_GRAPH_X     5
#define FD_GRAPH_Y     (display.height()-14)
#define FD_GRAPH_SIZE  4
#define FD_GRAPH_CNT   62

/* FFT state variables */
int16_t fft_zeroPoint = 0;           /* Zero-center point relative to which ADC is measured */
float fft_zeroPointF = 0.0F;         /* Zero-center point running average */
volatile byte fft_position = 0;      /* Running position of the FFT */
int16_t fft_capture[FFT_N];          /* Waveform ADC capturing buffer */
complex_t bfly_buff[FFT_N];          /* FFT buffer */
uint16_t fft_spectrum[FFT_N/2];      /* Spectrum output buffer */
uint16_t fft_spectrumPast[FFT_N/2];  /* Tracks previous spectrum to render differences */

/* Time domain graph variables */
long intensity = 30;          /* Time-domain intensity */
long intensityPast = 30;      /* Time-domain intensity of previous run */
long timePos = TD_GRAPH_X;  /* Time-domain running time position */

void setup() {
  // Initialize the ADC registers to receive input in the interrupt
  adcInit();

  // Draw the frames in which the graphs are shown
  display.drawRoundRect(0,0,display.width(),display.height(),7,RED);
  display.drawRoundRect(2,12,display.width()-4,display.height()/2-17,7,BLUE);
  display.drawRoundRect(2,display.height()/2+5,display.width()-4,display.height()/2-7,7,BLUE);

  // Top title
  display.setTextColor(WHITE);
  display.drawStringMiddle(0,2,display.width(),10,"Signal Analyzer (FFT and RAW)");

  // Graph titles
  display.setTextColor(YELLOW);
  display.drawStringMiddle(0,13,display.width(),10,"Intensity in Time Domain");
  display.drawStringMiddle(0,126,display.width(),10,"Frequency Using FFT");

  // Frequency labeling
  display.setTextColor(CYAN);
  display.drawStringMiddle(0,226,display.width(),10,"2.4kHz");
  display.drawStringMiddle(0,226,40,10,"0kHz");
  display.drawStringMiddle(276,226,40,10,"4.8kHz");
}

void loop() {
  // When interrupt finishes filling the buffer, execute and display FFT
  if (fft_position == FFT_N) {
    fft_input(fft_capture, bfly_buff);
    fft_execute(bfly_buff);
    fft_output(bfly_buff, fft_spectrum);
    displayFFT();
    
    // Fill the buffer again
    fft_position = 0;
  }
}

/* Draws the FFT spectrum and time-domain analysis to the screen */
void displayFFT() {
  // Add all previously captured ADC values to form an intensity
  // At the same time, update the difference for each frequency bar
  long captureSum = 0;
  int x = FD_GRAPH_X;
  int y = FD_GRAPH_Y;
  int fft_spectrumOld, fft_spectrumNew, fft_spectrumDiff;
  for (byte i = 0; i < FD_GRAPH_CNT; i++) {  
    captureSum += fft_capture[i];
    fft_spectrumOld = min(fft_spectrumPast[i], 80);
    fft_spectrumNew = min(fft_spectrum[i], 80);
    fft_spectrumDiff = fft_spectrumOld - fft_spectrumNew;
    fft_spectrumPast[i] = fft_spectrum[i];

    if (fft_spectrumDiff > 0) {  
      display.fillRect(x, y - fft_spectrumOld, FD_GRAPH_SIZE, fft_spectrumDiff, BLACK);
    } else if (fft_spectrumDiff < 0) {
      display.fillRect(x, y - fft_spectrumNew, FD_GRAPH_SIZE, fft_spectrumNew, GREEN);
    }
    x += FD_GRAPH_SIZE + 1;
  }

  // Refresh zero-point using a running average
  fft_zeroPointF += 0.01 * (float) (captureSum / FD_GRAPH_CNT);
  fft_zeroPoint = (int16_t) fft_zeroPointF;

  // Convert the sum of all waveform captures into a graph intensity range
  intensity = map(captureSum,-10000,10000,TD_GRAPH_Y,TD_GRAPH_Y+TD_GRAPH_H);
  intensity = constrain(intensity, TD_GRAPH_Y, TD_GRAPH_Y+TD_GRAPH_H);

  // Draw time-domain graph line
  display.drawLine(timePos,intensityPast,timePos+1,intensity,GREEN);
  intensityPast=intensity;

  // Increment time position, wipe when fully filled up
  if (++timePos >= (TD_GRAPH_X+TD_GRAPH_W)) {
    timePos = TD_GRAPH_X;
    display.fillRect(TD_GRAPH_X, TD_GRAPH_Y, TD_GRAPH_W+2, TD_GRAPH_H+1, BLACK);
  }
}

/* free running ADC fills capture buffer */
ISR(ADC_vect) {
  int16_t value;
  if (fft_position < FFT_N) {
    /* Input ADC value */
    value = ADC;

    /* Subtract zero-offset */
    value -= fft_zeroPoint;

    /* Add random noise */
    if (RANDOM_NOISE) {
      value += random(-RANDOM_NOISE, RANDOM_NOISE);
    }

    /* Store sample in fft capture buffer */
    fft_capture[fft_position++] = value;
  }
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
