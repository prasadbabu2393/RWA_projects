#include <HardwareSerial.h>
#include <WiFi.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>

// Use separate preferences for WiFi and switch count configurations
Preferences wifiPrefs;
Preferences switchPrefs;

WebServer webServer(80);

const char* ssid_h = "Andon_System";  // hotspot name and password
const char* password_h = "12345678";

IPAddress flash_ip;
String header;    
String network_1, password_1, client_ip, ip;
String ssid, password;

bool MODE;
const int resetPin = 19;     // Reset button pin
int programState = 0, buttonState;
long buttonMillis = 0;

// Add these global variables for WiFi reconnection
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 5000; // Check WiFi every 5 seconds
int reconnectAttempts = 0;
bool isReconnecting = false;

// Set your Static IP address
int a = 192, b = 168, c = 0, d = 114;
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

unsigned long previousTime = 0;
const long timeoutTime = 2000;

int switchState[8] = {0, 0, 0, 0, 0, 0, 0, 0};     // Array to store the states of the switches
int lastSwitchState[8] = {0, 0, 0, 0, 0, 0, 0, 0};  // Array to store the last states for comparison
int switchCount[8] = {0, 0, 0, 0, 0, 0, 0, 0};    // Array to store the toggle counts for each switch

const int switchPins[8] = {32, 35, 34, 33, 25, 26, 27, 14};  // Pins for the eight switches
int led_pin = 21;   // d5 for led
int retry_count = 0;

// Convert string IP to integers
void convert(String address) {
  // Split the IP address string by dots
  int d1 = address.indexOf('.');
  int d2 = address.indexOf('.', d1 + 1);
  int d3 = address.indexOf('.', d2 + 1);

  // Convert each part into an integer
  a = address.substring(0, d1).toInt();
  b = address.substring(d1 + 1, d2).toInt();
  c = address.substring(d2 + 1, d3).toInt();
  d = address.substring(d3 + 1).toInt();
}

// URL decode function
String urlDecode(String input) {
  input.replace("%20", " ");
  input.replace("%21", "!");
  input.replace("%22", "\"");
  input.replace("%23", "#");
  input.replace("%24", "$");
  input.replace("%25", "%");
  input.replace("%26", "&");
  input.replace("%27", "'");
  input.replace("%28", "(");
  input.replace("%29", ")");
  input.replace("%2A", "*");
  input.replace("%2B", "+");
  input.replace("%2C", ",");
  input.replace("%2D", "-");
  input.replace("%2E", ".");
  input.replace("%2F", "/");
  input.replace("%40", "@"); 
  return input;
}

// OTA Setup Function
void setupOTA() {
  // Handle OTA update form
  webServer.on("/ota", HTTP_GET, handleOTAForm);
  
  // Handle file upload
  webServer.on("/update", HTTP_POST, []() {
    webServer.sendHeader("Connection", "close");
    if (Update.hasError()) {
      webServer.send(500, "text/plain", "Update Failed!");
    } else {
      webServer.send(200, "text/plain", "Update Successful! Rebooting...");
      delay(1000);
      ESP.restart();
    }
  }, handleOTAUpload);
  
  Serial.println("OTA server ready");
}

