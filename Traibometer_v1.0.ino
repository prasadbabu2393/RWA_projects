

#include <HardwareSerial.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WebServer.h>
#include <Update.h>
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

Preferences preferences;

#define RXD2 16  //esp32 Rx for stm32
#define TXD2 17  //esp32 Tx for stm32

//pins:  12 14
const int HX711_dout = 14; //mcu > HX711 dout pin
const int HX711_sck = 12; //mcu > HX711 sck pin
//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int LVDT_pin = 35;  // Analog pin connected to LVDT output
const int limit_switch = 26;  // Digital pin for brake status

const int calVal_eepromAdress = 0;
unsigned long t = 0;

String header = "";
WebServer server(80);  // Back to WebServer for OTA functionality
WiFiServer dataServer(81);  // Separate server for data to avoid blocking
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

String ssid_h = "Tribometer_#1";  // hotspot name and password 
const char* password_h = "12345678";    

// OTA Config
const char* ota_username = "admin";
const char* ota_password = "password";
bool updateInProgress = false;
size_t updateSize = 0;
size_t updateProgress = 0;

bool SAMPLE = 0;
float calibrationValue; // calibration value
float dividing_factor; // calibration value
float battery_percent;
float LVDT_voltage;       // Variable to store the voltage reading
float cof = 0;
float meter, radius = 48; //diameter of disc of encoder is 96mm
float pulse_per_mm = 1.956;
int32_t number = 0;
uint8_t RxData[10];
uint8_t a1 = 0;// for serial.available
int brake_count = 0;
String password_1, network_1, pulses, wifi_name;

union FloatToBytes {
  float value;
  uint8_t bytes[4]; //for converting float value into bytes
};
FloatToBytes converter;

// this function for taking count of A,B from stm32 and converting received bytes into integer(+/-)
int32_t fromBytesToSignedInt(uint8_t* byteArray) {
  return (int32_t)(byteArray[3] | 
                   (byteArray[2] << 8) | 
                   (byteArray[1] << 16) | 
                   (byteArray[0] << 24));
}

// Helper function to URL decode parameters
String urlDecode(String input) {
  String decoded = "";
  char a, b;
  for (size_t i = 0; i < input.length(); i++) {
    if (input[i] == '%') {
      if (i + 2 < input.length()) {
        a = input[i + 1];
        b = input[i + 2];
        if (isxdigit(a) && isxdigit(b)) {
          decoded += (char)(hexToInt(a) * 16 + hexToInt(b));
          i += 2;
        } else {
          decoded += input[i];
        }
      } else {
        decoded += input[i];
      }
    } else if (input[i] == '+') {
      decoded += ' ';
    } else {
      decoded += input[i];
    }
  }
  return decoded;
}

// Helper function to convert hex char to int
int hexToInt(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
}

// Handle firmware update
void handleUpdate() {
  HTTPUpload& upload = server.upload();
  
  switch (upload.status) {
    case UPLOAD_FILE_START:
      Serial.printf("Update: %s\n", upload.filename.c_str());
      updateInProgress = true;
      updateProgress = 0;
      updateSize = 0;
      
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
        Serial.println("Error starting update");
        Update.printError(Serial);
      }
      break;
      
    case UPLOAD_FILE_WRITE:
      // Write firmware chunk
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Serial.println("Error writing update");
        Update.printError(Serial);
      }
      updateProgress += upload.currentSize;
      updateSize = upload.totalSize;
      Serial.printf("Progress: %u / %u bytes (%.1f%%)\n", 
        updateProgress, updateSize, (float)updateProgress * 100 / updateSize);
      break;
      
    case UPLOAD_FILE_END:
      // Finish update
      if (Update.end(true)) {
        Serial.printf("Update Success: %u bytes\n", upload.totalSize);
      } else {
        Serial.println("Update failed");
        Update.printError(Serial);
      }
      updateInProgress = false;
      break;
      
    case UPLOAD_FILE_ABORTED:
      Serial.println("Update aborted");
      Update.end();
      updateInProgress = false;
      break;
  }
}

