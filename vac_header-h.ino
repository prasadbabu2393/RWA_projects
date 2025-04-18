
//button pressed function
bool isButtonPressed(int pin) {
    return digitalRead(pin) == LOW;
}

// -------------------------------------------------
// Utility function to round to the nearest valid step
// -------------------------------------------------
double roundToStep(double value, double step) {
  return step * round(value / step);
}

////////////////////// actual code /////////
// void adcReadings_fun()
// {
//    adcReading_1 = ads.readADC_SingleEnded(0);
//    float voltage1_now = (adcReading_1 * 4096.0) / 32768.0;

//    voltage1_buffer[buffer_index] = voltage1_now;

//    float sum1 = 0;
//    for (int i = 0; i < NUM_SAMPLES; i++) {
//      sum1 += voltage1_buffer[i];
//    }
//    float volt_plus1 = sum1 / NUM_SAMPLES;
//     voltage_1 = volt_plus1 ;//adding 5millis to average voltage
//    Serial.println("voltage_1 (avg)");
//    Serial.println(voltage_1);

//    dac_output_1 = (255 * voltage_1 / 1000.0) / 3.3;
//    dacWrite(DAC_1, dac_output_1);
//    vacuum1 = getVacuumFromVoltage(voltage_1);

//    adcReading_2 = ads.readADC_SingleEnded(1);
//    float voltage2_now = (adcReading_2 * 4096.0) / 32768.0;

//    voltage2_buffer[buffer_index] = voltage2_now;

//    float sum2 = 0;
//    for (int i = 0; i < NUM_SAMPLES; i++) {
//      sum2 += voltage2_buffer[i];
//    }
//     float volt_plus2 = sum2 / NUM_SAMPLES;
//     voltage_2 = volt_plus2 + 5;//adding 5millis to average voltage
//    Serial.println("voltage_2 (avg)");
//    Serial.println(voltage_2);//correct

//    dac_output_2 = (255 * voltage_2 / 1000.0) / 3.3;
//    dacWrite(DAC_2, dac_output_2);
//    vacuum2 = getVacuumFromVoltage(voltage_2);

//    adcReading_3 = ads.readADC_SingleEnded(2);
//    float voltage3_now = (adcReading_3 * 4096.0) / 32768.0;

//    voltage3_buffer[buffer_index] = voltage3_now;

//    float sum3 = 0;
//    for (int i = 0; i < NUM_SAMPLES; i++) {
//      sum3 += voltage3_buffer[i];
//    }
//    voltage_3 = sum3 / NUM_SAMPLES;

//    temperature_1 = 0.1442 * voltage_3 + 1.1585;

//    buffer_index++;
//    if (buffer_index >= NUM_SAMPLES) buffer_index = 0;
// }
////////////////
void adcReadings_fun()
{
   adcReading_1 = ads.readADC_SingleEnded(0);
   float voltage1_now = (adcReading_1 * 4096.0) / 32768.0;

   voltage1_buffer[buffer_index] = voltage1_now;

   float sum1 = 0;
   for (int i = 0; i < NUM_SAMPLES; i++) {
     sum1 += voltage1_buffer[i];
   }
   float volt_plus1 = sum1 / NUM_SAMPLES;
    voltage_1 = volt_plus1 ;//adding 5millis to average voltage
//    Serial.println("voltage_1 (avg)");
//    Serial.println(voltage_1);

//    dac_output_1 = (255 * voltage_1 / 1000.0) / 3.3;
//    dacWrite(DAC_1, dac_output_1);
//    vacuum1 = getVacuumFromVoltage(voltage_1);

   adcReading_2 = ads.readADC_SingleEnded(1);
   float voltage2_now = (adcReading_2 * 4096.0) / 32768.0;

   voltage2_buffer[buffer_index] = voltage2_now;

   float sum2 = 0;
   for (int i = 0; i < NUM_SAMPLES; i++) {
     sum2 += voltage2_buffer[i];
   }
    float volt_plus2 = sum2 / NUM_SAMPLES;
    voltage_2 = volt_plus2 ;//adding 5millis to average voltage
//    Serial.println("voltage_2 (avg)");
//    Serial.println(voltage_2);//correct

    voltage_diff = voltage_1 - voltage_2 - 10;
    Serial.println("voltage (diff)");
    Serial.println(voltage_diff);//correct

   dac_output_2 = (255 * voltage_diff / 1000.0) / 3.3;
   dacWrite(DAC_2, dac_output_2);
   vacuum1 = getVacuumFromVoltage(voltage_diff);

   adcReading_3 = ads.readADC_SingleEnded(2);
   float voltage3_now = (adcReading_3 * 4096.0) / 32768.0;

   voltage3_buffer[buffer_index] = voltage3_now;

   float sum3 = 0;
   for (int i = 0; i < NUM_SAMPLES; i++) {
     sum3 += voltage3_buffer[i];
   }
   voltage_3 = sum3 / NUM_SAMPLES;

   temperature_1 = 0.1442 * voltage_3 + 1.1585;

   buffer_index++;
   if (buffer_index >= NUM_SAMPLES) buffer_index = 0;
}
////////////////