// Handle OTA Form
void handleOTAForm() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>OTA Update</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".form-group { margin: 20px 0; }";
  html += "label { display: block; margin-bottom: 5px; font-weight: bold; }";
  html += "input[type='file'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; }";
  html += ".btn { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; width: 100%; }";
  html += ".btn:hover { background-color: #45a049; }";
  html += ".info { background-color: #e7f3ff; padding: 15px; border-radius: 5px; margin: 20px 0; }";
  html += ".warning { background-color: #fff3cd; padding: 15px; border-radius: 5px; margin: 20px 0; border-left: 4px solid #ffc107; }";
  html += "#progress { width: 100%; background-color: #f0f0f0; border-radius: 5px; margin: 20px 0; display: none; }";
  html += "#progress-bar { width: 0%; height: 30px; background-color: #4CAF50; border-radius: 5px; text-align: center; line-height: 30px; color: white; }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>OTA Firmware Update</h1>";
  
  html += "<div class='info'>";
  html += "<strong>Device Information:</strong><br>";
  html += "MAC Address: " + WiFi.macAddress() + "<br>";
  html += "Current IP: " + WiFi.localIP().toString() + "<br>";
  html += "<strong>Switch Monitoring Configuration:</strong><br>";
  html += "Switch Pins: GPIO 34, 35, 32, 33, 25, 26, 27, 14<br>";
  html += "LED Pin: GPIO " + String(led_pin) + "<br>";
  html += "Reset Pin: GPIO " + String(resetPin) + "<br>";
  html += "</div>";
  
  html += "<div class='warning'>";
  html += "<strong>Warning:</strong> Only upload valid .bin firmware files. Invalid files may brick your device!";
  html += "</div>";
  
  html += "<form method='POST' action='/update' enctype='multipart/form-data' id='upload-form'>";
  html += "<div class='form-group'>";
  html += "<label for='update'>Select Firmware File (.bin):</label>";
  html += "<input type='file' name='update' id='file-input' accept='.bin' required>";
  html += "</div>";
  html += "<input type='submit' value='Update Firmware' class='btn' id='upload-btn'>";
  html += "</form>";
  
  html += "<div id='progress'>";
  html += "<div id='progress-bar'>0%</div>";
  html += "</div>";
  
  html += "<div style='margin-top: 20px; text-align: center;'>";
  html += "<a href='/' style='color: #007bff; text-decoration: none;'> Back to Main Dashboard</a>";
  html += "</div>";
  
  html += "</div>";
  
  // JavaScript for upload progress
  html += "<script>";
  html += "document.getElementById('upload-form').addEventListener('submit', function(e) {";
  html += "  var fileInput = document.getElementById('file-input');";
  html += "  if (!fileInput.files[0]) {";
  html += "    alert('Please select a file');";
  html += "    e.preventDefault();";
  html += "    return;";
  html += "  }";
  html += "  if (!fileInput.files[0].name.endsWith('.bin')) {";
  html += "    alert('Please select a .bin file');";
  html += "    e.preventDefault();";
  html += "    return;";
  html += "  }";
  html += "  document.getElementById('upload-btn').disabled = true;";
  html += "  document.getElementById('upload-btn').value = 'Uploading...';";
  html += "  document.getElementById('progress').style.display = 'block';";
  html += "  simulateProgress();";
  html += "});";
  
  html += "function simulateProgress() {";
  html += "  var progress = 0;";
  html += "  var interval = setInterval(function() {";
  html += "    progress += 2;";
  html += "    if (progress > 90) progress = 90;";
  html += "    document.getElementById('progress-bar').style.width = progress + '%';";
  html += "    document.getElementById('progress-bar').textContent = progress + '%';";
  html += "    if (progress >= 90) clearInterval(interval);";
  html += "  }, 200);";
  html += "}";
  html += "</script>";
  
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

