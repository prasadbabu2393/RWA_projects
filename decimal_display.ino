#include <TM1637Display.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>  // Supports the ADS1115

// // New variables for unit selection
// bool unitSelectionMode = false;
// enum UnitType { UNIT_MBA, UNIT_TORR, UNIT_PA };
// UnitType unitType1 = UNIT_MBA; // Default unit for display1
// UnitType unitType2 = UNIT_MBA; // Default unit for display2
//Replace the unit variables section with this:

// New variables for unit selection - combined unit for both displays
bool unitSelectionMode = false;
enum UnitType { UNIT_MBA, UNIT_TORR, UNIT_PA };
 UnitType currentUnit = UNIT_MBA; // Default unit for both displays

// Create an ADS1115 object (default I2C address is 0x48)
Adafruit_ADS1115 ads;

#define RE1 12
#define CLK1 18
#define DIO1 19
#define CLK2 32
#define DIO2 33
#define BTN_INC  27
#define BTN_DEC  14
//#define BTN_TOGGLE  15
#define BTN_TOGGLE  34 //for 1st pcb only
#define BTN_OK  13
#define relay1 4
#define relay2 2
#define DAC_1 25
#define DAC_2 26

// Define UART2 pins explicitly
#define UART2_RX 16
#define UART2_TX 17

TM1637Display display1(CLK1, DIO1);
TM1637Display display2(CLK2, DIO2);
Preferences preferences;
WebServer server(80);

#define NUM_SAMPLES 20

float voltage1_buffer[NUM_SAMPLES] = {0};
float voltage2_buffer[NUM_SAMPLES] = {0};
float voltage3_buffer[NUM_SAMPLES] = {0};

int buffer_index = 0;


// WiFi credentials for AP mode
String ssid = "Vacuum_meter#01";
String password = "12345678";
String wifi_ssid = "";
String wifi_password = "";

// UART settings - fixed to UART2 only
int uart_baud = 115200; // Default baud rate
int uart_data_bits = 8; // Default data bits
int uart_parity = 0; // Default parity (0 = none, 1 = odd, 2 = even)
int uart_stop_bits = 1; // Default stop bits
int modbus_slave_id = 1; // Default Modbus slave ID

// const char* modes[] = {"HL-1", "LL-1", "HL-2", "LL-2"};
// int currentModeIndex = -1;
// float floatValues[4] = {1.0e-3, 1.0e-3, 1.0e-3, 1.0e-3};
// int exponents[4] = {-3, -3, -3, -3};

float HL1, HL2, LL1, LL2;
float vacuum1 = 0.001; // Now in decimal format instead of exponential
float vacuum2 = 0.02; // Now in decimal format instead of exponential
float dac_output_1 = 0, dac_output_2 = 0;
float temperature_1 = 0, temperature_2 = 0;

// Modbus registers
uint16_t modbusRegisters[4]; // Two 16-bit registers for each float value

unsigned long btnPressStart = 0;
bool btnHeld = false;
unsigned long lastInteractionTime = 0;
bool settingsMode = false;

 //variables for adc readings 
int16_t adcReading_1, adcReading_2, adcReading_3;
double voltage_1, voltage_2, voltage_3, voltage_diff; 
double conversionFactor = 0.125; // mV per count (for ADS1115 GAIN_ONE)

// Hardware Serial instance for UART2
HardwareSerial &uart2 = Serial2;

// Structure to hold scientific notation components
struct ScientificNotation {
    float mantissa;
    int exponent;
};

// Union to convert between float and 32-bit integer for IEEE 754 format
union FloatToBytes {
    float f;
    uint32_t u32;
    uint16_t u16[2]; // To split into two 16-bit words
};

/////////////////////////////////

