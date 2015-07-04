#include "Measurement.h"

/* Definition of the probe array */
Pins probes[3] = {probe_a, probe_b, probe_c};

/* Performs a full measurement between two probes, evaluating the counter-resistor to use */
ADCData measureData(Pins probe_a, Pins probe_b) {
  // Data to be returned
  ADCData data;

  // First perform a quick check to see if there is a connection at all
  resetProbes();
  setPin(probe_a.res_100k, LOW);
  setPin(probe_b.adc, HIGH);
  delayMicroseconds(10);
  int adc_a = analogRead(probe_a.adc);

  // If first ADC value is too low, there is no capacitance/resistance
  if (adc_a <= 100) {
    // No connection, return empty data
    data = getEmptyData(1023);
    strcpy(data.mode, "NC");
    resetProbes();
    return data;
  }

  // Check for a potential resistor
  int adc_b;
  if (adc_a < 1000) {
    // Small capacitor or high resistance
    // If the voltage level went down too much, it is a small capacitor
    delay(2);
    adc_b = analogRead(probe_a.adc);
    if (adc_b && (adc_a - adc_b) < 200) {
      // No connection, return arbitrary data
      long resistor_used;
      if (adc_b > 700) {
        // Requires use of the 10K resistor for better accuracy
        resetProbes();
        setPin(probe_a.res_10k, LOW);
        setPin(probe_b.adc, HIGH);
        delay(2);
        adc_b = analogRead(probe_a.adc);
        resistor_used = 10000;
      } else {
        // Use of the 100K resistor is fine in this range
        resistor_used = 100000;
      }
      data = getEmptyData(1018 - adc_b);
      data.stable_volt = 3.3F * (1023 - adc_b) / 1023.0F;
      data.resistance = calcResistance(1018 - adc_b, resistor_used);
      if (data.resistance > MAX_RESISTANCE) {
        data.resistance = INFINITE;
      }
      if (resistor_used == 10000) {
        strcpy(data.mode, "R10K");
      } else {
        strcpy(data.mode, "R100K");
      }
      resetProbes();
      return data;
    }
  } else {
    // Large capacitor or small resistance
    // Charge using a smaller resistor and see where we will go
    resetProbes();
    setPin(probe_a.res_1k, LOW);
    setPin(probe_b.adc, HIGH);
    adc_a = analogRead(probe_a.adc);
    delay(2);
    adc_b = analogRead(probe_a.adc);
    if ((adc_a - adc_b) <= 1) {
      // Small Resistor
      data = getEmptyData(1010 - adc_b);
      strcpy(data.mode, "R1K");
      data.stable_volt = 3.3F * (1010 - adc_b) / 1023.0F;
      data.resistance = calcResistance(1010 - adc_b, 1000);
      resetProbes();
      return data;
    }
  }

  // Reset before starting measurements
  resetProbes();

  // Resistance values measured
  float resist_1k, resist_10k, resist_100k;
  int adc_1k, adc_10k, adc_100k;
  
  // If the time it took was relatively long, capacitance outweighs importance of resistance
  // Select it and be done!
  data = measureData(probe_a, probe_b, 1000);
  resist_1k = data.resistance;
  adc_1k = data.data[MAX_DATA_LENGTH - 1];
  if (data.time > 5000) {
    calcCapacitance(data);
    if (data.resistance > 20000.0F) {
      data.resistance = INFINITE;
    }
    return data;
  }
  data = measureData(probe_a, probe_b, 10000);
  resist_10k = data.resistance;
  adc_10k = data.data[MAX_DATA_LENGTH - 1];
  if (data.time > 5000) {
    calcCapacitance(data);
    if (data.resistance > 200000.0F) {
      data.resistance = INFINITE;
    }
    return data;
  }
  data = measureData(probe_a, probe_b, 100000);
  resist_100k = data.resistance;
  adc_100k = data.data[MAX_DATA_LENGTH - 1];
  if (data.time_adj > 1.0) {
    calcCapacitance(data);
    if (data.resistance > MAX_RESISTANCE) {
      data.resistance = INFINITE;
    }
    return data;
  }

  // No capacitor detected, measure resistance using the best suitable value
  if (resist_1k < 5000) {
    data = getEmptyData(adc_1k);
    data.resistance = resist_1k;
    strcpy(data.mode, "W1K");
  } else if (resist_10k < 50000) {
    data = getEmptyData(adc_10k);
    data.resistance = resist_10k;
    strcpy(data.mode, "W10K");
  } else {
    data = getEmptyData(adc_100k);
    data.resistance = resist_100k;
    strcpy(data.mode, "W100K");
  }
  if (data.resistance > MAX_RESISTANCE) {
    data.resistance = INFINITE;
  }

  return data;
}