float vaccum_vales(double voltage)
{
  // ---------------------------
  // 2. Select the appropriate linear model based on voltage
  //    and compute the vacuum value
  // ---------------------------
  float vacuum = 0.0;
  double validStep = 0.0;  // The rounding step for the scale in use

  if (voltage < THRESHOLD1) {
    vacuum = slope1 * voltage + intercept1;
    validStep = 0.001;
  } else if (voltage < THRESHOLD2) {
    vacuum = slope2 * voltage + intercept2;
    validStep = 0.1;
  } else if (voltage < THRESHOLD3) {
    vacuum = slope3 * voltage + intercept3;
    validStep = 10.0;
  } else {
    vacuum = slope4 * voltage + intercept4;
    validStep = 100.0;
  }

  // ---------------------------
  // 3. Round the computed vacuum to the nearest valid step
  // ---------------------------
  vacuum = roundToStep(vacuum, validStep);

  // Serial.print("Computed Vacuum (rounded): ");
  // Serial.println(vacuum, 6);
  return vacuum;
}

// Convert decimal to scientific notation
ScientificNotation toScientific(float value) {
    ScientificNotation result;
    
    if (value == 0) {
        result.mantissa = 0;
        result.exponent = 0;
        return result;
    }
    
    // Get absolute value for calculation
    float absValue = abs(value);
    int exp = 0;
    
    // Convert to 1.0 <= mantissa < 10.0 format
    while (absValue >= 10.0) {
        absValue /= 10.0;
        exp++;
    }
    
    while (absValue < 1.0 && absValue > 0.0) {
        absValue *= 10.0;
        exp--;
    }
    
    // Restore sign
    result.mantissa = (value < 0) ? -absValue : absValue;
    result.exponent = exp;
    
    return result;
}

//printing the values on displays
void showScientific(TM1637Display &disp, float num, int exponent) {
    int intNum = (int)(num * 10);
    uint8_t digits[4];
    digits[0] = disp.encodeDigit((intNum / 10) % 10) | 0x80;
    digits[1] = disp.encodeDigit(intNum % 10);
    digits[2] = (exponent >= 0) ? 0x00 : 0x40;
    digits[3] = disp.encodeDigit(abs(exponent));
    disp.setSegments(digits);
}

// //function for showing strings like HH 1 etc on displays
// void displayMode(TM1637Display &disp, String mode) {
//     uint8_t modeSegments[4];
//     uint8_t seg_H = 0b01110110; //H
//     uint8_t seg_L = 0b00111000; //L
//     uint8_t seg_1 = 0b00110000;  //1
//     uint8_t seg_2 = 0x5B;   //2
//     uint8_t seg_space = 0x00; //space