// Lookup table of voltage-vacuum pairs from the Excel data
// Format: {voltage (float), vacuum (float)}
const struct {
  float voltage;
  float vacuum;
} voltageToVacuumTable[] = {
  {21.000f, 0.001000f},
  {23.625f, 0.002000f},  // Missing value filled with average
  {25.250f, 0.003000f},
  {26.550f, 0.004000f},
  {28.120f, 0.005000f},
  {30.000f, 0.006000f},
  {36.130f, 0.007000f},
  {39.880f, 0.008000f},
  {40.500f, 0.009000f},
  {44.200f, 0.010000f},
  {48.380f, 0.011000f},
  {52.500f, 0.012000f},
  {57.000f, 0.013000f},
  {62.130f, 0.014000f},
  {65.000f, 0.015000f},
  {69.000f, 0.016000f},  // Missing value filled with average
  {73.000f, 0.017000f},
  {76.500f, 0.018000f},  // Missing value filled with average
  {79.750f, 0.019000f},  // Missing value filled with average
 // {80.000f, 0.480000f},
  {82.000f, 0.020000f},
  {86.125f, 0.021000f},  // Missing value filled with average
  {90.250f, 0.022000f},
  {93.000f, 0.023000f},
  {96.500f, 0.024000f},
  {101.200f, 0.025000f},
  {103.500f, 0.026000f},
  {106.000f, 0.027000f},
  {110.620f, 0.028000f},
  {113.000f, 0.029000f},
  {115.000f, 0.030000f},
  {118.000f, 0.031000f},
  {123.500f, 0.032000f},
  {127.000f, 0.033000f},
  {130.000f, 0.034000f},
  {135.800f, 0.035000f},
  {140.000f, 0.036000f},
  {147.000f, 0.037000f},
  {150.500f, 0.038000f},
  {153.800f, 0.039000f},
  {159.500f, 0.040000f},
  {163.500f, 0.041000f},
  {167.000f, 0.042000f},
  {170.000f, 0.043000f},
  {174.500f, 0.044000f},
  {181.000f, 0.045000f},
  {184.500f, 0.046000f},
  {188.000f, 0.047000f},
  {192.500f, 0.048000f},
  {200.250f, 0.049000f},
  {204.000f, 0.050000f},
  {207.000f, 0.051000f},  // Missing value filled with average
  {210.000f, 0.052000f},
  {215.000f, 0.053000f},
  {218.000f, 0.054000f},
  {221.000f, 0.055000f},
  {224.000f, 0.056000f},
  {227.000f, 0.057000f},
  {231.000f, 0.058000f},
  {235.000f, 0.059000f},
  {239.000f, 0.060000f},
  {243.750f, 0.061000f},  // Missing value filled with average
  {248.500f, 0.062000f},
  {251.200f, 0.063000f},    
  {255.000f, 0.064000f},
  {259.000f, 0.065000f},
  {266.500f, 0.066000f},
  {268.500f, 0.067000f},
  {271.000f, 0.068000f},
  {274.750f, 0.069000f},  // Missing value filled with average
  {278.500f, 0.070000f},
  {281.200f, 0.071000f},
  {284.500f, 0.072000f},
  {288.750f, 0.073000f},
  {293.500f, 0.074000f},
  {297.000f, 0.075000f},
  {301.500f, 0.076000f},
  {305.000f, 0.077000f},
  {307.500f, 0.078000f},
  {312.000f, 0.079000f},
  {315.000f, 0.080000f},
  {318.000f, 0.081000f},
  {323.500f, 0.082000f},
  {325.500f, 0.083000f},  // Missing value filled with average
  {327.500f, 0.084000f},
  {332.500f, 0.085000f},
  {335.000f, 0.086000f},
  {337.000f, 0.087000f},
  {339.000f, 0.088000f},
  {343.500f, 0.089000f},
  {347.000f, 0.090000f},
  {350.000f, 0.091000f},
  {352.000f, 0.092000f},
  {354.850f, 0.093000f},  // Missing value filled with average
  {357.025f, 0.094000f},  // Missing value filled with average
  {365.200f, 0.095000f},
  {367.500f, 0.096000f},
  {371.500f, 0.097000f},
  {375.000f, 0.098000f},
  {378.000f, 0.099000f},
  {382.000f, 0.100000f},
  {388.500f, 0.110000f},
  {419.000f, 0.120000f},
  {443.000f, 0.130000f}, 
  {460.000f, 0.140000f},
  {488.000f, 0.150000f},
  {501.000f, 0.160000f},
  {520.000f, 0.170000f},
  {540.000f, 0.180000f},
  {560.000f, 0.190000f},
  {585.000f, 0.200000f},
  {605.000f, 0.210000f},
  {620.000f, 0.220000f},
  {636.000f, 0.230000f},
  {650.000f, 0.240000f},
  {670.000f, 0.250000f},
  {672.000f, 0.260000f},
  {690.000f, 0.270000f},
  {700.000f, 0.280000f},
  {712.000f, 0.290000f},
  {725.000f, 0.300000f},
  {738.000f, 0.310000f},
  {746.000f, 0.320000f},
  {750.000f, 0.330000f},
  {755.000f, 0.340000f},
  {762.000f, 0.350000f},
  {765.000f, 0.360000f},
  {772.000f, 0.370000f},
  {779.000f, 0.380000f},
  {785.000f, 0.390000f},
  {792.000f, 0.400000f},
  {795.000f, 0.410000f},
  {798.000f, 0.420000f},
  {804.000f, 0.430000f},
  {806.000f, 0.440000f},
  {810.000f, 0.450000f},
  {813.000f, 0.460000f},
  {817.000f, 0.470000f},
  {820.000f, 0.480000f},
  {824.000f, 0.490000f},
  {829.200f, 0.500000f},
  {832.200f, 0.510000f},
  {835.000f, 0.520000f},
  {836.500f, 0.530000f},
  {838.000f, 0.540000f},
  {841.000f, 0.560000f},
  {844.500f, 0.570000f},
  {846.000f, 0.580000f},
  {848.500f, 0.590000f},  // Missing value filled with average
  {851.000f, 0.600000f},
  {853.500f, 0.610000f},
  {855.000f, 0.620000f},
  {859.000f, 0.630000f},
  {861.000f, 0.640000f},
  {862.500f, 0.650000f},
  {866.000f, 0.660000f},
  {868.100f, 0.670000f},
  {870.000f, 0.680000f},
  {872.100f, 0.690000f},
  {875.000f, 0.700000f},
  {876.500f, 0.710000f},
  {878.000f, 0.720000f},
  {878.500f, 0.730000f},  // Missing value filled with average
  {879.500f, 0.740000f},
  {881.000f, 0.750000f},
  {883.000f, 0.760000f},
  {885.000f, 0.770000f},
  {887.000f, 0.780000f},
  {888.500f, 0.790000f},
  {889.500f, 0.800000f},
  {889.750f, 0.810000f},  // Missing value filled with average
  {890.000f, 0.820000f},
  {891.000f, 0.830000f},
  {892.750f, 0.840000f},
  {893.000f, 0.850000f},
  {894.000f, 0.860000f},
  {895.000f, 0.870000f},
  {896.500f, 0.880000f},
  {897.250f, 0.890000f},
  {898.500f, 0.900000f},
  {899.100f, 0.910000f},
  {899.300f, 0.920000f},
  {900.000f, 0.930000f},
  {901.300f, 0.940000f},
  {902.300f, 0.950000f},
  {903.500f, 0.960000f},
  {904.500f, 0.970000f},
  {905.500f, 0.980000f},
  {905.750f, 0.990000f},
  {906.000f, 1.000000f},
  {914.000f, 2.000000f},
  {920.000f, 3.000000f},
  {931.500f, 4.000000f},
  {941.000f, 5.000000f},
  {949.000f, 6.000000f},
  {951.000f, 7.000000f},
  {952.100f, 8.000000f},
  {953.000f, 9.000000f},
  {954.000f, 10.000000f},
  {955.000f, 11.000000f},
  {956.000f, 12.000000f},
  {956.600f, 13.000000f},
  {957.500f, 14.000000f},
  {958.120f, 15.000000f},
  {958.500f, 16.000000f},
  {959.000f, 17.000000f},
  {959.000f, 18.000000f},
  {959.375f, 19.000000f},  // Missing value filled with average
  {959.750f, 20.000000f},
  {960.275f, 30.000000f},  // Missing value filled with average
  {960.800f, 40.000000f},
  {962.500f, 50.000000f},
  {963.000f, 60.000000f},
  {963.500f, 70.000000f},
  {964.000f, 80.000000f},
  {964.000f, 1000.000000f}  // For extreme case, using last available value
};

