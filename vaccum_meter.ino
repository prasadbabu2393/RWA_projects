#include <Wire.h>
#include <Adafruit_ADS1X15.h>  // Supports the ADS1115
#include <LiquidCrystal_I2C.h>
#include <math.h>              // For round()

// Create an ADS1115 object (default I2C address is 0x48)
Adafruit_ADS1115 ads;

// Create an LCD object (adjust I2C address if necessary)
LiquidCrystal_I2C lcd(0x27, 16, 2);

// -------------------------------------------------
// Linear model coefficients (double precision)
// -------------------------------------------------
// Scale 1 (Vacuum 0.001 to 0.1):
const double slope1     = 2.5708e-04;
const double intercept1 = -5.2591e-03;

// Scale 2 (Vacuum 0.1 to 1):
const double slope2     = 1.7328e-03;
const double intercept2 = -6.5332e-01;

// Scale 3 (Vacuum 10 to 100):
const double slope3     = 1.1250e+00;
const double intercept3 = -9.1250e+02;

// Scale 4 (Vacuum 100 to 1000):
const double slope4     = 1.2500e+01;
const double intercept4 = -1.1150e+04;

// -------------------------------------------------
// Voltage thresholds for selecting the model (in mV)
// (These are sample threshold values – adjust based on your calibration.)
// For example:
//   Model 1: voltage < THRESHOLD1
//   Model 2: THRESHOLD1 <= voltage < THRESHOLD2
//   Model 3: THRESHOLD2 <= voltage < THRESHOLD3
//   Model 4: voltage >= THRESHOLD3
const double THRESHOLD1 = 435.0;  // mV
const double THRESHOLD2 = 820.0;  // mV
const double THRESHOLD3 = 900.0;  // mV

// -------------------------------------------------
// Utility function to round to the nearest valid step
// -------------------------------------------------
double roundToStep(double value, double step) {
  return step * round(value / step);
}

void setup() {
  Serial.begin(115200);
  // Initialize I2C with specific pins for ESP32 (change if needed)
  Wire.begin(21, 22);

  // Initialize the ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    while (1);
  }

  // Optionally, set the gain (e.g., ads.setGain(GAIN_ONE);)
  ads.setGain(GAIN_ONE);
  // For GAIN_ONE, full-scale range is ±4.096V.

  // Initialize the LCD
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Vacuum Monitor");
  delay(2000);
}

void loop() {
  // ---------------------------
  // 1. Read the voltage from the ADS1115
  // ---------------------------
  int16_t adcReading = ads.readADC_SingleEnded(0);
  int16_t adcReading2 = ads.readADC_SingleEnded(1);
  //int16_t adcReading = 3488;
  double conversionFactor = 0.125; // mV per count (for ADS1115 GAIN_ONE)
  double voltage = adcReading * conversionFactor; // in mV
   //voltage = ads.computeVolts(adcReading); // in mV

  Serial.print("ADC reading: ");
  Serial.print(adcReading);
  Serial.print(" -> Voltage: ");
  Serial.print(voltage, 2);
  Serial.println(" mV");

  // ---------------------------
  // 2. Select the appropriate linear model based on voltage
  //    and compute the vacuum value
  // ---------------------------
  double vacuum = 0.0;
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
  if(vacuum>1000){
    vacuum=1000;
  }
  else if(vacuum<0.0005){
    vacuum=0;
  }

  Serial.print("Computed Vacuum (rounded): ");
  Serial.println(vacuum, 6);

  // ---------------------------
  // 4. Display the values on the LCD
  // ---------------------------
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Volt: ");
  lcd.print(voltage, 2);
  lcd.print(" mV");

  lcd.setCursor(0, 1);
  lcd.print("Vac:");
  lcd.print(vacuum, 3);
  lcd.print(" mBar");

  // lcd.setCursor(8, 1);
  // lcd.print("A:");
  // lcd.print(adcReading, 3);
  // if (voltage < THRESHOLD1) {
  //   lcd.print("M1");
  // } else if (voltage < THRESHOLD2) {
  //   lcd.print("M2");
  // } else if (voltage < THRESHOLD3) {
  //   lcd.print("M3");
  // } else {
  //   lcd.print("M4");
  // }

  delay(1000);
}