//     if (mode == "HL-1") { modeSegments[0] = seg_H; modeSegments[1] = seg_L; modeSegments[2] = seg_space; modeSegments[3] = seg_1; }
//     else if (mode == "LL-1") { modeSegments[0] = seg_L; modeSegments[1] = seg_L; modeSegments[2] = seg_space; modeSegments[3] = seg_1; }
//     else if (mode == "HL-2") { modeSegments[0] = seg_H; modeSegments[1] = seg_L; modeSegments[2] = seg_space; modeSegments[3] = seg_2; }
//     else if (mode == "LL-2") { modeSegments[0] = seg_L; modeSegments[1] = seg_L; modeSegments[2] = seg_space; modeSegments[3] = seg_2; }
    
//     disp.setSegments(modeSegments);
// }

void updateValue(bool increment, unsigned long holdTime) {
    float &currentValue = floatValues[currentModeIndex];
    int &exponent = exponents[currentModeIndex];

    if (holdTime < 3000) { // Change after decimal point
        currentValue += (increment ? 0.1 : -0.1);
    } else if (holdTime < 5000) { // Change before decimal point
        currentValue = (increment ? currentValue + 1.0 : currentValue - 1.0);
    } else {
        exponent += (increment ? 1 : -1); // Change exponent
    }

    // Normalize the value between 1.0 and 9.9, adjust exponent
    if (currentValue >= 10.0) { currentValue /= 10.0; exponent++; }
    while (currentValue < 1.0 && exponent > -3) { currentValue *= 10.0; exponent--; }

    // Clamp the exponent and currentValue
    if (exponent > 3) { exponent = 3; currentValue = 9.9; }
    if (exponent < -3) { exponent = -3; currentValue = 1.0; }
    if (currentValue > 9.9) { currentValue = 9.9; }
    if (currentValue < 1.0) { currentValue = 1.0; }

    // Ensure final value does not exceed 1000
    float finalValue = currentValue * pow(10, exponent);
    if (finalValue > 1000.0) {
        finalValue = 1000.0;
        exponent = 0;

        // Recalculate currentValue and exponent from 1000.0
        while (finalValue >= 10.0 && exponent < 3) {
            finalValue /= 10.0;
            exponent++;
        }
        currentValue = finalValue;
    }
}


// Function to display mBar unit
void displayMBar(TM1637Display &disp) {
    uint8_t segments[4];
    // First half of 'm'
    segments[0] = 0b01010100;  // First half of 'm' 
    // Second half of 'm'
    segments[1] = 0b01010100;  // Second half of 'm'
    // 'b'
    segments[2] = 0b01111100;  // b
    // 'A'
    segments[3] = 0b01110111;  // A
    disp.setSegments(segments);
}

// Function to display Torr unit
void displayTorr(TM1637Display &disp) {
    uint8_t segments[4];
    segments[0] = 0b01111000;  // t
    segments[1] = 0b00111111;  // o
    segments[2] = 0b01010000;  // r
    segments[3] = 0b01010000;  // r
    disp.setSegments(segments);
}

// Function to display Pa unit
void displayPa(TM1637Display &disp) {
    uint8_t segments[4];
    segments[0] = 0b01110011;  // P
    segments[1] = 0b01110111;  // A
    segments[2] = 0x00;        // blank
    segments[3] = 0x00;        // blank
    disp.setSegments(segments);
}

// Function to display unit type based on enum value
void displayUnitType(TM1637Display &disp, UnitType type) {
    switch (type) {
        case UNIT_MBA:
            displayMBar(disp);
            break;
        case UNIT_TORR:
            displayTorr(disp);
            break;
        case UNIT_PA:
            displayPa(disp);
            break;
    }
}

// // Function to save unit settings to flash memory
// void saveUnitSettings() {
//     preferences.begin("unit_settings", false);
//     preferences.putInt("unit1", (int)unitType1);
//     preferences.putInt("unit2", (int)unitType2);
//     preferences.end();
    
//     // Serial.print("Saved unit settings: Display1=");
//     // Serial.print(unitType1);
//     // Serial.print(", Display2=");
//     // Serial.println(unitType2);
// }