void setup() {
  // Initialize the switch pins as inputs
  for (int i = 0; i < 8; i++) {
    pinMode(switchPins[i], INPUT);  // Using INPUT for switch monitoring
  }

  pinMode(resetPin, INPUT_PULLUP); // Set the reset pin as input with internal pull-up resistor
  digitalWrite(resetPin, HIGH);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, LOW);
  Serial.begin(115200);

  // Initialize separate Preferences for WiFi and switch data
  wifiPrefs.begin("wifi_config", false);
  switchPrefs.begin("switch_config", false);

  String wifi_flag = wifiPrefs.getString("wifi_saved", "");

  // Load WiFi configuration
  if (wifi_flag != "") {
    ssid = wifiPrefs.getString("ssid", "");
    password = wifiPrefs.getString("password", "");
    ip = wifiPrefs.getString("staticIP", "");
  }

  Serial.println("Configuration loaded from flash:");
  Serial.println("SSID: " + ssid);
  Serial.println("Static IP: " + ip);
  
  // Configure static IP if provided
  if(ip != "") {
    Serial.println("Configuring static IP: " + ip);
    convert(ip);
    IPAddress local_IP(a, b, c, d);
    Serial.println("IP configuration: " + local_IP.toString());
    
    if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("Static IP configuration failed");
    }
  }
  
  // Attempt WiFi connection if credentials are available
  if (ssid != "" && password != "") {
    Serial.println("Connecting to WiFi: " + ssid);

    WiFi.begin(ssid.c_str(), password.c_str());
    while (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connecting...");
      digitalWrite(led_pin, !digitalRead(led_pin)); // Blink LED while connecting
      delay(1000);
      retry_count++;
      
      if (retry_count > 20) {
        Serial.println("Connection timeout, restarting to try again...");
        ESP.restart();
       }
      
      // Check for reset button during connection
      unsigned long currentMillis = millis();
      buttonState = digitalRead(resetPin);
      if (buttonState == LOW && programState == 0) {
        buttonMillis = currentMillis;
        programState = 1;
      } else if (programState == 1 && buttonState == HIGH) {
        programState = 0;
      }
      if (((currentMillis - buttonMillis) > 3000) && programState == 1) {
        programState = 0;
        Serial.println("WiFi reset during connection - clearing WiFi credentials");
        wifiPrefs.clear();
        ESP.restart();
      }
    }
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("WiFi connected successfully");
      Serial.println("IP address: " + WiFi.localIP().toString());
      digitalWrite(led_pin, HIGH); // Solid LED when connected
      MODE = 1;
      Serial.println("MAC Address: " + WiFi.macAddress());
    } else {
      // Failed to connect with existing credentials, restart and try again
      Serial.println("Failed to connect to WiFi. Restarting to try again...");
      ESP.restart();
    }
  }
  else { 
    MODE = 0;
    Serial.println("No WiFi credentials found, starting AP mode");
    WiFi.softAP(ssid_h, password_h);
    IPAddress IP = WiFi.softAPIP();
    Serial.println("AP IP address: " + IP.toString());
    digitalWrite(led_pin, LOW); // LED off in AP mode
  }

  // Load stored switch counts from separate preferences
  Serial.println("Loading switch counts...");
  for (int i = 0; i < 8; i++) {
    switchCount[i] = switchPrefs.getInt(("count" + String(i)).c_str(), 0);
    Serial.println("Switch " + String(i+1) + " count: " + String(switchCount[i]));
  }

  // Setup web server routes
  webServer.on("/", handleMain);
  webServer.on("/login", handleLogin);
  webServer.on("/data", handle_getdata);
  webServer.on("/reset", handle_reset);
  webServer.on("/save", HTTP_POST, handleSave);
  
  // Setup OTA
  setupOTA();
  
  webServer.begin();
  Serial.println("Web server started successfully");
  
  digitalWrite(led_pin, HIGH);
}

void loop() {
  webServer.handleClient();
  
  // Handle reset button
  unsigned long currentMillis = millis();
  buttonState = digitalRead(resetPin);
  
  if (buttonState == LOW && programState == 0) {
    buttonMillis = currentMillis;
    programState = 1;
  } else if (programState == 1 && buttonState == HIGH) {
    programState = 0;
  }
  
  if (((currentMillis - buttonMillis) > 3000) && programState == 1) {
    programState = 0;
    Serial.println("Reset button pressed - clearing all preferences");
    wifiPrefs.clear();
    switchPrefs.clear();
    wifiPrefs.end();
    switchPrefs.end();
    ESP.restart();
  }

  // WiFi reconnection logic (only for STA mode)
  if (MODE == 1) {
    // Check WiFi connection periodically
    if (currentMillis - lastWiFiCheck > wifiCheckInterval) {
      lastWiFiCheck = currentMillis;
      
      if (WiFi.status() != WL_CONNECTED) {
        if (!isReconnecting) {
          Serial.println("WiFi connection lost! Attempting to reconnect...");
          isReconnecting = true;
          reconnectAttempts = 0;
          digitalWrite(led_pin, LOW); // Turn off LED when disconnected
        }
        
        // Try to reconnect
        if (reconnectAttempts < 20) {
          reconnectAttempts++;
          Serial.print("Reconnection attempt ");
          Serial.print(reconnectAttempts);
          Serial.println(" of 20");
          
          WiFi.disconnect();
          delay(100);
          WiFi.begin(ssid.c_str(), password.c_str());
          
          // Wait up to 10 seconds for connection
          int waitCount = 0;
          while (WiFi.status() != WL_CONNECTED && waitCount < 10) {
            delay(1000);
            waitCount++;
            digitalWrite(led_pin, !digitalRead(led_pin)); // Blink LED while reconnecting
            
            // Check for reset button during reconnection
            buttonState = digitalRead(resetPin);
            if (buttonState == LOW && programState == 0) {
              buttonMillis = millis();
              programState = 1;
            } else if (programState == 1 && buttonState == HIGH) {
              programState = 0;
            }
            if (((millis() - buttonMillis) > 3000) && programState == 1) {
              programState = 0;
              Serial.println("Reset button pressed during reconnection - clearing preferences");
              wifiPrefs.clear();
              wifiPrefs.end();
              ESP.restart();
            }
          }
          
          if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Reconnected to WiFi!");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            isReconnecting = false;
            reconnectAttempts = 0;
            digitalWrite(led_pin, HIGH); // Solid LED when connected
          }
        } else {
          // After 20 failed attempts, restart ESP32
          Serial.println("Failed to reconnect after 20 attempts. Restarting ESP32...");
          ESP.restart();
        }
      } else {
        // WiFi is connected
        if (isReconnecting) {
          isReconnecting = false;
          reconnectAttempts = 0;
        }
        digitalWrite(led_pin, HIGH); // Solid LED when connected
      }
    }
  }
  
  // Check switch states and update counts
  for (int i = 0; i < 8; i++) { 
    switchState[i] = digitalRead(switchPins[i]);
    // If the switch was just pressed (transition from HIGH to LOW)
    if (switchState[i] == LOW && lastSwitchState[i] == HIGH) {   
      // Increment the switch count
      switchCount[i]++;
      switchPrefs.putInt(("count" + String(i)).c_str(), switchCount[i]);
      Serial.println("Switch " + String(i+1) + " activated. New count: " + String(switchCount[i]));
    }
    // Update last switch state
    lastSwitchState[i] = switchState[i];
  }
  
  // Status LED management
  if (MODE == 1) {
    // Connected to WiFi - solid LED (unless reconnecting)
    if (!isReconnecting) {
      digitalWrite(led_pin, HIGH);
    }
  } else {
    // AP mode - slow blink
    if ((currentMillis / 1000) % 2 == 0) {
      digitalWrite(led_pin, HIGH);
    } else {
      digitalWrite(led_pin, LOW);
    }
  }
  
  delay(10);
}