// Modern HTML CSS Template for login page
const char* loginHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Tribometer Configuration</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
      background-color: #f5f5f5;
      color: #333;
    }
    .container {
      background-color: white;
      border-radius: 8px;
      padding: 20px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h1 {
      color: #2c3e50;
      margin-top: 0;
      border-bottom: 2px solid #3498db;
      padding-bottom: 10px;
    }
    .form-group {
      margin-bottom: 15px;
    }
    label {
      display: inline-block;
      width: 150px;
      font-weight: bold;
    }
    input[type="text"] {
      width: 60%;
      padding: 8px;
      border: 1px solid #ddd;
      border-radius: 4px;
      font-size: 16px;
    }
    .btn {
      background-color: #3498db;
      color: white;
      border: none;
      padding: 10px 15px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 16px;
      transition: background-color 0.3s;
      margin-top: 10px;
    }
    .btn:hover {
      background-color: #2980b9;
    }
    .current-values {
      margin-top: 20px;
      padding: 15px;
      background-color: #ecf0f1;
      border-radius: 4px;
    }
    .current-values p {
      margin: 5px 0;
    }
    .value-label {
      font-weight: bold;
      display: inline-block;
      width: 150px;
    }
    .ota-link {
      display: block;
      margin-top: 20px;
      text-align: center;
      color: #3498db;
      text-decoration: none;
    }
    .ota-link:hover {
      text-decoration: underline;
    }
    #restart-msg {
      display: none;
      margin-top: 20px;
      padding: 15px;
      background-color: #e74c3c;
      color: white;
      border-radius: 4px;
      text-align: center;
      font-weight: bold;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>Tribometer Configuration</h1>
    
    <form action="/action_page" method="GET">
      <div class="form-group">
        <label for="load_value">Load Value:</label>
        <input type="text" id="load_value" name="load_value">
      </div>
      
      <div class="form-group">
        <label for="dividing_factor">Dividing Factor:</label>
        <input type="text" id="dividing_factor" name="dividing_factor">
      </div>
      
      <div class="form-group">
        <label for="pulse_per_mm">Pulse Per mm:</label>
        <input type="text" id="pulse_per_mm" name="pulse_per_mm">
      </div>
      
      <div class="form-group">
        <label for="wifi_name">WiFi Name:</label>
        <input type="text" id="wifi_name" name="wifi_name">
      </div>
      
      <button type="submit" class="btn" id="save-btn">Save Configuration</button>
    </form>
    
    <div id="restart-msg">Device is restarting... Please wait...</div>
    
    <div class="current-values">
      <h3>Current Values</h3>
      <p><span class="value-label">Calibration Value:</span> %CALIBRATION_VALUE%</p>
      <p><span class="value-label">Dividing Factor:</span> %DIVIDING_FACTOR%</p>
      <p><span class="value-label">Pulse Per mm:</span> %PULSE_PER_MM%</p>
      <p><span class="value-label">WiFi Name:</span> %WIFI_NAME%</p>
    </div>
    
    <a href="/update" class="ota-link">Firmware Update</a>
    <a href="/" class="ota-link">Back to Dashboard</a>
  </div>
  
  <script>
    document.getElementById('save-btn').addEventListener('click', function() {
      document.getElementById('restart-msg').style.display = 'block';
    });
  </script>
</body>
</html>
)rawliteral";