// Number of entries in the lookup table
const int TABLE_SIZE = sizeof(voltageToVacuumTable) / sizeof(voltageToVacuumTable[0]);

/**
 * Get the vacuum reading based on voltage input
 * 
 * @param voltage The voltage reading from the sensor (as float)
 * @return The corresponding vacuum value
 */
float getVacuumFromVoltage(float voltage) {
  // If voltage is less than the minimum in the table, return the minimum vacuum value
  if (voltage <= voltageToVacuumTable[0].voltage) {
    return voltageToVacuumTable[0].vacuum;
  }
  
  // If voltage is greater than the maximum in the table, return the maximum vacuum value
  if (voltage >= voltageToVacuumTable[TABLE_SIZE - 1].voltage) {
    return voltageToVacuumTable[TABLE_SIZE - 1].vacuum;
  }
  
  // Find the closest voltage value in the table
  int closestIndex = 0;
  float minDifference = abs(voltage - voltageToVacuumTable[0].voltage);
  
  for (int i = 1; i < TABLE_SIZE; i++) {
    float difference = abs(voltage - voltageToVacuumTable[i].voltage);
    if (difference < minDifference) {
      minDifference = difference;
      closestIndex = i;
    }
  }
  
  // Return the vacuum reading for the closest voltage
  return voltageToVacuumTable[closestIndex].vacuum;
}