// Handle OTA Upload
void handleOTAUpload() {
  HTTPUpload& upload = webServer.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    
    // Start update process
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    // Write firmware data
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    } else {
      Serial.printf("Written: %d bytes\n", upload.currentSize);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

// API endpoint for getting switch data
void handle_getdata() {
  String response = "{";
  for (int i = 0; i < 8; i++) {
    switchState[i] = digitalRead(switchPins[i]);
    // If the switch was just pressed (transition from HIGH to LOW)
    if (switchState[i] == LOW && lastSwitchState[i] == HIGH) {   
      // Increment the switch count
      switchCount[i]++;
      switchPrefs.putInt(("count" + String(i)).c_str(), switchCount[i]); 
    }
    // Update last switch state
    lastSwitchState[i] = switchState[i];
  }
  for (int i = 0; i < 8; i++) {
    response += String(switchState[i]);
    response += ",";
    response += String(switchCount[i]);
    if(i < 7) response += ",";
  }
  response += "}";
  
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "text/plain", response);
}

// Reset counts endpoint
void handle_reset() {
  for (int i = 0; i < 8; i++) {
    switchCount[i] = 0;
    switchPrefs.putInt(("count" + String(i)).c_str(), switchCount[i]); 
  }
  
  // Redirect response
  String html = "<!DOCTYPE html>";
  html += "<html>";
  html += "<head>";
  html += "<meta http-equiv='refresh' content='2;url=/' />";
  html += "<title>Andon Signaling System</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; text-align: center; background-color: #f0f8ff; margin: 0; padding: 20px; color: #333; }";
  html += ".message { background-color: #4CAF50; color: white; padding: 15px; border-radius: 5px; margin: 20px auto; max-width: 400px; box-shadow: 0 4px 8px rgba(0,0,0,0.1); }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<div class='message'><h2>Counters Reset Successfully!</h2><p>Redirecting to home page...</p></div>";
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

// Enhanced WiFi Configuration page handler
void handleLogin() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Andon System Configuration</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }";
  html += ".container { max-width: 900px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 15px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; margin-bottom: 30px; }";
  html += ".form-group { margin: 20px 0; }";
  html += "label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }";
  html += "input[type='text'], input[type='password'] { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 5px; font-size: 16px; }";
  html += ".btn { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; margin: 10px 5px; }";
  html += ".btn:hover { background-color: #45a049; }";
  html += ".btn-secondary { background-color: #007bff; }";
  html += ".btn-secondary:hover { background-color: #0056b3; }";
  html += ".info { background-color: #e7f3ff; padding: 15px; border-radius: 5px; margin: 20px 0; }";
  html += ".current-counts { background-color: #f8f9fa; padding: 20px; border-radius: 5px; margin: 20px 0; }";
  html += ".count-item { display: inline-block; margin: 10px 15px; padding: 10px; background: white; border-radius: 5px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  
  // Add password visibility toggle styles
  html += ".password-container { position: relative; }";
  html += ".password-toggle { position: absolute; right: 12px; top: 50%; transform: translateY(-50%); cursor: pointer; user-select: none; padding: 5px; }";
  html += ".password-toggle:hover { color: #4CAF50; }";
  html += ".eye-icon { width: 20px; height: 20px; fill: #666; }";
  html += ".eye-icon:hover { fill: #4CAF50; }";
  
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>Andon System Configuration</h1>";
  
  // Current switch counts display
  html += "<div class='current-counts'>";
  html += "<h3>Current Switch Activations:</h3>";
  for (int i = 0; i < 8; i++) {
    switchState[i] = digitalRead(switchPins[i]);
    String status = (switchState[i] == LOW) ? "Active" : "Inactive";
    html += "<div class='count-item'><strong>Switch " + String(i+1) + ":</strong> " + String(switchCount[i]) + " (" + status + ")</div>";
  }
  html += "</div>";
  
  html += "<div class='info'>";
  html += "<strong>Device Information:</strong><br>";
  html += "MAC Address: " + WiFi.macAddress() + "<br>";
  html += "Current IP: " + WiFi.localIP().toString() + "<br>";
  html += "</div>";
  
  html += "<form method='POST' action='/save'>";
  
  html += "<h3>WiFi Configuration:</h3>";
  html += "<div class='form-group'>";
  html += "<label for='ssid'>WiFi Network Name (SSID):</label>";
  html += "<input type='text' name='ssid' value='" + ssid + "' placeholder='Enter WiFi network name (leave empty to keep current)'>";
  html += "</div>";
  
  html += "<div class='form-group'>";
  html += "<label for='password'>WiFi Password:</label>";
  html += "<div class='password-container'>";
  html += "<input type='password' name='password' id='password' value='" + password + "' placeholder='Enter WiFi password (leave empty to keep current)'>";
  html += "<span class='password-toggle' onclick='togglePassword()'>";
  // SVG eye icon
  html += "<svg class='eye-icon' id='eye-icon' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>";
  html += "<path id='eye-open' d='M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z'/>";
  html += "<path id='eye-closed' style='display:none' d='M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46C3.08 8.3 1.78 10.02 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z'/>";
  html += "</svg>";
  html += "</span>";
  html += "</div>";
  html += "</div>";
  
  html += "<div class='form-group'>";
  html += "<label for='staticIP'>Static IP Address:</label>";
  html += "<input type='text' name='staticIP' value='" + ip + "' placeholder='192.168.1.100 (leave empty for DHCP)'>";
  html += "</div>";
  
  html += "<div style='background-color: #fff3cd; padding: 15px; border-radius: 5px; margin: 20px 0; border-left: 4px solid #ffc107;'>";
  html += "<strong>Configuration Notes:</strong><br>";
  html += " Leave fields empty to keep current settings<br>";
  html += " Device will restart after saving changes<br>";
  html += " If WiFi connection fails, device returns to AP mode";
  html += "</div>";
  
  html += "<input type='submit' value='Save Configuration' class='btn'>";
  html += "<a href='/ota' class='btn btn-secondary'>Firmware Update</a>";
  html += "<button type='button' onclick='location.reload()' class='btn btn-secondary'>Refresh Data</button>";
  html += "</form>";
  
  html += "</div>";
  
  // JavaScript for password toggle
  html += "<script>";
  html += "function togglePassword() {";
  html += "  var passwordInput = document.getElementById('password');";
  html += "  var eyeOpen = document.getElementById('eye-open');";
  html += "  var eyeClosed = document.getElementById('eye-closed');";
  html += "  if (passwordInput.type === 'password') {";
  html += "    passwordInput.type = 'text';";
  html += "    eyeOpen.style.display = 'none';";
  html += "    eyeClosed.style.display = 'block';";
  html += "  } else {";
  html += "    passwordInput.type = 'password';";
  html += "    eyeOpen.style.display = 'block';";
  html += "    eyeClosed.style.display = 'none';";
  html += "  }";
  html += "}";
  html += "</script>";
  
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

