// ESP32 Integrated System: UART + Modbus + Web Server Monitoring
// Button press -> Send UART frame -> Receive sensor data -> Send to PLC via Modbus
// Web Server for monitoring all data and error messages

#include <HardwareSerial.h>
#include <WiFi.h>
#include <WebServer.h>

// WiFi AP Configuration
const char* ssid = "Wheel_Vision_RWA";
const char* password = "12345678";

// Web Server
WebServer server(80);

// RS485 Modbus Configuration (Second UART - Serial2)
#define MAX485_DE_RE_PIN 21    // Driver Enable and Receiver Enable pin for Modbus only
#define RXD_PIN 16           // RS485 RX pin for Modbus
#define TXD_PIN 17           // RS485 TX pin for Modbus

// System Configuration  
#define ledpin 2             // LED pin
#define pcb_ledpin 5            // LED pin
#define resetPin 19          // Reset button pin
#define signalPin 32//19          // Reset button pin

// Modbus Configuration
#define SLAVE_ID 1           // PLC Slave ID
#define BAUD_RATE 19200      // Match your PLC baud rate
#define START_REGISTER_ADDRESS 0   // Starting register address (0x0000 = register 40001)

// Variables to send to PLC
float height = 0.0;          // Height value from sensor
float diameter = 0.0;        // Diameter value from sensor
uint16_t model = 0;          // Model value from sensor
uint16_t status = 0;         // Status value from sensor
uint16_t temp = 0;         // Temperature value 

// Convert float values to uint16_t for Modbus transmission (multiply by 100)
uint16_t height_modbus = 0;
uint16_t diameter_modbus = 0;

// Communication and timing variables
#define RESPONSE_TIMEOUT 1000  // milliseconds
int programState = 0, buttonState;
long buttonMillis = 0;

// UART frame for sensor communication (First UART - Serial)
byte TxData[10] = { 1, 3, 0x01, 0xF4, 0x00, 0x02 };   // slave_id, func code, start address, no. of bytes
byte rxdata[15], received_bytes;

// Web monitoring variables
String webLog = "";
String lastOperation = "System Started";
String lastError = "None";
unsigned long lastUpdateTime = 0;
int operationCount = 0;
bool lastModbusStatus = false;

// Function to add log entry to web page
void addWebLog(String message) {
  String timestamp = String(millis() / 1000);
  webLog += "[" + timestamp + "s] " + message + "<br>";
  
  // Keep only last 50 entries to prevent memory overflow
  int brCount = 0;
  int pos = webLog.length() - 1;
  while (pos >= 0 && brCount < 50) {
    if (webLog.substring(pos, pos + 4) == "<br>") {
      brCount++;
    }
    pos--;
  }
  if (brCount >= 50 && pos >= 0) {
    webLog = webLog.substring(pos + 5);
  }
}