// Dashboard HTML template
const char* dashboardHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>Tribometer Dashboard</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 900px;
      margin: 0 auto;
      padding: 20px;
      background-color: #f5f5f5;
      color: #333;
    }
    .container {
      background-color: white;
      border-radius: 8px;
      padding: 20px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h1 {
      color: #2c3e50;
      margin-top: 0;
      border-bottom: 2px solid #3498db;
      padding-bottom: 10px;
    }
    .dashboard-header {
      display: flex;
      justify-content: space-between;
      align-items: center;
      margin-bottom: 20px;
    }
    .status-indicator {
      display: inline-block;
      width: 15px;
      height: 15px;
      border-radius: 50%;
      margin-right: 5px;
      background-color: #e74c3c;
    }
    .status-indicator.active {
      background-color: #2ecc71;
    }
    .data-grid {
      display: grid;
      grid-template-columns: 1fr 1fr;
      gap: 20px;
      margin-bottom: 20px;
    }
    .data-card {
      background-color: #ecf0f1;
      border-radius: 8px;
      padding: 15px;
    }
    .data-card h3 {
      margin-top: 0;
      color: #2c3e50;
      border-bottom: 1px solid #bdc3c7;
      padding-bottom: 8px;
    }
    .data-value {
      font-size: 24px;
      font-weight: bold;
      color: #3498db;
      margin: 10px 0;
    }
    .data-unit {
      font-size: 14px;
      color: #7f8c8d;
    }
    .action-buttons {
      display: flex;
      justify-content: center;
      gap: 20px;
      margin: 20px 0;
    }
    .btn {
      background-color: #3498db;
      color: white;
      border: none;
      padding: 12px 24px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 16px;
      font-weight: bold;
      transition: background-color 0.3s;
      min-width: 120px;
      text-align: center;
    }
    .btn.start {
      background-color: #2ecc71;
    }
    .btn.start:hover {
      background-color: #27ae60;
    }
    .btn.stop {
      background-color: #e74c3c;
    }
    .btn.stop:hover {
      background-color: #c0392b;
    }
    .btn.config {
      background-color: #f39c12;
    }
    .btn.config:hover {
      background-color: #d35400;
    }
    .footer {
      margin-top: 30px;
      text-align: center;
      font-size: 14px;
      color: #7f8c8d;
    }
    .ota-link {
      display: block;
      margin-top: 20px;
      text-align: center;
      color: #3498db;
      text-decoration: none;
    }
    .ota-link:hover {
      text-decoration: underline;
    }
  </style>
</head>
<body>
  <div class="container">
    <div class="dashboard-header">
      <h1>Tribometer Dashboard</h1>
      <div>
        <span class="status-indicator" id="sample-status"></span>
        <span id="status-text">Inactive</span>
      </div>
    </div>
    
    <div class="data-grid">
      <div class="data-card">
        <h3>Coefficient of Friction</h3>
        <div class="data-value" id="cof-value">0.00</div>
      </div>
      
      <div class="data-card">
        <h3>Distance</h3>
        <div class="data-value" id="meter-value">0.00</div>
        <div class="data-unit">mm</div>
      </div>
      
      <div class="data-card">
        <h3>Brake Status</h3>
        <div class="data-value" id="brake-status">OFF</div>
      </div>
      
      <div class="data-card">
        <h3>Brake Count</h3>
        <div class="data-value" id="brake-count">0</div>
      </div>
      
      <div class="data-card">
        <h3>Load Cell</h3>
        <div class="data-value" id="load-cell">0.00</div>
      </div>
      
      <div class="data-card">
        <h3>LVDT Voltage</h3>
        <div class="data-value" id="lvdt-voltage">0.00</div>
        <div class="data-unit">V</div>
      </div>
    </div>
    
    <div class="action-buttons">
      <button id="start-btn" class="btn start">Start</button>
      <button id="stop-btn" class="btn stop">Stop</button>
      <button id="config-btn" class="btn config">Configuration</button>
    </div>
    
    <a href="/update" class="ota-link">Firmware Update</a>
    
    <div class="footer">
      <p>Tribometer #1 | ESP32 Controller</p>
      <p>IP: %IP_ADDRESS%</p>
    </div>
  </div>
  
  <script>
    const startBtn = document.getElementById('start-btn');
    const stopBtn = document.getElementById('stop-btn');
    const configBtn = document.getElementById('config-btn');
    const statusIndicator = document.getElementById('sample-status');
    const statusText = document.getElementById('status-text');
    
    let sampling = false;
    
    function updateData() {
      fetch('/data')
        .then(response => response.text())
        .then(data => {
          const values = data.split(',');
          
          document.getElementById('cof-value').textContent = parseFloat(values[0]).toFixed(3);
          document.getElementById('meter-value').textContent = parseFloat(values[1]).toFixed(2);
          
          const brakeStatus = parseInt(values[2]);
          document.getElementById('brake-status').textContent = brakeStatus === 0 ? 'ON' : 'OFF';
          
          document.getElementById('brake-count').textContent = values[3];
          document.getElementById('load-cell').textContent = parseFloat(values[9]).toFixed(2);
          document.getElementById('lvdt-voltage').textContent = parseFloat(values[10]).toFixed(2);
          
          // Update sampling status indicator (position 4 in data array)
          const samplingStatus = parseInt(values[4]);
          if (samplingStatus === 1) {
            sampling = true;
            statusIndicator.classList.add('active');
            statusText.textContent = 'Active';
          } else {
            sampling = false;
            statusIndicator.classList.remove('active');
            statusText.textContent = 'Inactive';
          }
        })
        .catch(error => {
          console.error('Error fetching data:', error);
        });
    }
    
    startBtn.addEventListener('click', () => {
      fetch('/start')
        .then(() => {
          sampling = true;
          statusIndicator.classList.add('active');
          statusText.textContent = 'Active';
        })
        .catch(error => {
          console.error('Error starting sampling:', error);
        });
    });
    
    stopBtn.addEventListener('click', () => {
      fetch('/stop')
        .then(() => {
          sampling = false;
          statusIndicator.classList.remove('active');
          statusText.textContent = 'Inactive';
        })
        .catch(error => {
          console.error('Error stopping sampling:', error);
        });
    });
    
    configBtn.addEventListener('click', () => {
      window.location.href = '/login';
    });
    
    // Update data every 500ms
    setInterval(updateData, 500);
    
    // Initial data load
    updateData();
  </script>
</body>
</html>
)rawliteral";