// Enhanced Handle configuration save
void handleSave() {
  // Check if any form parameter was submitted
  if (webServer.args() > 0) {
    
    bool wifi_changes_made = false;
    
    // Handle SSID independently
    if (webServer.hasArg("ssid")) {
      String new_ssid = urlDecode(webServer.arg("ssid"));
      if (new_ssid != "" && new_ssid != ssid) {
        ssid = new_ssid;
        wifiPrefs.putString("ssid", ssid);
        wifi_changes_made = true;
        Serial.println("SSID updated: " + ssid);
      } else if (new_ssid == "" && ssid != "") {
        // Clear SSID if field is empty
        ssid = "";
        wifiPrefs.remove("ssid");
        wifi_changes_made = true;
        Serial.println("SSID cleared");
      }
    }
    
    // Handle Password independently
    if (webServer.hasArg("password")) {
      String new_password = urlDecode(webServer.arg("password"));
      if (new_password != "" && new_password != password) {
        password = new_password;
        wifiPrefs.putString("password", password);
        wifi_changes_made = true;
        Serial.println("Password updated");
      } else if (new_password == "" && password != "") {
        // Clear password if field is empty
        password = "";
        wifiPrefs.remove("password");
        wifi_changes_made = true;
        Serial.println("Password cleared");
      }
    }
    
    // Handle Static IP independently
    if (webServer.hasArg("staticIP")) {
      String new_ip = urlDecode(webServer.arg("staticIP"));
      if (new_ip != "" && new_ip != ip) {
        ip = new_ip;
        wifiPrefs.putString("staticIP", ip);
        wifi_changes_made = true;
        Serial.println("Static IP updated: " + ip);
      } else if (new_ip == "" && ip != "") {
        // Clear static IP if field is empty
        ip = "";
        wifiPrefs.remove("staticIP");
        wifi_changes_made = true;
        Serial.println("Static IP cleared");
      }
    }
    
    // Set flash flag to indicate data is saved (only if changes made)
    if (wifi_changes_made) {
      wifiPrefs.putString("wifi_saved", "saved");
      Serial.println("WiFi configuration changes saved to flash");
    }
    
    // Response HTML
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Configuration Update</title>";
    html += "<meta http-equiv='refresh' content='3;url=/'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; text-align: center; padding: 50px; background-color: #f0f0f0; }";
    html += ".container { max-width: 400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #4CAF50; }";
    html += ".no-change { color: #ffc107; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    
    if (wifi_changes_made) {
      html += "<h1>Configuration Updated!</h1>";
      html += "<p>The device will restart and apply the new settings.</p>";
      html += "<p>Redirecting in 3 seconds...</p>";
    } else {
      html += "<h1 class='no-change'>No Changes Detected</h1>";
      html += "<p>No configuration changes were made.</p>";
      html += "<p>Redirecting in 3 seconds...</p>";
    }
    
    html += "</div></body></html>";
    
    webServer.send(200, "text/html", html);
    
    // Only restart if changes were made
    if (wifi_changes_made) {
      delay(2000);
      ESP.restart();
    }
    
  } else {
    webServer.send(400, "text/plain", "No form data received");
  }
}