////////////////////////////////

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

// Update the modes array to remove separate Unit-1 and Unit-2 modes
const char* modes[] = {"HL-1", "LL-1", "HL-2", "LL-2", "Unit"};
int currentModeIndex = -1;
float floatValues[4] = {1.0e-3, 1.0e-3, 1.0e-3, 1.0e-3};
int exponents[4] = {-3, -3, -3, -3};

// Button debounce variables
unsigned long lastButtonPressTime = 0;
const unsigned long debounceDelay = 200; // 200ms debounce time

// Update the displayMode function to handle the new combined Unit mode
void displayMode(TM1637Display &disp, String mode) {
    uint8_t modeSegments[4];
    uint8_t seg_H = 0b01110110; //H
    uint8_t seg_L = 0b00111000; //L
    uint8_t seg_1 = 0b00110000;  //1
    uint8_t seg_2 = 0x5B;   //2
    uint8_t seg_U = 0b00111110; //U
    uint8_t seg_n = 0b01010100; //n
    uint8_t seg_t = 0b01111000; //t
    uint8_t seg_space = 0x00; //space

    if (mode == "HL-1") { modeSegments[0] = seg_H; modeSegments[1] = seg_L; modeSegments[2] = seg_space; modeSegments[3] = seg_1; }
    else if (mode == "LL-1") { modeSegments[0] = seg_L; modeSegments[1] = seg_L; modeSegments[2] = seg_space; modeSegments[3] = seg_1; }
    else if (mode == "HL-2") { modeSegments[0] = seg_H; modeSegments[1] = seg_L; modeSegments[2] = seg_space; modeSegments[3] = seg_2; }
    else if (mode == "LL-2") { modeSegments[0] = seg_L; modeSegments[1] = seg_L; modeSegments[2] = seg_space; modeSegments[3] = seg_2; }
    else if (mode == "Unit") { modeSegments[0] = seg_U; modeSegments[1] = seg_n; modeSegments[2] = seg_t; modeSegments[3] = seg_space; }
    
    disp.setSegments(modeSegments);
}


void setup() {
    Serial.begin(115200);
  // Initialize I2C with specific pins for ESP32 (change if needed)
  Wire.begin(21, 22);

  // Initialize the ADS1115
  if (!ads.begin()) {
    Serial.println("Failed to initialize ADS.");
    //while (1);
  }
  else{
      ads.setGain(GAIN_ONE); // +/- 4.096V (1 bit = 125uV)
     Serial.println("ADS. initialized ");
  }

    display1.setBrightness(7);
    display2.setBrightness(7); 

    pinMode(RE1, OUTPUT); //rs485 pin
    pinMode(BTN_INC, INPUT_PULLUP);
    pinMode(BTN_DEC, INPUT_PULLUP); 
    pinMode(BTN_TOGGLE, INPUT_PULLUP);
    pinMode(BTN_OK, INPUT_PULLUP);

   // delay(1000);
    pinMode(relay1, OUTPUT);
    digitalWrite(relay1, LOW);
    pinMode(relay2, OUTPUT);
    digitalWrite(relay2, LOW);
    
    // Initially set RE1 to LOW for receive mode
    digitalWrite(RE1, LOW);

    preferences.begin("unit_settings", false);
    // Load saved unit settings
    loadUnitSettings();
    preferences.end();

    // Load vacuum settings
    preferences.begin("settings", false);
    ssid = preferences.getString("ssid", ssid);       // Load saved SSID
    password = preferences.getString("password", password); // Load saved password
    Serial.println("ssid");
    Serial.println(ssid);
    Serial.println(password);

    for (int i = 0; i < 4; i++) {
        floatValues[i] = preferences.getFloat(modes[i], 1.0);
        exponents[i] = preferences.getInt((String(modes[i]) + "_exp").c_str(), -3);
    }
    preferences.end();
    
    // Combine floatValues and exponents into 4 different float variables
    HL1 = floatValues[0] * pow(10, exponents[0]);
    LL1 = floatValues[1] * pow(10, exponents[1]);
    HL2 = floatValues[2] * pow(10, exponents[2]);
    LL2 = floatValues[3] * pow(10, exponents[3]);

    // Print values to Serial Monitor
    Serial.print("HL-1: "); Serial.println(HL1, 6);
    Serial.print("LL-1: "); Serial.println(LL1, 6);
    Serial.print("HL-2: "); Serial.println(HL2, 6);
    Serial.print("LL-2: "); Serial.println(LL2, 6);

    // Load and initialize UART settings
    loadSettings();
    //uart2.begin(115200, SERIAL_8N1, UART2_RX, UART2_TX);

    delay(500);
    // // Set up WiFi Access Point
    // WiFi.softAP(ssid, password);
    // IPAddress IP = WiFi.softAPIP(); 
    // Serial.print("AP IP address: ");
    // Serial.println(IP);
    
    // // Set up web server routes
    // server.on("/", handleRoot);
    // server.on("/uart", HTTP_GET, handleLogin);
    // server.on("/save", HTTP_POST, handleSave);
    // server.on("/temp", handleTemperature);
    // server.on("/admin", HTTP_GET, handleSSIDPage);
    // server.on("/save_wifi", HTTP_POST, handleSaveSSID);

    // // Start server
    // server.begin();
    // Serial.println("HTTP server started");
    
    // Initialize modbus registers with current values
    updateModbusRegisters();
    
   Serial.println("Modbus RTU Slave started with ID: " + String(modbus_slave_id));
}

  
// void loop() 
// {
//     //taking the ads readings from ADS1115 sensor
//     adcReadings_fun();
 