// OTA update HTML template (RESTORED FULL FUNCTIONALITY)
const char* otaHtml = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 OTA Update</title>
  <style>
    body {
      font-family: Arial, sans-serif;
      max-width: 800px;
      margin: 0 auto;
      padding: 20px;
      background-color: #f5f5f5;
      color: #333;
    }
    .container {
      background-color: white;
      border-radius: 8px;
      padding: 20px;
      box-shadow: 0 2px 10px rgba(0,0,0,0.1);
    }
    h1 {
      color: #2c3e50;
      margin-top: 0;
      border-bottom: 2px solid #3498db;
      padding-bottom: 10px;
    }
    .form-group {
      margin-bottom: 15px;
    }
    .btn {
      background-color: #3498db;
      color: white;
      border: none;
      padding: 10px 15px;
      border-radius: 4px;
      cursor: pointer;
      font-size: 16px;
      transition: background-color 0.3s;
    }
    .btn:hover {
      background-color: #2980b9;
    }
    .btn:disabled {
      background-color: #95a5a6;
      cursor: not-allowed;
    }
    #upload-form {
      margin-top: 20px;
    }
    .file-input {
      margin-bottom: 15px;
      width: 100%;
    }
    #progress-container {
      display: none;
      margin-top: 20px;
    }
    .progress-bar {
      height: 20px;
      background-color: #ecf0f1;
      border-radius: 10px;
      margin-bottom: 10px;
      overflow: hidden;
    }
    .progress-fill {
      height: 100%;
      background-color: #2ecc71;
      width: 0%;
      transition: width 0.3s;
    }
    .status {
      font-weight: bold;
    }
    .success {
      color: #27ae60;
    }
    .error {
      color: #e74c3c;
    }
    .info {
      color: #3498db;
      margin-top: 20px;
      padding: 10px;
      background-color: #e8f4f8;
      border-radius: 4px;
    }
    .device-info {
      margin-top: 30px;
      font-size: 14px;
      color: #7f8c8d;
    }
    .back-link {
      display: block;
      margin-top: 20px;
      text-align: center;
      color: #3498db;
      text-decoration: none;
    }
    .back-link:hover {
      text-decoration: underline;
    }
    .ota-status {
      padding: 10px;
      border-radius: 4px;
      margin-top: 15px;
      font-weight: bold;
      text-align: center;
    }
    .ota-active {
      background-color: #2ecc71;
      color: white;
    }
    .ota-inactive {
      background-color: #3498db;
      color: white;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>ESP32 OTA Update</h1>
    
    <div class="ota-status ota-inactive">OTA Update Ready</div>
    
    <div class="info">
      <p>Upload a new firmware (.bin file) to update your ESP32 device.</p>
      <p>Device will automatically restart after successful update.</p>
    </div>
    
    <form id="upload-form" method="POST" action="/update" enctype="multipart/form-data">
      <div class="form-group">
        <input type="file" name="update" id="firmware-file" class="file-input" accept=".bin">
      </div>
      <button type="submit" class="btn" id="upload-btn">Upload & Update</button>
    </form>
    
    <div id="progress-container">
      <p class="status" id="status-text">Uploading firmware...</p>
      <div class="progress-bar">
        <div class="progress-fill" id="progress"></div>
      </div>
      <p id="progress-text">0%</p>
    </div>

    <div class="device-info">
      <p>Device: ESP32 Tribometer</p>
      <p>IP Address: %IP_ADDRESS%</p>
    </div>
    
    <a href="/" class="back-link">Back to Dashboard</a>
  </div>

  <script>
    document.getElementById('upload-form').addEventListener('submit', function(e) {
      const fileInput = document.getElementById('firmware-file');
      if (!fileInput.files.length) {
        e.preventDefault();
        alert('Please select a firmware file');
        return;
      }
      
      document.getElementById('upload-btn').disabled = true;
      document.getElementById('progress-container').style.display = 'block';
      document.querySelector('.ota-status').className = 'ota-status ota-active';
      document.querySelector('.ota-status').textContent = 'OTA Update in Progress';
      
      const xhr = new XMLHttpRequest();
      const formData = new FormData(this);
      
      xhr.upload.addEventListener('progress', function(e) {
        if (e.lengthComputable) {
          const percentComplete = (e.loaded / e.total) * 100;
          document.getElementById('progress').style.width = percentComplete + '%';
          document.getElementById('progress-text').textContent = Math.round(percentComplete) + '%';
        }
      });
      
      xhr.addEventListener('load', function() {
        if (xhr.status === 200) {
          document.getElementById('status-text').textContent = 'Update successful! Device is restarting...';
          document.getElementById('status-text').className = 'status success';
        } else {
          document.getElementById('status-text').textContent = 'Update failed! Please try again.';
          document.getElementById('status-text').className = 'status error';
          document.getElementById('upload-btn').disabled = false;
          document.querySelector('.ota-status').className = 'ota-status ota-inactive';
          document.querySelector('.ota-status').textContent = 'OTA Update Ready';
        }
      });
      
      xhr.addEventListener('error', function() {
        document.getElementById('status-text').textContent = 'Connection error! Please try again.';
        document.getElementById('status-text').className = 'status error';
        document.getElementById('upload-btn').disabled = false;
        document.querySelector('.ota-status').className = 'ota-status ota-inactive';
        document.querySelector('.ota-status').textContent = 'OTA Update Ready';
      });
      
      xhr.open('POST', '/update', true);
      xhr.send(formData);
      e.preventDefault();
    });
  </script>
</body>
</html>
)rawliteral";