/* Measures the capacitance and resistance between two probes using a specific counter-resistor */
ADCData measureData(Pins probe_a, Pins probe_b, long counter_resistor) {
  // AnalogRead takes about 100 microseconds to complete
  // First measure every 100us, then every 200us, then 400us, etc.
  // This way we can do a large range of larger capacitance testing
  ADCData data;
  
  // Initial data state, first value is ADC 0
  data.time = 0;
  data.length = 0;

  // Assume ADC(0) = 0
  data.data[data.length++] = 0;

  // Find the resistor pins to toggle
  int resistor_pin;
  if (counter_resistor == 1000) {
    resistor_pin = probe_b.res_1k;
    strcpy(data.mode, "C001K");
  } else if (counter_resistor == 10000) {
    resistor_pin = probe_b.res_10k;
    strcpy(data.mode, "C010K");
  } else if (counter_resistor == 100000) {
    resistor_pin = probe_b.res_100k;
    strcpy(data.mode, "C100K");
  } else {
    resistor_pin = probe_b.res_100k;
    strcpy(data.mode, "CUNK");
  }

  // First measure at the maximum rate of every 200 microseconds with no delays (use delay to calibrate)
  const int delay_time_min = 200;
  const int delay_time_min2 = 4;
  const int delay_time_self = 130;
  int delay_time = delay_time_min;
  int adc_value = 0;
  boolean do_delay = true;
  boolean use_slow_measurement = true;

  // Start charging at first
  resetProbes();
  setPin(probe_a.adc, LOW);
  setPin(resistor_pin, HIGH);

  // Start measuring right away
  for (;;) {
    if (use_slow_measurement) {
      // For more than 200 microsecond interval measurements, use repetetive analogRead calls

      // Microsecond delay
      if (do_delay) {
        if (delay_time > 16000) {
          delay((delay_time - delay_time_self) / 1000);
        } else {
          delayMicroseconds(delay_time - delay_time_self);
        }
      }
      do_delay = true;

      // Read ADC
      adc_value = analogRead(probe_b.adc);
      data.data[data.length++] = adc_value;
    } else {
      // In fast mode, fully discharge and charge for a known period

      // Start off by draining the capacitance really quickly
      setPin(probe_a.adc, LOW);
      setPin(probe_b.adc, LOW);
      setPin(probe_b.adc, FLOATING);
      setPin(resistor_pin, HIGH);
      delayMicroseconds(data.time);
      data.data[data.length++] = analogRead(probe_b.adc);

      // Change back to floating state to preserve charge
      setPin(probe_a.adc, FLOATING);
      setPin(resistor_pin, FLOATING);
    }

    // Increment time
    data.time += delay_time;

    // If the data is full, half it by discarding one half
    // Then double the delay interval
    // Note: this operation is a little slow, properly adjust the delay for this!
    if (data.length == MAX_DATA_LENGTH) {
      // Timeout when ADC is left unchanged for too long
      // This indicates a shortcut ('infinite capacitance')
      if (data.time > 1000 && !data.data[MAX_DATA_LENGTH - 1]) {
        break;
      }

      // Compare first half with second half to see if we finished a charging cycle
      const int slope_len = MAX_DATA_LENGTH / CAP_SLOPE_DIV;
      int slope_a = (data.data[slope_len - 1] - data.data[0]);
      int slope_b = (data.data[MAX_DATA_LENGTH - 1] - data.data[MAX_DATA_LENGTH - slope_len - 2]);
      if (slope_a > CAP_SLOPE_FACTOR * slope_b) {
        // If the data slope is too short to be usable, switch modes and retry with smaller intervals
        if (counter_resistor == 100000 && use_slow_measurement && abs(data.data[10] - data.data[data.length - 1]) < 10) {
          strcpy(data.mode + 5, "_F");
          use_slow_measurement = false;
          data.length = 1;
          delay_time = delay_time_min2;
          data.time = delay_time;
          resetProbes();
          continue;
        }
        break;
      }

      // Half data and double delay
      data.length = MAX_REDU_LENGTH;
      for (int i = 0; i < data.length; i++) {
        data.data[i] = data.data[(int) ((float) i / CAP_REDUCE_RATE)];
      }
      delay_time /= CAP_REDUCE_RATE;

      if (delay_time < 1000) {
        do_delay = false;
      }
    }
  }
  filterData(data);
  resetProbes();

  // Calculate resistance using the last known ADC value
  data.stable_volt = 3.3F * ((float) (1023.0 - adc_value) / 1023.0);
  data.resistance = calcResistance(adc_value, counter_resistor);

  // Resistor factor logic
  data.time_adj = (double) data.time / (double) (counter_resistor / 1000);

  return data;
}