//     // // Handle web server clients
//     // server.handleClient();
    
//     // Read data from UART
//     if (uart2.available()) {
//         Serial.println("receiving");
//         uint8_t buffer[128]; // Buffer for Modbus frame
//         int bytesRead = 0;
        
//         // Read data until timeout
//         unsigned long startTime = millis();
//         while (millis() - startTime < 100) { // 100ms timeout
//             if (uart2.available()) {
//                 if (bytesRead < sizeof(buffer)) {
//                     buffer[bytesRead++] = uart2.read();
//                     startTime = millis(); // Reset timeout
//                 } else {
//                     // Buffer overflow, discard remaining
//                     while (uart2.available()) uart2.read();
//                     break;
//                 }
//             }
//             yield(); // Allow other tasks to run
//         }
        
//         if (bytesRead > 0) {
//             Serial.print("Received Modbus frame: ");
//             for (int i = 0; i < bytesRead; i++) {
//                 Serial.print(buffer[i], HEX);
//                 Serial.print(" ");
//             }
//             Serial.println();
            
//             // Process the Modbus request
//             processModbusRequest(buffer, bytesRead);
//         }
//     }
    
//     if (isButtonPressed(BTN_TOGGLE)) { //menu button
//         // If we're in unit selection mode, exit it first
//         if (unitSelectionMode) {
//             unitSelectionMode = false;
//             saveUnitSettings(); // Save current unit settings
//         } else {
//             currentModeIndex = (currentModeIndex + 1) % 5;
//             if (currentModeIndex == 4) {
//                 settingsMode = false;
//                 currentModeIndex = -1;
//             } else {
//                 settingsMode = true;
//             }
//         }
//         lastInteractionTime = millis();
//         Serial.println("currentModeIndex: " + String(currentModeIndex));
//         delay(200);
//     }
    
//     // Handle OK button press in non-settings mode
//     if (!settingsMode && !unitSelectionMode && isButtonPressed(BTN_OK)) {
//         unitSelectionMode = true;
//         lastInteractionTime = millis();
//         Serial.println("Entering unit selection mode");
//         delay(200);
//     }
    
//    // Handle OK button press in unit selection mode
//     if (unitSelectionMode && isButtonPressed(BTN_OK)) {
//         unitSelectionMode = false;
//         saveUnitSettings(); // Save unit settings
//         Serial.println("Exiting unit selection mode and saving settings");
//         lastInteractionTime = millis();
//         delay(200);
//     }
    
//     // Handle buttons in settings mode
//     if (settingsMode) {
//         if (isButtonPressed(BTN_INC) || isButtonPressed(BTN_DEC)) {
//             if (!btnHeld) {
//                 btnPressStart = millis();
//                 btnHeld = true;
//             }
//             unsigned long holdTime = millis() - btnPressStart;
//             updateValue(isButtonPressed(BTN_INC), holdTime);
//             lastInteractionTime = millis();
//             delay(200);
//         } else {
//             btnHeld = false;
//         }