void setup() {
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);  // uart_begin
  pinMode(LVDT_pin, INPUT); // analog pin of esp32 for battery
  pinMode(limit_switch, INPUT_PULLUP); // brake status pin (0, 1)
  
  preferences.begin("memory", false);

  ssid_h = preferences.getString("wifi_name", ssid_h);
  dividing_factor = preferences.getString("password", "4").toFloat();
  pulse_per_mm = preferences.getString("pulses", "1.956").toFloat();
  calibrationValue = preferences.getString("ssid", "67286.00").toFloat();

  delay(10);
  LoadCell.begin();
  
  #if defined(ESP8266)|| defined(ESP32)
  //EEPROM.begin(512);
  #endif

  unsigned long stabilizingtime = 2000;
  boolean _tare = true;
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag()) {
    Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue);
    Serial.println("Startup is complete");
  }

  // Start WiFi Access Point
  WiFi.softAP(ssid_h, password_h);
  Serial.print("AP started with IP: ");
  Serial.println(WiFi.softAPIP());

  // Set up server endpoints
  setupServer();
  
  // Start both servers
  server.begin();
  dataServer.begin();
}

void setupServer() {
  // Main dashboard
  server.on("/", HTTP_GET, []() {
    String html = String(dashboardHtml);
    html.replace("%IP_ADDRESS%", WiFi.softAPIP().toString());
    server.send(200, "text/html", html);
  });

  // Login page
  server.on("/login", HTTP_GET, []() {
    String html = String(loginHtml);
    html.replace("%CALIBRATION_VALUE%", String(calibrationValue));
    html.replace("%DIVIDING_FACTOR%", String(dividing_factor));
    html.replace("%PULSE_PER_MM%", String(pulse_per_mm));
    html.replace("%WIFI_NAME%", ssid_h);
    server.send(200, "text/html", html);
  });

  // Handle configuration form submission
  server.on("/action_page", HTTP_GET, []() {
    String loadValue = server.arg("load_value");
    String divFactor = server.arg("dividing_factor");
    String pulseMM = server.arg("pulse_per_mm");
    String wifiName = server.arg("wifi_name");
    
    // Save values to preferences if not empty
    if (loadValue.length() > 0) {
      preferences.putString("ssid", loadValue);
      calibrationValue = loadValue.toFloat();
      LoadCell.setCalFactor(calibrationValue);
    }
    
    if (divFactor.length() > 0) {
      preferences.putString("password", divFactor);
      dividing_factor = divFactor.toFloat();
    }
    
    if (pulseMM.length() > 0) {
      preferences.putString("pulses", pulseMM);
      pulse_per_mm = pulseMM.toFloat();
    }
    
    if (wifiName.length() > 0) {
      preferences.putString("wifi_name", wifiName);
      ssid_h = wifiName;
    }
    
    // Send response and restart ESP32
    server.send(200, "text/html", "<html><head><meta http-equiv='refresh' content='5;URL=/'></head><body><h2>Configuration saved! Device is restarting...</h2></body></html>");
    delay(1000);
    ESP.restart();
  });

  // Endpoint to provide live data to dashboard
  server.on("/data", HTTP_GET, []() {
    // Process fresh UART data when requested
    if (Serial2.available() >= 4) {
      a1 = Serial2.available();
      if (a1 > 10) a1 = 10;
      Serial2.readBytes(RxData, a1);
      number = fromBytesToSignedInt(RxData);
      meter = (float)number / pulse_per_mm;
    }
    
    String dataString = String(cof) + "," + 
                      String(meter) + "," + 
                      String(digitalRead(limit_switch)) + "," + 
                      String(brake_count) + "," +
                      String(SAMPLE) + "," +
                      "0,0,0,0," +  // Temperature, pressure, etc. placeholders
                      String(LoadCell.getData()) + "," +
                      String(LVDT_voltage);
    
    server.send(200, "text/plain", dataString);
  });

  // Start sampling endpoint
  server.on("/start", HTTP_GET, []() {
    uint8_t x1 = 0;
    Serial2.write(x1); // Reset STM32 counter
    LoadCell.tareNoDelay();
    brake_count = 0;
    uint8_t b1 = 1;
    Serial2.write(b1); // Send reset signal to STM32
    SAMPLE = 1;
    Serial.println("Sampling started");
    server.send(200, "text/plain", "Sampling started");
  });

  server.on("/stop", HTTP_GET, []() {
    SAMPLE = 0;
    Serial.println("Sampling stopped");
    server.send(200, "text/plain", "Sampling stopped");
  });

  // OTA Update page with authentication
  server.on("/update", HTTP_GET, []() {
    if (!server.authenticate(ota_username, ota_password)) {
      return server.requestAuthentication();
    }
    
    String html = String(otaHtml);
    html.replace("%IP_ADDRESS%", WiFi.softAPIP().toString());
    
    if (updateInProgress) {
      html.replace("OTA Update Ready", "OTA Update in Progress");
      html.replace("ota-inactive", "ota-active");
    }
    
    server.send(200, "text/html", html);
  });

  // Handle OTA update file upload
  server.on("/update", HTTP_POST, []() {
    // Respond after update completes
    server.sendHeader("Connection", "close");
    
    if (Update.hasError()) {
      server.send(200, "text/plain", "UPDATE FAILED");
    } else {
      server.send(200, "text/plain", "UPDATE SUCCESS");
      delay(1000);
      ESP.restart();
    }
  }, handleUpdate);
}