/* CRC Tables for UART communication */
static const uint8_t table_crc_hi[] = {
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
  0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
  0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
  0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

static const uint8_t table_crc_lo[] = {
  0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
  0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
  0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
  0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
  0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
  0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
  0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
  0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
  0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
  0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
  0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
  0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
  0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
  0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
  0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
  0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
  0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
  0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
  0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
  0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
  0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
  0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
  0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
  0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
  0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
  0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

// UART CRC16 function for sensor communication
uint16_t crc16(uint8_t *buffer, uint16_t buffer_length) {
  uint8_t crc_hi = 0xFF;
  uint8_t crc_lo = 0xFF;
  unsigned int i;

  while (buffer_length--) {
    i = crc_lo ^ *buffer++;
    crc_lo = crc_hi ^ table_crc_hi[i];
    crc_hi = table_crc_lo[i];
  }

  return (crc_hi << 8 | crc_lo);
}

// Modbus CRC16 function
uint16_t calculateCRC16(uint8_t *data, uint8_t length) {
  uint16_t crc = 0xFFFF;
  
  for (uint8_t i = 0; i < length; i++) {
    crc ^= data[i];
    
    for (uint8_t j = 0; j < 8; j++) {
      if (crc & 0x0001) {
        crc >>= 1;
        crc ^= 0xA001;
      } else {
        crc >>= 1;
      }
    }
  }
  
  return crc;
}

// Float conversion function for UART data
float convertToFloat(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6) {
  uint8_t sign = Data3 & 0x80;
  uint8_t expo = (Data3 << 1) | (Data4 >> 7);
  uint32_t m = ((uint32_t)(Data4 & 0x7F) << 16) | ((uint32_t)Data5 << 8) | Data6;
  float val = 0;

  for (int p = 22; p >= 0; --p) {
    if (m & (1 << p)) {
      val += pow(2, p - 23);
    }
  }
  val += 1;
  val = val * (pow(2, expo - 127));
  if (sign) {
    val = -val;
  }
  return val;
}

// Function to control RS485 transceiver (ONLY for Modbus - Serial2)
void setTransmitMode() {
  digitalWrite(MAX485_DE_RE_PIN, HIGH);
  delayMicroseconds(100);
}

void setReceiveMode() {
  delayMicroseconds(100);
  digitalWrite(MAX485_DE_RE_PIN, LOW);
}

// Function to send Modbus frame
void sendModbusFrame(uint8_t *frame, uint8_t length) {
  setTransmitMode();
  
  for (uint8_t i = 0; i < length; i++) {
    Serial2.write(frame[i]);
  }
  
  Serial2.flush();
  setReceiveMode();
}

// Function to write multiple registers (Modbus Function Code 16)
bool writeMultipleRegisters(uint8_t slaveId, uint16_t startAddr, uint16_t *values, uint8_t count) {
  uint8_t frameLength = 9 + (count * 2);
  uint8_t frame[frameLength];
  uint16_t crc;
  
  // Build Modbus frame
  frame[0] = slaveId;
  frame[1] = 0x10;
  frame[2] = startAddr >> 8;
  frame[3] = startAddr & 0xFF;
  frame[4] = 0x00;
  frame[5] = count;
  frame[6] = count * 2;
  
  // Add register values
  for (uint8_t i = 0; i < count; i++) {
    frame[7 + (i * 2)] = values[i] >> 8;
    frame[8 + (i * 2)] = values[i] & 0xFF;
  }
  
  // Calculate CRC
  crc = calculateCRC16(frame, frameLength - 2);
  frame[frameLength - 2] = crc & 0xFF;
  frame[frameLength - 1] = crc >> 8;
  
  // Log Modbus frame to web
  String frameStr = "Modbus Frame: ";
  for (int i = 0; i < frameLength; i++) {
    if (frame[i] < 0x10) frameStr += "0";
    frameStr += String(frame[i], HEX) + " ";
  }
  addWebLog(frameStr);
  
  // Clear receive buffer
  while (Serial2.available()) {
    Serial2.read();
  }
  
  // Send the frame
  sendModbusFrame(frame, frameLength);
  
  // Wait for response
  unsigned long startTime = millis();
  uint8_t responseIndex = 0;
  uint8_t response[8];
  
  while (millis() - startTime < RESPONSE_TIMEOUT) {
    if (Serial2.available()) {
      response[responseIndex] = Serial2.read();
      responseIndex++;
      
      if (responseIndex >= 8) {
        break;
      }
    }
  }
  
  // Check response
  if (responseIndex >= 8) {
    String responseStr = "Modbus Response: ";
    for (int i = 0; i < responseIndex; i++) {
      if (response[i] < 0x10) responseStr += "0";
      responseStr += String(response[i], HEX) + " ";
    }
    addWebLog(responseStr);
    
    // Verify response
    uint16_t receivedCRC = (response[7] << 8) | response[6];
    uint16_t calculatedCRC = calculateCRC16(response, 6);
    
    if (receivedCRC == calculatedCRC && response[0] == slaveId && response[1] == 0x10) {
      addWebLog("✓ Modbus write successful!");
      lastModbusStatus = true;
      lastError = "None";
      return true;
    } else {
      addWebLog("✗ Invalid Modbus response or CRC error");
      lastModbusStatus = false;
      lastError = "Invalid Modbus response";
      return false;
    }
  } else {
    addWebLog("✗ No Modbus response received (timeout)");
    lastModbusStatus = false;
    lastError = "Modbus timeout";
    return false;
  }
}

// Function to send all 5 variables to Schneider PLC
bool sendAllVariablesToPLC() {
  height_modbus = (uint16_t)(height * 10);
  diameter_modbus = (uint16_t)(diameter * 10);
  
  uint16_t values[5] = {height_modbus, diameter_modbus, model, status, temp};
  
  addWebLog("=== Sending variables to Schneider PLC ===");
  addWebLog("Height: " + String(height) + " -> " + String(height_modbus) + " (Reg 40001)");
  addWebLog("Diameter: " + String(diameter) + " -> " + String(diameter_modbus) + " (Reg 40002)");
  addWebLog("Model: " + String(model) + " (Reg 40003)");
  addWebLog("Status: " + String(status) + " (Reg 40004)");
  addWebLog("Temperature: " + String(temp) + " (Reg 40005)");
  
  lastOperation = "Sending to PLC";
  lastUpdateTime = millis();
  
  return writeMultipleRegisters(SLAVE_ID, START_REGISTER_ADDRESS, values, 5);
}

void hard_reset_button() {
  unsigned long currentMillis = millis();
  static int lastButtonState = HIGH; // Add this static variable
  buttonState = digitalRead(resetPin);
  
  // Detect HIGH to LOW transition (button press)
  if (lastButtonState == HIGH && buttonState == LOW) {
    operationCount++;
    
    addWebLog("=== BUTTON PRESSED - Operation #" + String(operationCount) + " ===");
    // ... rest of your frame sending code stays the same

    // Send UART frame for sensor data request
    uint16_t abc = crc16(TxData, 6);
    TxData[6] = byte((abc) & 0xFF);
    TxData[7] = byte((abc >> 8) & 0xFF);

    String uartFrameStr = "UART Frame: ";
    for (int i = 0; i < 8; i++) {
      if (TxData[i] < 0x10) uartFrameStr += "0";
      uartFrameStr += String(TxData[i], HEX) + " ";
    }
    addWebLog(uartFrameStr);
    
    // Send via Serial (pins 1,3) - NO DE/RE control
    Serial.write(TxData, 8);
    Serial.flush();
    
    addWebLog("UART sensor request sent via power cable");
    addWebLog("Waiting for sensor response...");
    lastOperation = "Waiting for sensor";
    
    // Wait for response
    delay(500);
    
    // Check if we received sensor data
    if (Serial.available() >= 12) {
      addWebLog("Sensor data received! Processing...");
    } else {
      addWebLog("No sensor response - using current values");
      delay(100);
      bool success = sendAllVariablesToPLC();
      
      if (success) {
        addWebLog("✓ Current variables sent to Schneider PLC!");
      } else {
        addWebLog("✗ Failed to send variables to PLC");
      }
    }
  }
    lastButtonState = buttonState; 
}

void signal_button() {
  unsigned long currentMillis = millis();
  static int lastSignalState = HIGH;
  int signalState = digitalRead(signalPin);
  
  if (lastSignalState == HIGH && signalState == LOW) {

        temp = 0;  // Set temp to 0 for signal button
        addWebLog("✓ ----------- temp = 0  -----------------");      
        addWebLog("✓ Signal pin detected 24v came:");      
        addWebLog("✓ setting  temp = 0 :");
        addWebLog("  Height: " + String(height));
        addWebLog("  Diameter: " + String(diameter));
        addWebLog("  Model: " + String(model));
        addWebLog("  Status: " + String(status));
        addWebLog("  Temperature: " + String(temp));
        
        // Send updated values to Schneider PLC via Modbus
        addWebLog("Sending updated data to Schneider PLC with temp = 0...");
        bool success = sendAllVariablesToPLC();
        
        if (success) {
          addWebLog("✓ All variables successfully sent to Schneider PLC!");
          lastOperation = "PLC Update Complete";
        } else {
          addWebLog("✗ Failed to send variables to Schneider PLC");
          lastOperation = "PLC Update Failed";
        }
        
        lastUpdateTime = millis();
  }
  lastSignalState = signalState;
}

// Web server handlers
void handleRoot() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ESP32 UART + Modbus Monitor</title>";
  html += "<meta charset='UTF-8'>";
  html += "<meta http-equiv='refresh' content='5'>";
  html += "<style>";
  html += "body{font-family:Arial,sans-serif;margin:20px;background:#f0f0f0;}";
  html += ".container{max-width:1200px;margin:0 auto;background:white;padding:20px;border-radius:10px;box-shadow:0 2px 10px rgba(0,0,0,0.1);}";
  html += ".header{background:#2c3e50;color:white;padding:15px;border-radius:5px;margin-bottom:20px;}";
  html += ".status-card{background:#ecf0f1;padding:15px;margin:10px 0;border-radius:5px;border-left:4px solid #3498db;}";
  html += ".success{border-left-color:#27ae60;}";
  html += ".error{border-left-color:#e74c3c;}";
  html += ".data-table{width:100%;border-collapse:collapse;margin:20px 0;}";
  html += ".data-table th,.data-table td{border:1px solid #ddd;padding:8px;text-align:left;}";
  html += ".data-table th{background:#34495e;color:white;}";
  html += ".log-container{max-height:400px;overflow-y:auto;background:#2c3e50;color:#ecf0f1;padding:15px;border-radius:5px;font-family:monospace;font-size:12px;}";
  html += ".clear-btn{background:#e74c3c;color:white;padding:10px 20px;border:none;border-radius:5px;cursor:pointer;margin:10px 0;}";
  html += ".clear-btn:hover{background:#c0392b;}";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<div class='header'><h1>🔧 ESP32 UART + Modbus Monitor</h1>";
  html += "<p>Access Point: Wheel Vision</p></div>";
  
  // Status Cards
  html += "<div class='status-card " + String(lastModbusStatus ? "success" : "error") + "'>";
  html += "<h3>📊 System Status</h3>";
  html += "<p><strong>Last Operation:</strong> " + lastOperation + "</p>";
  html += "<p><strong>Last Error:</strong> " + lastError + "</p>";
  html += "<p><strong>Operations Count:</strong> " + String(operationCount) + "</p>";
  html += "<p><strong>Last Update:</strong> " + String(lastUpdateTime/1000) + "s ago</p>";
  html += "</div>";
  
  // Current Data Table
  html += "<table class='data-table'>";
  html += "<tr><th colspan='4'>📈 Current Sensor Data</th></tr>";
  html += "<tr><th>Variable</th><th>Raw Value</th><th>Modbus Value</th><th>PLC Register</th></tr>";
  html += "<tr><td>Height</td><td>" + String(height) + "</td><td>" + String(height_modbus) + "</td><td>40001</td></tr>";
  html += "<tr><td>Diameter</td><td>" + String(diameter) + "</td><td>" + String(diameter_modbus) + "</td><td>40002</td></tr>";
  html += "<tr><td>Model</td><td>" + String(model) + "</td><td>" + String(model) + "</td><td>40003</td></tr>";
  html += "<tr><td>Status</td><td>" + String(status) + "</td><td>" + String(status) + "</td><td>40004</td></tr>";
  html += "<tr><td>Temperature</td><td>" + String(temp) + "</td><td>" + String(temp) + "</td><td>40005</td></tr>";
  html += "</table>";
  
  // Clear button
  html += "<button class='clear-btn' onclick=\"window.location.href='/clear'\">🗑️ Clear Log</button>";
  
  // Log Container
  html += "<div class='status-card'><h3>📝 System Log</h3>";
  html += "<div class='log-container'>" + webLog + "</div></div>";
  
  html += "</div></body></html>";
  
  server.send(200, "text/html", html);
}

void handleClear() {
  webLog = "";
  addWebLog("Log cleared by user");
  
  String html = "<!DOCTYPE html><html><head>";
  html += "<meta http-equiv='refresh' content='2; url=/'>";
  html += "<style>body{font-family:Arial,sans-serif;text-align:center;margin-top:100px;}</style>";
  html += "</head><body>";
  html += "<h2>✅ Log Cleared Successfully!</h2>";
  html += "<p>Redirecting to main page...</p>";
  html += "</body></html>";
  
  server.send(200, "text/html", html);
}

void setup() {
  // Initialize pins
  pinMode(MAX485_DE_RE_PIN, OUTPUT);
  pinMode(ledpin, OUTPUT);
  pinMode(pcb_ledpin, OUTPUT);
  pinMode(resetPin, INPUT_PULLUP); 
  pinMode(signalPin, INPUT_PULLUP); 
  
  digitalWrite(resetPin, HIGH);
  digitalWrite(ledpin, LOW);
  digitalWrite(pcb_ledpin, HIGH);
  setReceiveMode();

  // Initialize UART communications
  Serial.begin(19200, SERIAL_8E1);
  Serial2.begin(19200, SERIAL_8E1, RXD_PIN, TXD_PIN);
  
  // Setup WiFi Access Point
  WiFi.softAP(ssid, password);
  IPAddress IP = WiFi.softAPIP();
  
  // Setup web server routes
  server.on("/", handleRoot);
  server.on("/clear", handleClear);
  server.begin();
  
  digitalWrite(ledpin, HIGH);
  
  // Initialize web log
  addWebLog("=== ESP32 UART + Modbus System Started ===");
  addWebLog("WiFi AP: " + String(ssid));
  addWebLog("Web Server: http://" + IP.toString());
  addWebLog("Process: Button -> UART -> Sensor Data -> Modbus PLC");
  addWebLog("System ready - Press button to start...");
}

void loop() {
  // Handle web server
  server.handleClient();
  
  // Handle button press
  hard_reset_button();

  signal_button();

  // Check for UART sensor data
  if (Serial.available() >= 13) {
    
    received_bytes = Serial.available();
    Serial.readBytes(rxdata, received_bytes);
    
    String rawDataStr = "UART Raw Data: ";
    for (int i = 0; i < received_bytes; i++) {
      if (rxdata[i] < 0x10) rawDataStr += "0";
      rawDataStr += String(rxdata[i], HEX) + " ";
    }
    addWebLog(rawDataStr);
    
    // Process the received sensor data
    if (rxdata[0] == 0x01) { // Check slave ID
      if (rxdata[1] == 0x03) { // Check function code
        // Extract sensor values
        height = convertToFloat(rxdata[2], rxdata[3], rxdata[4], rxdata[5]);
        diameter = convertToFloat(rxdata[6], rxdata[7], rxdata[8], rxdata[9]);
        model = rxdata[10];
        status = rxdata[11];
        temp = rxdata[12]; // Fixed temperature value
        
        addWebLog("✓ Sensor data extracted successfully:");
        addWebLog("  Height: " + String(height));
        addWebLog("  Diameter: " + String(diameter));
        addWebLog("  Model: " + String(model));
        addWebLog("  Status: " + String(status));
        addWebLog("  Temperature: " + String(temp));
        
        // Send updated values to Schneider PLC via Modbus
        addWebLog("Sending updated data to Schneider PLC...");
        bool success = sendAllVariablesToPLC();
        
        if (success) {
          addWebLog("✓ All variables successfully sent to Schneider PLC!");
          lastOperation = "PLC Update Complete";
        } else {
          addWebLog("✗ Failed to send variables to Schneider PLC");
          lastOperation = "PLC Update Failed";
        }
        
      } else {
        addWebLog("✗ Invalid UART function code: " + String(rxdata[1], HEX));
        lastError = "Invalid UART function code";
      }
    } else {
      addWebLog("✗ Invalid UART slave ID: " + String(rxdata[0], HEX));
      lastError = "Invalid UART slave ID";
    }
  }

}