/* Calculates the resistance by looking at the read ADC value and the counter-resistance used */
float calcResistance(int adc_value, long counter_resistor) {
  float res_volt = (float) (1023.0 - adc_value) / 1023.0;
  float resistance = ((float) counter_resistor * (1.0 / res_volt - 1.0));
  if (resistance < MIN_RESISTANCE) {
    resistance = 0.0F;
  }
  return resistance;
}

/* Calculates the capacitance using the data gathered, the result is stored inside the data struct */
void calcCapacitance(ADCData &data) {
  // First check if there is capacitance at all
  // Compare the slopes between first/middle and last/middle values
  float slope_a = data.data[MAX_DATA_LENGTH / 2] - data.data[0];
  float slope_b = data.data[MAX_DATA_LENGTH - 1] - data.data[MAX_DATA_LENGTH / 2];
  if (slope_a < (5.0 * slope_b)) {
    data.capacitance = 0.0F;
    return;
  }

  // Figure out how much time it took to achieve 75% of the maximum voltage
  float charge_time = 0.0F;
  int max_voltage = data.data[MAX_DATA_LENGTH - 1];
  int limit = (int) (0.75 * (float) max_voltage);
  for (int i = 0; i < MAX_DATA_LENGTH; i++) {
    if (data.data[i] >= limit) {
      charge_time = data.time_adj * ((float) i / (float) MAX_DATA_LENGTH);
      break;
    }
  }

  // These values were found after careful calibration
  // They depend on a lot of factors, including the measurement algorithm itself
  charge_time /= 6.3 * max_voltage;
  data.capacitance = 0.0048 * charge_time;
}

/* Obtains the gain of a transistor (Hfe), 0 if no gain, negative for NPN, positive for PNP */
int calcTransistorGain(Pins collector, Pins base, Pins emitter) {
  int adc_on, adc_off, adc_gain;
  boolean pnp = false;
  boolean npn = false;

  // Test for NPN transistor
  resetProbes();
  setPin(collector.adc, LOW);
  setPin(emitter.res_1k, HIGH);
  setPin(base.res_100k, LOW);
  delay(1);
  adc_off = 1023 - analogRead(emitter.adc);
  setPin(base.res_100k, HIGH);
  delay(1);
  adc_on = 1023 - analogRead(emitter.adc);
  adc_gain = (adc_on - adc_off);
  resetProbes();

  if (adc_gain < 3) {
    // PNP: perform a new measurement
    setPin(collector.adc, HIGH);
    setPin(emitter.res_1k, LOW);
    setPin(base.res_100k, HIGH);
    delay(1);
    adc_off = analogRead(emitter.adc);
    setPin(base.res_100k, LOW);
    delay(1);
    adc_on = analogRead(emitter.adc);
    adc_gain = (adc_on - adc_off);
    resetProbes();
    if (adc_gain >= 3) {
      pnp = true;
      adc_gain = 1023 - adc_gain;
    }
  } else {
    // NPN; use negative gain
    npn = true;
    adc_gain = -(1023 - adc_gain);
  }

  // No gain?
  if (!npn && !pnp) {
    return 0.0F;
  }

  /*
  Test results with various transistors:
  GAIN_ADC  Hfe
  42    520
  67    195
  73    144
  292   87
  439   75
  583   66
  807   23
  */

  // Transform adc_gain into Hfe by approximation
  const float fact = 21839.6F;
  return (int) (fact / (float) adc_gain);
}