//         if (isButtonPressed(BTN_OK)) {
//             preferences.begin("settings", false);
//             preferences.putFloat(modes[currentModeIndex], floatValues[currentModeIndex]);
//             preferences.putInt((String(modes[currentModeIndex]) + "_exp").c_str(), exponents[currentModeIndex]);
//             preferences.end();
//             // After updating the value
//             if (currentModeIndex >= 0 && currentModeIndex < 4) {
//                // Update the threshold variable based on the currentModeIndex
//                switch (currentModeIndex) {
//                    case 0: // HL-1
//                        HL1 = floatValues[0] * pow(10, exponents[0]);
//                        break;
//                    case 1: // LL-1
//                        LL1 = floatValues[1] * pow(10, exponents[1]);
//                        break;
//                    case 2: // HL-2
//                        HL2 = floatValues[2] * pow(10, exponents[2]);
//                        break;
//                    case 3: // LL-2
//                        LL2 = floatValues[3] * pow(10, exponents[3]);
//                        break;
//                } 
//            }
    
//             lastInteractionTime = millis();
//             delay(200);
//         }

//         if (millis() - lastInteractionTime > 25000) {
//             settingsMode = false;
//         }
//     }
    
//     // Handle buttons in unit selection mode
//     if (unitSelectionMode) {
//         // Handle INC button for display1
//         if (isButtonPressed(BTN_INC)) {
//             unitType1 = (UnitType)(((int)unitType1 + 1) % 3); // Cycle through mBar, Torr, Pa
//             lastInteractionTime = millis();
//             Serial.print("Display1 unit changed to: ");
//             Serial.println(unitType1);
//             delay(200);
//         }
        
//         // Handle DEC button for display2
//         if (isButtonPressed(BTN_DEC)) {
//             unitType2 = (UnitType)(((int)unitType2 + 1) % 3); // Cycle through mBar, Torr, Pa
//             lastInteractionTime = millis();
//             Serial.print("Display2 unit changed to: ");
//             Serial.println(unitType2);
//             delay(200);
//         }
        
//         // Timeout for unit selection mode
//         if (millis() - lastInteractionTime > 25000) {
//             unitSelectionMode = false;
//             saveUnitSettings(); // Save on timeout
//         }
//     }

//     // Display handling
//    if (unitSelectionMode) {
//         // Display the currently selected units
//         displayUnitType(display1, unitType1);
//         displayUnitType(display2, unitType2);
//    } 
//    else if (!settingsMode) {
//         // If not in settings or unit selection mode, show vacuum readings or unit displays
//         static bool showingUnits = false;
//         static unsigned long unitDisplayStartTime = 0;
        
//         //If BTN_OK was just pressed (showing units), show the unit labels for 3 seconds
//         if (isButtonPressed(BTN_OK) && !showingUnits) {
//             showingUnits = true;
//             unitDisplayStartTime = millis();
//         }
        
//         if (showingUnits && millis() - unitDisplayStartTime < 3000) {
//             // Show just the units for 3 seconds
//             displayUnitType(display1, unitType1);
//             displayUnitType(display2, unitType2);
//         } else {
//             showingUnits = false;
//             // Show regular vacuum readings
//             ScientificNotation vac1 = toScientific(vacuum1);
//            // ScientificNotation vac2 = toScientific(vacuum2);
//             showScientific(display2, vac1.mantissa, vac1.exponent);
//            // showScientific(display1, vac2.mantissa, vac2.exponent);
//            displayUnitType(display1, unitType2);
//         }
//     } else {
//         //Settings mode display
//         showScientific(display1, floatValues[currentModeIndex], exponents[currentModeIndex]);
//         displayMode(display2, modes[currentModeIndex]);
//     }

//     // Handle relay control based on vacuum thresholds
//     if (vacuum1 < LL1) {
//       if(digitalRead(relay1) != HIGH) {
//         Serial.println("LL1-on");
//         digitalWrite(relay1, HIGH); // relay1 will ON
//       }
//     } 
//     else
//     {
//       if (vacuum1 > HL1) {
//        if(digitalRead(relay1) != LOW) {
//           Serial.println("HL1-off");
//          digitalWrite(relay1, LOW);
//         }
//       }
//     }


//     if (vacuum2 < LL2) {
//       if(digitalRead(relay2) != HIGH) {
//         Serial.println("LL2-on");
//         digitalWrite(relay2, HIGH); // relay2 will ON
//       }
//     } 
//     else
//     {
//       if (vacuum2 > HL2) { 
//         if(digitalRead(relay2) != LOW) {
//           Serial.println("HL2-off");
//           digitalWrite(relay2, LOW); 
//        }
//       }
//     }
//     delay(100);
// }