void loop() {
  // Handle UART data with HIGHEST PRIORITY
  static unsigned long lastUARTCheck = 0;
  if (millis() - lastUARTCheck > 10) { // Check every 10ms
    handleUARTData();
    lastUARTCheck = millis();
  }
  
  // Handle Load Cell data
  static unsigned long lastLoadCellCheck = 0;
  if (millis() - lastLoadCellCheck > 50) { // Check every 50ms
    handleLoadCellData();
    lastLoadCellCheck = millis();
  }
  
  // Handle web server with LOWEST PRIORITY
  static unsigned long lastServerCheck = 0;
  if (millis() - lastServerCheck > 20) { // Check every 20ms
    server.handleClient();
    lastServerCheck = millis();
  }
}

void handleUARTData() {
  if (Serial2.available() >= 4) {
    a1 = Serial2.available();
    if (a1 > 10) a1 = 10; // Safety check
    
    Serial2.readBytes(RxData, a1);
    number = fromBytesToSignedInt(RxData);
    meter = (float)number / pulse_per_mm;
    
    Serial.print("UART - number: ");
    Serial.print(number);
    Serial.print(", meter: ");
    Serial.println(meter);
  }
}

void handleLoadCellData() {
  static boolean newDataReady = false;
  
  if (LoadCell.update()) newDataReady = true;
  
  if (newDataReady) {
    // Read LVDT voltage
    int lvdtRaw = analogRead(LVDT_pin);
    LVDT_voltage = (float)lvdtRaw * 3.3 / 4095.0;
    
    // Read brake status and count
    int brakeStatus = digitalRead(limit_switch);
    static int prevBrakeStatus = -1;
    
    if (brakeStatus == 0 && prevBrakeStatus == 1) {
      brake_count++;
    }
    prevBrakeStatus = brakeStatus;
    
    // Calculate coefficient of friction if sampling
    if (SAMPLE == 1) {
      float loadValue = LoadCell.getData();
      if (loadValue < 0) loadValue = abs(loadValue);
      if (loadValue != 0) {
        cof = loadValue / dividing_factor;
      }
    }
    
    newDataReady = false;
  }
}