/* Obtains empty, 'no-connection' data results */
ADCData getEmptyData(int adc_max) {
  ADCData data;
  data.time = 1;
  data.time_adj = 0.01;
  data.capacitance = 0.0F;
  data.resistance = INFINITE;
  data.length = MAX_DATA_LENGTH;
  data.stable_volt = 3.3F * (adc_max / 1023.0F);
  for (int i = 0; i < MAX_DATA_LENGTH; i++) {
    data.data[i] = (i == 0) ? 0 : adc_max;
  }
  return data;
}


/*
 * This is a fairly complex filtering algorithm meant to trim off irrelevant data from the
 * charging curve. Depending on cicumstances, the graph will show a really long end-slope
 * until the maximum capacitance is reached. This end-slope is trimmed off, after which the
 * curve is 'stretched', using interpolation to fill the trimmed-off part of the curve.
 *
 * End-result is a clean-looking capacitor charging curve.
 */
void filterData(ADCData &data) {
  // Do not do anything special if a maximum is achieved too soon
  if (abs(data.data[1] - data.data[data.length - 1]) < 10) {
    data.time /= MAX_DATA_LENGTH;
    return;
  }

  // Resize the data to exclude the linear slope at the end
  const int min_len = 10;
  for (int data_len = min_len; data_len < data.length; data_len++) {
    const int slope_len = data_len / CAP_SLOPE_DIV;
    int slope_a = (data.data[slope_len - 1] - data.data[0]);
    int slope_b = (data.data[data_len - 1] - data.data[data_len - slope_len - 2]);
    if (slope_a > CAP_SLOPE_FACTOR * slope_b) {
      data.time *= ((float) data_len / (float) MAX_DATA_LENGTH);
      data.length = data_len;
      break;
    }
  }

  // If data length was reduced, now use interpolation to fill the full range of values
  if (data.length < MAX_DATA_LENGTH) {
    // Copy old data before transforming
    int old_data[data.length];
    memcpy(old_data, data.data, data.length * sizeof(int));

    float idx_mod = (float) data.length / (float) MAX_DATA_LENGTH;
    int max_idx = data.length - 1;
    for (int i = 0; i < MAX_DATA_LENGTH; i++) {
      // Find the two indices of the source data in between that has to be read
      float floating_idx = (float) i * idx_mod;
      int idx_low  = min(max_idx, floor(floating_idx));
      int idx_high = min(max_idx, ceil(floating_idx));
      floating_idx -= idx_low;
      data.data[i] =((1.0 - floating_idx) * old_data[idx_low]) + (floating_idx * old_data[idx_high]);
    }
    data.length = MAX_DATA_LENGTH;
  }
}

/* Drains any capacitance voltage and sets all probes to the default INPUT state */
void resetProbes() {
  // Short turn pin 13 on to indicate we are discharging
  // For large capacitors, this indicator saves lives.
  pinMode(13, OUTPUT);
  digitalWrite(13, LOW);

  // Set all pins to floating except the 1K resistor, pull the group to LOW to drain
  for (int i = 0; i < 3; i++) {
    setPin(probes[i].adc, FLOATING);
    setPin(probes[i].res_1k, LOW);
    setPin(probes[i].res_10k, LOW);
    setPin(probes[i].res_100k, LOW);
  }

  // Wait until all voltage levels are drained enough
  boolean is_drained;
  do {
    is_drained = true;
    for (int i = 0; i < 3; i++) {
      int adc = analogRead(probes[i].adc);
      if (adc) {
        is_drained = false;
        break;
      }
    }
  } while (!is_drained);

  // Set remaining 1K pins to floating
  for (int i = 0; i < 3; i++) {
    setPin(probes[i].res_1k, FLOATING);
    setPin(probes[i].res_10k, FLOATING);
    setPin(probes[i].res_100k, FLOATING);
  }
  
  // Done charging!
  digitalWrite(13, HIGH);
}

/* Sets a single pin to LOW, HIGH or FLOATING state */
void setPin(int pin, int state) {
  if (state == FLOATING) {
    pinMode(pin, INPUT);
  } else {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, state);
  }
}