// Add conversion factor constants for different units
const float MBAR_TO_TORR = 0.750062;  // 1 mBar = 0.750062 Torr 
const float MBAR_TO_PA = 100.0;       // 1 mBar = 100 Pa

// // Function to convert vacuum reading to selected units
// float convertToSelectedUnit(float vacuumInMbar, UnitType unitType) {
//     switch (unitType) {
//         case UNIT_TORR:
//             return vacuumInMbar * MBAR_TO_TORR;
//         case UNIT_PA:
//             return vacuumInMbar * MBAR_TO_PA;
//         case UNIT_MBA:
//         default:
//             return vacuumInMbar;  // Already in mBar
//     }
// }

// Replace the unit settings functions with these:
// Function to load unit settings from flash memory
void loadUnitSettings() {
    preferences.begin("unit_settings", false);
    currentUnit = (UnitType)preferences.getInt("unit", UNIT_MBA);
    preferences.end();
    
    Serial.print("Loaded unit setting: ");
    Serial.println(currentUnit);
}

// Function to save unit settings to flash memory
void saveUnitSettings() {
    preferences.begin("unit_settings", false);
    preferences.putInt("unit", currentUnit);
    preferences.end();
    
    Serial.print("Saved unit setting: ");
    Serial.println(currentUnit);
}


// Update the convertToSelectedUnit function to use combined unit
float convertToSelectedUnit(float vacuumInMbar, UnitType unitType) {
        switch (unitType) {
            case UNIT_TORR:
                return vacuumInMbar * MBAR_TO_TORR;
            case UNIT_PA:
                return vacuumInMbar * MBAR_TO_PA;
            case UNIT_MBA:
            default:
                return vacuumInMbar;  // Already in mBar
        }
}