// Main dashboard page
void handleMain() {
  // Display the HTML web page for the main dashboard
  String html = "<!DOCTYPE html>";
  html += "<html lang='en'>";
  html += "<head>";
  html += "<meta charset='UTF-8'>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<title>Andon Signaling Monitor</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; line-height: 1.6; margin: 0; padding: 0; background-color: #f0f8ff; color: #333; }";
  html += ".container { max-width: 900px; margin: 0 auto; padding: 20px; }";
  html += "header { background-color: #0066cc; color: white; padding: 15px 0; text-align: center; margin-bottom: 20px; }";
  html += "h1 { margin: 0; }";
  html += ".dashboard { display: grid; grid-template-columns: repeat(auto-fit, minmax(200px, 1fr)); gap: 20px; margin-bottom: 20px; }";
  html += ".switch-card { background-color: white; border-radius: 8px; padding: 15px; box-shadow: 0 4px 6px rgba(0,0,0,0.1); text-align: center; }";
  html += ".switch-number { font-size: 18px; font-weight: bold; margin-bottom: 10px; color: #0066cc; }";
  html += ".status { display: inline-block; width: 20px; height: 20px; border-radius: 50%; margin-right: 10px; vertical-align: middle; }";
  html += ".active { background-color: #4CAF50; }";
  html += ".inactive { background-color: #ccc; }";
  html += ".count { font-size: 24px; font-weight: bold; margin: 10px 0; color: #333; }";
  html += ".controls { display: flex; justify-content: center; gap: 10px; margin-top: 20px; }";
  html += ".button { display: inline-block; padding: 10px 20px; background-color: #0066cc; color: white; text-decoration: none; border-radius: 4px; font-weight: bold; transition: background-color 0.3s; }";
  html += ".reset-btn { background-color: #f44336; }";
  html += ".config-btn { background-color: #4CAF50; }";
  html += ".ota-btn { background-color: #ff9800; }";
  html += ".button:hover { opacity: 0.9; }";
  html += ".footer { text-align: center; margin-top: 20px; padding-top: 10px; border-top: 1px solid #ddd; font-size: 14px; color: #666; }";
  html += ".device-info { background-color: #e7f3fe; border-left: 4px solid #2196F3; margin-bottom: 20px; padding: 10px; border-radius: 4px; }";
  html += "@media (max-width: 600px) { .dashboard { grid-template-columns: 1fr 1fr; } }";
  html += "</style>";
  html += "</head>";
  html += "<body>";
  html += "<header>";
  html += "<h1>Andon Signaling Monitor</h1>";
  html += "</header>";
  html += "<div class='container'>";
  html += "<div class='device-info'>";
  
  // Display connection status
  if (MODE) {
    html += "<p><strong>Connection Status:</strong> Connected to WiFi (" + String(ssid) + ")</p>";
    html += "<p><strong>IP Address:</strong> " + WiFi.localIP().toString() + "</p>";
  } else {
    html += "<p><strong>Connection Status:</strong> Access Point Mode</p>";
    html += "<p><strong>SSID:</strong> " + String(ssid_h) + " (Password: " + String(password_h) + ")</p>";
    html += "<p><strong>IP Address:</strong> " + WiFi.softAPIP().toString() + "</p>";
  }
  
  html += "</div>";
  html += "<div class='dashboard'>";
  
  // Generate switch cards
  for (int i = 0; i < 8; i++) {
    switchState[i] = digitalRead(switchPins[i]);
    html += "<div class='switch-card'>";
    html += "<div class='switch-number'>Switch " + String(i + 1) + "</div>";
    if (switchState[i] == LOW) {
      html += "<p><span class='status active'></span>Active</p>";
    } else {
      html += "<p><span class='status inactive'></span>Inactive</p>";
    }
    html += "<div class='count'>" + String(switchCount[i]) + "</div>";
    html += "<p>Activations</p>";
    html += "</div>";
  }
  
  html += "</div>";
  html += "<div class='controls'>";
  html += "<a href='/reset' class='button reset-btn'>Reset Counters</a>";
  html += "<a href='/login' class='button config-btn'>WiFi Configuration</a>";
  html += "<a href='/ota' class='button ota-btn'>Firmware Update</a>";
  html += "</div>";
  html += "<div class='footer'>";
  html += "<p>Andon Signaling Monitor v2.0 with OTA</p>";
  html += "</div>";
  html += "</div>";
  
  html += "<script>";
  html += "function refreshData() {";
  html += "  fetch('/data')";
  html += "    .then(response => response.text())";
  html += "    .then(data => {";
  html += "      console.log(data);";
  html += "      const values = data.replace('{', '').replace('}', '').split(',');";
  html += "      for (let i = 0; i < 8; i++) {";
  html += "        const statusElement = document.querySelectorAll('.status')[i];";
  html += "        const countElement = document.querySelectorAll('.count')[i];";
  html += "        const statusTextElement = statusElement.nextSibling;";
  html += "        if (values[i*2] == '0') {";
  html += "          statusElement.className = 'status active';";
  html += "          statusTextElement.textContent = 'Active';";
  html += "        } else {";
  html += "          statusElement.className = 'status inactive';";
  html += "          statusTextElement.textContent = 'Inactive';";
  html += "        }";
  html += "        countElement.textContent = values[i*2+1];";
  html += "      }";
  html += "    });";
  html += "}";
  html += "setInterval(refreshData, 1000);";
  html += "</script>";
  
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}