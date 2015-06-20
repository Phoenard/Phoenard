/*
 * This section of code is all about performing resistance, capacitance and transistor Hfe measurement.
 * Measurement is based on working with probe pairs, each probe bundled with controlled 1k/10k/100k resistors.
 * Capacitance is measured by charging to a defined voltage (with a given resistor, or counter-resistor).
 * Resistance is measured by toggling a resistor, waiting for it to settle and then measuring the end-voltage.
 * Transistor Hfe is measured based on the transistor gain from providing power to the base.
 *
 * Pin probes (probe_a, probe_b, probe_c) must be defined elsewhere.
 */
#include <Arduino.h>

#ifndef _MEASUREMENT_H_
#define _MEASUREMENT_H_

/* Constants used during measurements */
#define INFINITE          (1.0 / 0.0)
#define FLOATING          0xFF
#define CAP_SLOPE_FACTOR  20
#define CAP_SLOPE_DIV     4
#define CAP_REDUCE_RATE   0.75
#define MAX_DATA_LENGTH   130
#define MAX_REDU_LENGTH   (int) (CAP_REDUCE_RATE * MAX_DATA_LENGTH)
#define MIN_RESISTANCE    20.0F
#define MAX_RESISTANCE    1000000.0F

/* Struct that stores the pins used to a single probe */
typedef struct {
  int adc;
  int res_1k;
  int res_10k;
  int res_100k;
} Pins;

/* Struct that stores measurement data */
typedef struct {
  int data[MAX_DATA_LENGTH];      // Collected ADC data over time
  int length;         // Length of the data collected (max=MAX_DATA_LENGTH)
  long time;          // Total measurement duration
  double time_adj;    // Total measurement duration with resistor taken into account
  float stable_volt;  // Stable voltage level that is achieved
  float resistance;   // Resistance value measured (estimated)
  float capacitance;  // Capacitance value measured (estimated)
  char mode[9];          // What measurement mode was used (textual, max 8 length)
} ADCData;

/* Pin definitions */
extern Pins probe_a, probe_b, probe_c;
extern Pins probes[3];

/* Function definitions for Measurement.cpp */
ADCData measureData(Pins probe_a, Pins probe_b);
ADCData measureData(Pins probe_a, Pins probe_b, long counter_resistor);
ADCData getEmptyData(int adc_max);
void filterData(ADCData &data);
void calcCapacitance(ADCData &data);
int calcTransistorGain(Pins collector, Pins base, Pins emitter);
float calcResistance(int adc_value, long counter_resistor);
void resetProbes();
void setPin(int pin, int state);

#endif