void loop() 
{
 
    // //Handle web server clients and UART code remains the same
    // server.handleClient();

     // Read data from UART
    if (uart2.available())
    {
        Serial.println("receiving");
        uint8_t buffer[128]; // Buffer for Modbus frame
        int bytesRead = 0;
        
        // Read data until timeout
        unsigned long startTime = millis();
        while (millis() - startTime < 100) { // 100ms timeout
            if (uart2.available()) {
                if (bytesRead < sizeof(buffer)) {
                    buffer[bytesRead++] = uart2.read();
                    startTime = millis(); // Reset timeout
                } else {
                    // Buffer overflow, discard remaining
                    while (uart2.available()) uart2.read();
                    break;
                }
            }
            yield(); // Allow other tasks to run
        }
        
        if (bytesRead > 0) {
            Serial.print("Received Modbus frame: ");
            for (int i = 0; i < bytesRead; i++) {
                Serial.print(buffer[i], HEX);
                Serial.print(" ");
            }
            Serial.println();
            
            // Process the Modbus request
            processModbusRequest(buffer, bytesRead);
        }
    }
    // else
    // {
      // Taking the ads readings from ADS1115 sensor
        adcReadings_fun();
    // }
    
    // Get current time for button debouncing
    unsigned long currentTime = millis();
    
    // TOGGLE button (menu button) - cycle through settings
    if (isButtonPressed(BTN_TOGGLE) && currentTime - lastButtonPressTime > debounceDelay) {
        lastButtonPressTime = currentTime;
        currentModeIndex = (currentModeIndex + 1) % 6; // Cycle through 5 modes + normal mode
        if (currentModeIndex == 5) {
            settingsMode = false;
            currentModeIndex = -1;
        } else {
            settingsMode = true;
        }
        lastInteractionTime = currentTime;
        Serial.println("currentModeIndex: " + String(currentModeIndex));
        delay(100);
    }
    
    // Handle buttons in settings mode
    if (settingsMode) 
    {
        // Handle increment/decrement buttons
        if ((isButtonPressed(BTN_INC) || isButtonPressed(BTN_DEC)) && currentTime - lastButtonPressTime > debounceDelay) {
            lastButtonPressTime = currentTime;
            
            if (!btnHeld) {
                btnPressStart = currentTime;
                btnHeld = true;
            }
            
            unsigned long holdTime = currentTime - btnPressStart;
            
            // If in unit selection mode (index 4)
            if (currentModeIndex == 4) { // Combined Unit setting
                if (isButtonPressed(BTN_INC)) {
                    // Cycle forward through units with INC button
                    currentUnit = (UnitType)(((int)currentUnit + 1) % 3);
                } else if (isButtonPressed(BTN_DEC)) {
                    // Cycle backward through units with DEC button
                    currentUnit = (UnitType)(((int)currentUnit + 2) % 3); // +2 is same as -1 but avoids negative numbers
                }
                Serial.print("Unit changed to: ");
                Serial.println(currentUnit);
            }
            else {
                // For HL-1, LL-1, HL-2, LL-2 settings
                updateValue(isButtonPressed(BTN_INC), holdTime);
            }
            
            lastInteractionTime = currentTime;
            delay(100);
        } else {
            btnHeld = false;
        }

        // OK button - save settings
        if (isButtonPressed(BTN_OK) && currentTime - lastButtonPressTime > debounceDelay) {
            lastButtonPressTime = currentTime;
            
            // For unit setting
            if (currentModeIndex == 4) {
                saveUnitSettings();
                Serial.println("Saved unit setting");
            } 
            // For threshold settings
            else if (currentModeIndex >= 0 && currentModeIndex < 4) {
                preferences.begin("settings", false);
                preferences.putFloat(modes[currentModeIndex], floatValues[currentModeIndex]);
                preferences.putInt((String(modes[currentModeIndex]) + "_exp").c_str(), exponents[currentModeIndex]);
                preferences.end();
                
                // Update the threshold variable
                switch (currentModeIndex) {
                    case 0: // HL-1
                        HL1 = floatValues[0] * pow(10, exponents[0]);
                        break;
                    case 1: // LL-1
                        LL1 = floatValues[1] * pow(10, exponents[1]);
                        break;
                    case 2: // HL-2
                        HL2 = floatValues[2] * pow(10, exponents[2]);
                        break;
                    case 3: // LL-2
                        LL2 = floatValues[3] * pow(10, exponents[3]);
                        break;
                }
                Serial.println("Saved threshold settings");
            }
            
            lastInteractionTime = currentTime;
            delay(100);
        }

        // Auto-exit settings mode after timeout
        if (currentTime - lastInteractionTime > 25000) {
            settingsMode = false;
            currentModeIndex = -1;
        }
    }
    else // Not in settings mode
    {
        // OK button in normal mode - briefly show the units
        if (isButtonPressed(BTN_OK) && currentTime - lastButtonPressTime > debounceDelay) {
            lastButtonPressTime = currentTime;
            
            // Show units for 3 seconds
            static bool showingUnits = false;
            static unsigned long unitDisplayStartTime = 0;
            
            showingUnits = true;
            unitDisplayStartTime = currentTime;
            
            lastInteractionTime = currentTime;
            delay(100);
        }
    }

    // Display handling
    if (settingsMode) 
    {
        // For unit selection mode
        if (currentModeIndex == 4) { // Combined Unit setting
            displayUnitType(display1, currentUnit);
            displayMode(display2, modes[currentModeIndex]);
        }
        else {
            // Display HL-1, LL-1, HL-2, LL-2 settings
            showScientific(display1, floatValues[currentModeIndex], exponents[currentModeIndex]);
            displayMode(display2, modes[currentModeIndex]);
        }
    } 
    else // Normal mode
    {
        // Normal display mode - show vacuum readings
        static bool showingUnits = false;
        static unsigned long unitDisplayStartTime = 0;
        
        // If we were showing units and the time has expired
        if (showingUnits && millis() - unitDisplayStartTime >= 3000) {
            showingUnits = false;
        }
        
        if (showingUnits) {
            // Show just the units for 3 seconds on both displays
            displayUnitType(display1, currentUnit);
            displayUnitType(display2, currentUnit);
        } 
        else {
            // Show vacuum readings with units
            // Convert vacuum readings to selected unit before displaying
            float displayVacuum1 = convertToSelectedUnit(vacuum1, currentUnit);
            
            // Show vacuum reading with unit
            ScientificNotation vac1 = toScientific(displayVacuum1);
            showScientific(display2, vac1.mantissa, vac1.exponent);
            displayUnitType(display1, currentUnit);
        }
    }


    // Handle relay control - code remains the same, always using mBar values
   //     // Handle relay control based on vacuum thresholds
    if (vacuum1 < LL1) {
      if(digitalRead(relay2) != HIGH) {
        Serial.println("LL1-on");
        digitalWrite(relay2, HIGH); // relay1 will ON
      }
    } 
    else
    {
      if (vacuum1 > HL1) {
       if(digitalRead(relay2) != LOW) {
          Serial.println("HL1-off");
         digitalWrite(relay2, LOW);
        }
      }
    }
    
    delay(50); // Small delay for stability
}

    