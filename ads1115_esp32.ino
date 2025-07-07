// #include <Wire.h>
// #include <Adafruit_ADS1X15.h>  // Supports the ADS1115
// #include <LiquidCrystal_I2C.h>
// #include <math.h>              // For round()

// // Create an ADS1115 object (default I2C address is 0x48)
// Adafruit_ADS1115 ads;

// double roundToStep(double value, double step) {
//   return step * round(value / step);
// }

// void setup() {
//   Serial.begin(115200);
//   // Initialize I2C with specific pins for ESP32 (change if needed)
//   Wire.begin(21, 22);

//   // Initialize the ADS1115
//   if (!ads.begin()) {
//     Serial.println("Failed to initialize ADS.");
//     while (1);
//   }
//   // Optionally, set the gain (e.g., ads.setGain(GAIN_ONE);)
//   // For GAIN_ONE, full-scale range is ±4.096V.

// }

// void loop() {
//   // ---------------------------
//   // 1. Read the voltage from the ADS1115
//   // ---------------------------
//   int16_t adcReading = ads.readADC_SingleEnded(0);
//   int16_t adcReading_1 = ads.readADC_SingleEnded(1);
//   int16_t adcReading_2 = ads.readADC_SingleEnded(2);
//   int16_t adcReading_3 = ads.readADC_SingleEnded(3);
//   double conversionFactor = 0.125; // mV per count (for ADS1115 GAIN_ONE)
//   double voltage = adcReading * conversionFactor; // in mV
//   double voltage_1 = adcReading_1 * conversionFactor; // in mV
//   double voltage_2 = adcReading_2 * conversionFactor; // in mV
//   double voltage_3 = adcReading_3 * conversionFactor; // in mV

//   Serial.print("ADC reading: ");
//   Serial.print(adcReading);
//   Serial.print(" -> Voltage: ");
//   Serial.println(voltage, 2);
//   Serial.println(voltage_1, 2);
//   Serial.println(voltage_2, 2);
//   Serial.println(voltage_3, 2);
//   delay(2000);

// }




//------------- ads voltages in web server



// #include <Wire.h>
// #include <Adafruit_ADS1X15.h>
// #include <WiFi.h>
// #include <WebServer.h>

// // Create ADS1115 object
// Adafruit_ADS1115 ads;

// // ADC voltages
// float voltage[4];

// // Set up web server
// WebServer server(80);

// void handleRoot() {
//   String html = "<html><head><title>ESP32 ADS1115 Readings</title></head><body>";
//   html += "<h2>ADS1115 Voltage Readings</h2>";
//   for (int i = 0; i < 4; i++) {
//     html += "Channel " + String(i) + ": " + String(voltage[i], 3) + " V<br>";
//   }
//   html += "</body></html>";
//   server.send(200, "text/html", html);
// }

// void setup() {
//   Serial.begin(115200);
//   Wire.begin(21, 22);  // SDA, SCL

//   // Initialize ADS1115
//   if (!ads.begin()) {
//     Serial.println("Failed to initialize ADS1115.");
//     while (1);
//   }
//   //ads.setGain(GAIN_ONE);  // ±4.096V range, 125 µV per bit
//   ads.setGain(GAIN_TWOTHIRDS);  /// ±6.144V

//   // SoftAP Setup
//   WiFi.softAP("ESP32-ADS1115", "12345678");
//   Serial.println("SoftAP started. Connect to WiFi SSID: ESP32-ADS1115");

//   // Web server routes
//   server.on("/", handleRoot);
//   server.begin();
//   Serial.println("Web server started");
// }

// void loop() {
//   for (int i = 0; i < 4; i++) {
//     int16_t adc = ads.readADC_SingleEnded(i);
//     //voltage[i] = adc * 0.000125;  // 125 µV per bit 
//     voltage[i] = adc * 0.0001875;  //voltages[i] = val * 0.0001875;
//     Serial.print("CH");
//     Serial.print(i);
//     Serial.print(": ");
//     Serial.print(voltage[i], 3);
//     Serial.println(" V");
//   }
//   Serial.println("-----");
//   server.handleClient();
//   delay(2000);
// }


// //-----------------   wifi details page
// #include <WiFi.h>
// #include <WebServer.h>
// #include <WebSocketsServer.h>
// #include <Preferences.h>
// #include <Wire.h>
// #include <Adafruit_ADS1X15.h>

// Adafruit_ADS1115 ads;
// WebServer server(80);
// WebSocketsServer webSocket = WebSocketsServer(81);
// Preferences preferences;

// // SoftAP Configuration
// const char* softAP_SSID = "ADS-channels";
// const char* softAP_Password = "12345678";

// // WiFi info
// String ssid, password, ipStr, gatewayStr, subnetStr;
// IPAddress local_IP, gateway, subnet;
// float voltages[4] = {0.0, 0.0, 0.0, 0.0}; // Initialize with default values

// // Connection attempts
// const int MAX_WIFI_RETRIES = 20;
// bool softAPMode = false;

// void eraseFlashMemory() {
//   Serial.println("Erasing entire flash memory...");
//   preferences.begin("wifi", false);
//   preferences.clear();
//   preferences.end();
//   Serial.println("Flash memory erased successfully");
// }

// void saveWiFiSettings(String ssid, String password, String ip, String gateway, String subnet) {
//   preferences.begin("wifi", false);
  
//   // Save all values, empty or not
//   preferences.putString("ssid", ssid);
//   preferences.putString("password", password);
//   preferences.putString("ip", ip);
//   preferences.putString("gateway", gateway);
//   preferences.putString("subnet", subnet);
  
//   preferences.end();
//   Serial.println("WiFi settings saved to flash memory");
// }

// void readWiFiSettings() {
//   preferences.begin("wifi", true);
//   ssid = preferences.getString("ssid", "");
//   password = preferences.getString("password", "");
//   ipStr = preferences.getString("ip", "");
//   gatewayStr = preferences.getString("gateway", "");
//   subnetStr = preferences.getString("subnet", "");
//   preferences.end();
  
//   // Print current settings from flash
//   Serial.println("=== WiFi Settings from Flash ===");
//   Serial.println("SSID: " + (ssid.length() > 0 ? ssid : "Not set"));
//   Serial.print("Password: ");
//   Serial.println(password.length() > 0 ? "***********" : "Not set");
//   Serial.println("Static IP: " + (ipStr.length() > 0 ? ipStr : "Not set"));
//   Serial.println("Gateway: " + (gatewayStr.length() > 0 ? gatewayStr : "Not set"));
//   Serial.println("Subnet: " + (subnetStr.length() > 0 ? subnetStr : "Not set"));
//   Serial.println("================================");
// }

// void connectToWiFi() {
//   if (ssid.length() > 0 && password.length() > 0) {
//     // Configure static IP if provided
//     if (ipStr.length() > 0 && gatewayStr.length() > 0 && subnetStr.length() > 0) {
//       local_IP.fromString(ipStr);
//       gateway.fromString(gatewayStr);
//       subnet.fromString(subnetStr);
//       if (WiFi.config(local_IP, gateway, subnet)) {
//         Serial.println("Static IP configuration set");
//       } else {
//         Serial.println("Failed to configure static IP");
//       }
//     }
    
//     WiFi.begin(ssid.c_str(), password.c_str());
//     Serial.print("Connecting to WiFi: " + ssid);
    
//     int retries = 0;
//     while (WiFi.status() != WL_CONNECTED && retries < MAX_WIFI_RETRIES) {
//       delay(1000); // Changed to 1000ms as requested
//       Serial.print(".");
//       retries++;
//     }
//     Serial.println();
    
//     if (WiFi.status() == WL_CONNECTED) {
//       Serial.println("✓ Successfully connected to WiFi!");
//       Serial.println("IP Address: " + WiFi.localIP().toString());
//       Serial.println("Gateway: " + WiFi.gatewayIP().toString());
//       Serial.println("Subnet: " + WiFi.subnetMask().toString());
//       softAPMode = false;
//     } else {
//       Serial.println("✗ Failed to connect after " + String(MAX_WIFI_RETRIES) + " attempts");
//       Serial.println("Starting SoftAP mode...");
//       startSoftAP();
//     }
//   } else {
//     Serial.println("No WiFi credentials found in flash memory");
//     Serial.println("Starting SoftAP mode...");
//     startSoftAP();
//   }
// }

// void startSoftAP() {
//   softAPMode = true;
//   eraseFlashMemory(); // Erase flash when starting SoftAP
//   WiFi.softAP(softAP_SSID, softAP_Password);
//   Serial.println("SoftAP Started");
//   Serial.println("SSID: " + String(softAP_SSID));
//   Serial.println("Password: " + String(softAP_Password));
//   Serial.println("IP Address: " + WiFi.softAPIP().toString());
//   Serial.println("Connect to configure WiFi settings");
// }

// void handleRoot() {
//   String currentSSID = ssid.length() > 0 ? ssid : "Not configured";
//   String currentIP = ipStr.length() > 0 ? ipStr : "Not configured";
//   String currentGateway = gatewayStr.length() > 0 ? gatewayStr : "Not configured";
//   String currentSubnet = subnetStr.length() > 0 ? subnetStr : "Not configured";
  
//   String html = R"rawliteral(
// <!DOCTYPE html>
// <html lang="en">
// <head>
//     <meta charset="UTF-8">
//     <meta name="viewport" content="width=device-width, initial-scale=1.0">
//     <title>ESP32 WiFi Configuration</title>
//     <style>
//         * {
//             margin: 0;
//             padding: 0;
//             box-sizing: border-box;
//         }
        
//         body {
//             font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
//             background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
//             min-height: 100vh;
//             display: flex;
//             align-items: center;
//             justify-content: center;
//             padding: 20px;
//         }
        
//         .container {
//             background: white;
//             border-radius: 15px;
//             box-shadow: 0 20px 40px rgba(0,0,0,0.1);
//             padding: 40px;
//             width: 100%;
//             max-width: 500px;
//             animation: slideUp 0.5s ease-out;
//         }
        
//         @keyframes slideUp {
//             from {
//                 opacity: 0;
//                 transform: translateY(30px);
//             }
//             to {
//                 opacity: 1;
//                 transform: translateY(0);
//             }
//         }
        
//         .header {
//             text-align: center;
//             margin-bottom: 30px;
//         }
        
//         .header h1 {
//             color: #333;
//             font-size: 28px;
//             margin-bottom: 10px;
//         }
        
//         .header p {
//             color: #666;
//             font-size: 16px;
//         }
        
//         .form-group {
//             margin-bottom: 25px;
//             position: relative;
//         }
        
//         .form-group label {
//             display: block;
//             margin-bottom: 8px;
//             color: #333;
//             font-weight: 600;
//             font-size: 14px;
//         }
        
//         .current-value {
//             font-size: 12px;
//             color: #666;
//             font-style: italic;
//             margin-left: 5px;
//         }
        
//         .form-group input {
//             width: 100%;
//             padding: 12px 15px;
//             border: 2px solid #e1e5e9;
//             border-radius: 8px;
//             font-size: 16px;
//             transition: all 0.3s ease;
//             background: #f8f9fa;
//         }
        
//         .password-container {
//             position: relative;
//         }
        
//         .password-toggle {
//             position: absolute;
//             right: 15px;
//             top: 50%;
//             transform: translateY(-50%);
//             cursor: pointer;
//             color: #666;
//             font-size: 18px;
//             user-select: none;
//         }
        
//         .password-toggle:hover {
//             color: #333;
//         }
        
//         .form-group input:focus {
//             outline: none;
//             border-color: #667eea;
//             background: white;
//             box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
//         }
        
//         .form-group input:valid {
//             border-color: #28a745;
//         }
        
//         .section-title {
//             color: #667eea;
//             font-size: 18px;
//             font-weight: 600;
//             margin: 30px 0 15px 0;
//             padding-bottom: 10px;
//             border-bottom: 2px solid #e1e5e9;
//         }
        
//         .section-title:first-of-type {
//             margin-top: 0;
//         }
        
//         .submit-btn {
//             width: 100%;
//             padding: 15px;
//             background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
//             color: white;
//             border: none;
//             border-radius: 8px;
//             font-size: 16px;
//             font-weight: 600;
//             cursor: pointer;
//             transition: all 0.3s ease;
//             margin-top: 20px;
//         }
        
//         .submit-btn:hover {
//             transform: translateY(-2px);
//             box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
//         }
        
//         .submit-btn:active {
//             transform: translateY(0);
//         }
        
//         .current-settings {
//             background: #f8f9fa;
//             border-radius: 8px;
//             padding: 20px;
//             margin-bottom: 20px;
//             border-left: 4px solid #667eea;
//         }
        
//         .current-settings h3 {
//             color: #333;
//             margin-bottom: 15px;
//             font-size: 16px;
//         }
        
//         .current-settings p {
//             color: #666;
//             font-size: 14px;
//             margin-bottom: 5px;
//         }
        
//         .note {
//             background: #e3f2fd;
//             border: 1px solid #bbdefb;
//             border-radius: 8px;
//             padding: 15px;
//             margin-top: 20px;
//             color: #1976d2;
//             font-size: 14px;
//         }
        
//         @media (max-width: 600px) {
//             .container {
//                 padding: 20px;
//                 margin: 10px;
//             }
            
//             .header h1 {
//                 font-size: 24px;
//             }
//         }
//     </style>
// </head>
// <body>
//     <div class="container">
//         <div class="header">
//             <h1>🌐 WiFi Configuration</h1>
//             <p>Configure your ESP32 network settings</p>
//         </div>
        
//         <div class="current-settings">
//             <h3>📋 Current Settings</h3>
//             <p><strong>SSID:</strong> )rawliteral" + currentSSID + R"rawliteral(</p>
//             <p><strong>Static IP:</strong> )rawliteral" + currentIP + R"rawliteral(</p>
//             <p><strong>Gateway:</strong> )rawliteral" + currentGateway + R"rawliteral(</p>
//             <p><strong>Subnet:</strong> )rawliteral" + currentSubnet + R"rawliteral(</p>
//         </div>
        
//         <form action="/save" method="post">
//             <div class="section-title">
//                 🔐 WiFi Credentials
//             </div>
            
//             <div class="form-group">
//                 <label for="ssid">Network Name (SSID) <span class="current-value">Current: )rawliteral" + currentSSID + R"rawliteral(</span></label>
//                 <input type="text" id="ssid" name="ssid" placeholder="Enter WiFi network name" required>
//             </div>
            
//             <div class="form-group">
//                 <label for="password">WiFi Password <span class="current-value">)rawliteral" + (password.length() > 0 ? "Current: ***********" : "Current: Not set") + R"rawliteral(</span></label>
//                 <div class="password-container">
//                     <input type="password" id="password" name="password" placeholder="Enter WiFi password" required>
//                     <span class="password-toggle" onclick="togglePassword()">👁️</span>
//                 </div>
//             </div>
            
//             <div class="section-title">
//                 🌐 Network Configuration (Optional)
//             </div>
            
//             <div class="form-group">
//                 <label for="ip">Static IP Address <span class="current-value">Current: )rawliteral" + currentIP + R"rawliteral(</span></label>
//                 <input type="text" id="ip" name="ip" placeholder="e.g., 192.168.1.100">
//             </div>
            
//             <div class="form-group">
//                 <label for="gateway">Gateway Address <span class="current-value">Current: )rawliteral" + currentGateway + R"rawliteral(</span></label>
//                 <input type="text" id="gateway" name="gateway" placeholder="e.g., 192.168.1.1">
//             </div>
            
//             <div class="form-group">
//                 <label for="subnet">Subnet Mask <span class="current-value">Current: )rawliteral" + currentSubnet + R"rawliteral(</span></label>
//                 <input type="text" id="subnet" name="subnet" placeholder="e.g., 255.255.255.0">
//             </div>
            
//             <button type="submit" class="submit-btn">
//                 💾 Save Configuration & Restart
//             </button>
//         </form>
        
//         <div class="note">
//             <strong>📝 Note:</strong> Leave IP fields empty for automatic DHCP configuration. SSID and Password are required fields.
//         </div>
//     </div>
    
//     <script>
//         function togglePassword() {
//             const passwordField = document.getElementById('password');
//             const toggle = document.querySelector('.password-toggle');
            
//             if (passwordField.type === 'password') {
//                 passwordField.type = 'text';
//                 toggle.innerHTML = '🙈';
//             } else {
//                 passwordField.type = 'password';
//                 toggle.innerHTML = '👁️';
//             }
//         }
        
//         // Add form validation
//         document.querySelector('form').addEventListener('submit', function(e) {
//             const ssid = document.getElementById('ssid').value.trim();
//             const password = document.getElementById('password').value.trim();
            
//             if (ssid === '') {
//                 alert('Please enter a WiFi network name (SSID)');
//                 e.preventDefault();
//                 return false;
//             }
            
//             if (password === '') {
//                 alert('Please enter a WiFi password');
//                 e.preventDefault();
//                 return false;
//             }
            
//             // Show loading state
//             const btn = document.querySelector('.submit-btn');
//             btn.innerHTML = '⏳ Saving & Restarting...';
//             btn.disabled = true;
//         });
        
//         // Auto-focus SSID field
//         window.addEventListener('load', function() {
//             document.getElementById('ssid').focus();
//         });
//     </script>
// </body>
// </html>
//   )rawliteral";
//   server.send(200, "text/html", html);
// }

// void handleSave() {
//   String newSSID = server.arg("ssid");
//   String newPassword = server.arg("password");
//   String newIP = server.arg("ip");
//   String newGateway = server.arg("gateway");
//   String newSubnet = server.arg("subnet");
  
//   // Trim whitespace manually
//   newSSID.trim();
//   newPassword.trim();
//   newIP.trim();
//   newGateway.trim();
//   newSubnet.trim();

//   Serial.println("=== Saving New Settings ===");
//   Serial.println("New SSID: " + newSSID);
//   Serial.println("New Password: ***********");
//   if (newIP.length() > 0) Serial.println("New IP: " + newIP);
//   if (newGateway.length() > 0) Serial.println("New Gateway: " + newGateway);
//   if (newSubnet.length() > 0) Serial.println("New Subnet: " + newSubnet);

//   saveWiFiSettings(newSSID, newPassword, newIP, newGateway, newSubnet);
  
//   String html = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
//     <meta charset="UTF-8">
//     <meta name="viewport" content="width=device-width, initial-scale=1.0">
//     <title>Settings Saved</title>
//     <style>
//         body {
//             font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
//             background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
//             min-height: 100vh;
//             display: flex;
//             align-items: center;
//             justify-content: center;
//             margin: 0;
//             padding: 20px;
//         }
//         .container {
//             background: white;
//             border-radius: 15px;
//             padding: 40px;
//             text-align: center;
//             box-shadow: 0 20px 40px rgba(0,0,0,0.1);
//             max-width: 400px;
//             width: 100%;
//         }
//         .success-icon {
//             font-size: 60px;
//             margin-bottom: 20px;
//             animation: bounce 1s ease-out;
//         }
//         @keyframes bounce {
//             0%, 20%, 60%, 100% { transform: translateY(0); }
//             40% { transform: translateY(-20px); }
//             80% { transform: translateY(-10px); }
//         }
//         h1 {
//             color: #28a745;
//             margin-bottom: 20px;
//             font-size: 24px;
//         }
//         p {
//             color: #666;
//             margin-bottom: 20px;
//             font-size: 16px;
//         }
//         .spinner {
//             border: 3px solid #f3f3f3;
//             border-top: 3px solid #667eea;
//             border-radius: 50%;
//             width: 30px;
//             height: 30px;
//             animation: spin 1s linear infinite;
//             margin: 20px auto;
//         }
//         @keyframes spin {
//             0% { transform: rotate(0deg); }
//             100% { transform: rotate(360deg); }
//         }
//     </style>
// </head>
// <body>
//     <div class="container">
//         <div class="success-icon">✅</div>
//         <h1>Settings Saved Successfully!</h1>
//         <p>Your ESP32 is restarting with the new configuration...</p>
//         <div class="spinner"></div>
//         <p><small>This page will automatically redirect in 5 seconds</small></p>
//     </div>
//     <script>
//         setTimeout(function() {
//             window.location.href = '/';
//         }, 5000);
//     </script>
// </body>
// </html>
//   )rawliteral";
  
//   server.send(200, "text/html", html);
//   Serial.println("Restarting ESP32 in 3 seconds...");
//   delay(3000);
//   ESP.restart();
// }

// void handleDataPage() {
//   String html = R"rawliteral(
// <!DOCTYPE html>
// <html>
// <head>
//     <meta charset="UTF-8">
//     <meta name="viewport" content="width=device-width, initial-scale=1.0">
//     <title>ADS1115 Live Data</title>
//     <style>
//         body {
//             font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
//             background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
//             min-height: 100vh;
//             margin: 0;
//             padding: 20px;
//         }
//         .container {
//             max-width: 800px;
//             margin: 0 auto;
//             background: white;
//             border-radius: 15px;
//             padding: 30px;
//             box-shadow: 0 20px 40px rgba(0,0,0,0.1);
//         }
//         .header {
//             text-align: center;
//             margin-bottom: 30px;
//         }
//         .header h1 {
//             color: #333;
//             font-size: 28px;
//             margin-bottom: 10px;
//         }
//         .voltage-grid {
//             display: grid;
//             grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
//             gap: 20px;
//             margin-bottom: 30px;
//         }
//         .voltage-card {
//             background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
//             border-radius: 10px;
//             padding: 20px;
//             text-align: center;
//             border: 2px solid #e1e5e9;
//             transition: all 0.3s ease;
//         }
//         .voltage-card:hover {
//             transform: translateY(-5px);
//             box-shadow: 0 10px 20px rgba(0,0,0,0.1);
//         }
//         .channel-name {
//             font-size: 14px;
//             color: #666;
//             margin-bottom: 10px;
//             font-weight: 600;
//         }
//         .voltage-value {
//             font-size: 24px;
//             font-weight: bold;
//             color: #333;
//             margin-bottom: 5px;
//         }
//         .status {
//             padding: 10px;
//             border-radius: 8px;
//             margin-bottom: 20px;
//             text-align: center;
//         }
//         .status.connected {
//             background: #d4edda;
//             color: #155724;
//             border: 1px solid #c3e6cb;
//         }
//         .status.disconnected {
//             background: #f8d7da;
//             color: #721c24;
//             border: 1px solid #f5c6cb;
//         }
//         .nav-button {
//             display: inline-block;
//             padding: 12px 24px;
//             background: #667eea;
//             color: white;
//             text-decoration: none;
//             border-radius: 8px;
//             transition: all 0.3s ease;
//             font-weight: 600;
//             margin: 0 10px;
//         }
//         .nav-button:hover {
//             background: #5a6fd8;
//             transform: translateY(-2px);
//             box-shadow: 0 5px 15px rgba(102, 126, 234, 0.3);
//         }
//         .button-container {
//             text-align: center;
//             margin-top: 20px;
//         }
//     </style>
//     <script>
//         var ws;
//         var wsConnected = false;
        
//         function connectWebSocket() {
//             ws = new WebSocket("ws://" + location.hostname + ":81/");
            
//             ws.onopen = function() {
//                 wsConnected = true;
//                 document.getElementById("status").innerHTML = "🟢 Connected - Receiving live data";
//                 document.getElementById("status").className = "status connected";
//             };
            
//             ws.onmessage = function(event) {
//                 var data = JSON.parse(event.data);
//                 for (let i = 0; i < 4; i++) {
//                     document.getElementById("ch" + i).innerText = data[i].toFixed(3) + " V";
//                 }
//             };
            
//             ws.onclose = function() {
//                 wsConnected = false;
//                 document.getElementById("status").innerHTML = "🔴 Disconnected - Attempting to reconnect...";
//                 document.getElementById("status").className = "status disconnected";
//                 setTimeout(connectWebSocket, 3000);
//             };
            
//             ws.onerror = function() {
//                 wsConnected = false;
//                 document.getElementById("status").innerHTML = "🔴 Connection Error - Retrying...";
//                 document.getElementById("status").className = "status disconnected";
//             };
//         }
        
//         window.onload = function() {
//             connectWebSocket();
//         };
//     </script>
// </head>
// <body>
//     <div class="container">
//         <div class="header">
//             <h1>📊 ADS1115 Live Voltage Monitor</h1>
//             <div id="status" class="status disconnected">🔄 Connecting...</div>
//         </div>
        
//         <div class="voltage-grid">
//             <div class="voltage-card">
//                 <div class="channel-name">Channel 0</div>
//                 <div class="voltage-value" id="ch0">0.000 V</div>
//             </div>
//             <div class="voltage-card">
//                 <div class="channel-name">Channel 1</div>
//                 <div class="voltage-value" id="ch1">0.000 V</div>
//             </div>
//             <div class="voltage-card">
//                 <div class="channel-name">Channel 2</div>
//                 <div class="voltage-value" id="ch2">0.000 V</div>
//             </div>
//             <div class="voltage-card">
//                 <div class="channel-name">Channel 3</div>
//                 <div class="voltage-value" id="ch3">0.000 V</div>
//             </div>
//         </div>
        
//         <div class="button-container">
//             <a href="/" class="nav-button">⚙️ WiFi Configuration</a>
//         </div>
//     </div>
// </body>
// </html>
//   )rawliteral";
//   server.send(200, "text/html", html);
// }

// void setup() {
//   Serial.begin(115200);
//   Serial.println("\n=== ESP32 WiFi Configuration System ===");
  
//   Wire.begin(21, 22);
//   if (!ads.begin()) {
//     Serial.println("Failed to initialize ADS1115");
//   } else {
//     Serial.println("ADS1115 initialized successfully");
//   }
  
//   ads.setGain(GAIN_TWOTHIRDS);  // ±6.144V range
//   Serial.println("ADS1115 gain set to ±6.144V");

//   readWiFiSettings();
//   connectToWiFi();

//   // Setup web server routes
//   server.on("/", handleRoot);
//   server.on("/save", HTTP_POST, handleSave);
//   server.on("/data", handleDataPage);
//   server.begin();
//   Serial.println("Web server started on port 80");

//   // Setup WebSocket server
//   webSocket.begin();
//   webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
//     // Handle WebSocket events if needed
//     if (type == WStype_CONNECTED) {
//       Serial.println("WebSocket client connected: " + String(num));
//     } else if (type == WStype_DISCONNECTED) {
//       Serial.println("WebSocket client disconnected: " + String(num));
//     }
//   });
//   Serial.println("WebSocket server started on port 81");
  
//   Serial.println("=== Setup Complete ===");
//   if (WiFi.status() == WL_CONNECTED) {
//     Serial.println("Ready! Access the web interface at: http://" + WiFi.localIP().toString());
//     Serial.println("Live data page: http://" + WiFi.localIP().toString() + "/data");
//   } else {
//     Serial.println("Ready! Connect to SoftAP and access: http://192.168.4.1");
//   }
// }

// void loop() {
//   // Check WiFi connection status and reconnect if needed
//   if (!softAPMode && WiFi.status() != WL_CONNECTED) {
//     Serial.println("WiFi connection lost. Attempting to reconnect...");
//     connectToWiFi();
//   }
  
//   //Read ADS1115 channels
//   for (int i = 0; i < 4; i++) {
//     int16_t val = ads.readADC_SingleEnded(i);
//     voltages[i] = val * 0.0001875;  // 187.5 µV per bit for GAIN_TWOTHIRDS (±6.144V)
//   }

//   // Create JSON data for WebSocket
//   String json = "[";
//   for (int i = 0; i < 4; i++) {
//     json += String(voltages[i], 3);
//     if (i < 3) json += ",";
//   }
//   json += "]";
  
//   // Send data to all connected WebSocket clients
//   webSocket.broadcastTXT(json);

//   // Handle web server and WebSocket
//   server.handleClient();
//   webSocket.loop();
  
//   //delay(1000);  // Update every second
// }


//------------------------------- wifi with ota
//-----------------   wifi details page with OTA Update
#include <WiFi.h>
#include <WebServer.h>
#include <WebSocketsServer.h>
#include <Update.h>  // Added for OTA functionality
#include <Preferences.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>

Adafruit_ADS1115 ads;
WebServer server(80);
WebSocketsServer webSocket = WebSocketsServer(81);
Preferences preferences;

// SoftAP Configuration
const char* softAP_SSID = "ADS-channels";
const char* softAP_Password = "12345678";

// OTA Configuration
const char* ota_username = "admin";
const char* ota_password = "password";
bool updateInProgress = false;
size_t updateSize = 0;
size_t updateProgress = 0;

// WiFi info
String ssid, password, ipStr, gatewayStr, subnetStr;
IPAddress local_IP, gateway, subnet;
float voltages[4] = {0.0, 0.0, 0.0, 0.0}; // Initialize with default values

// Connection attempts
const int MAX_WIFI_RETRIES = 20;
bool softAPMode = false;

// Helper function to convert hex char to int
int hexToInt(char c) {
  if (c >= '0' && c <= '9') return c - '0';
  if (c >= 'A' && c <= 'F') return c - 'A' + 10;
  if (c >= 'a' && c <= 'f') return c - 'a' + 10;
  return 0;
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

void eraseFlashMemory() {
  Serial.println("Erasing entire flash memory...");
  preferences.begin("wifi", false);
  preferences.clear();
  preferences.end();
  Serial.println("Flash memory erased successfully");
}

void saveWiFiSettings(String ssid, String password, String ip, String gateway, String subnet) {
  preferences.begin("wifi", false);
  
  // Save all values, empty or not
  preferences.putString("ssid", ssid);
  preferences.putString("password", password);
  preferences.putString("ip", ip);
  preferences.putString("gateway", gateway);
  preferences.putString("subnet", subnet);
  
  preferences.end();
  Serial.println("WiFi settings saved to flash memory");
}

void readWiFiSettings() {
  preferences.begin("wifi", true);
  ssid = preferences.getString("ssid", "");
  password = preferences.getString("password", "");
  ipStr = preferences.getString("ip", "");
  gatewayStr = preferences.getString("gateway", "");
  subnetStr = preferences.getString("subnet", "");
  preferences.end();
  
  // Print current settings from flash
  Serial.println("=== WiFi Settings from Flash ===");
  Serial.println("SSID: " + (ssid.length() > 0 ? ssid : "Not set"));
  Serial.print("Password: ");
  Serial.println(password.length() > 0 ? "***********" : "Not set");
  Serial.println("Static IP: " + (ipStr.length() > 0 ? ipStr : "Not set"));
  Serial.println("Gateway: " + (gatewayStr.length() > 0 ? gatewayStr : "Not set"));
  Serial.println("Subnet: " + (subnetStr.length() > 0 ? subnetStr : "Not set"));
  Serial.println("================================");
}

void connectToWiFi() {
  if (ssid.length() > 0 && password.length() > 0) {
    // Configure static IP if provided
    if (ipStr.length() > 0 && gatewayStr.length() > 0 && subnetStr.length() > 0) {
      local_IP.fromString(ipStr);
      gateway.fromString(gatewayStr);
      subnet.fromString(subnetStr);
      if (WiFi.config(local_IP, gateway, subnet)) {
        Serial.println("Static IP configuration set");
      } else {
        Serial.println("Failed to configure static IP");
      }
    }
    
    WiFi.begin(ssid.c_str(), password.c_str());
    Serial.print("Connecting to WiFi: " + ssid);
    
    int retries = 0;
    while (WiFi.status() != WL_CONNECTED && retries < MAX_WIFI_RETRIES) {
      delay(1000);
      Serial.print(".");
      retries++;
    }
    Serial.println();
    
    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("✓ Successfully connected to WiFi!");
      Serial.println("IP Address: " + WiFi.localIP().toString());
      Serial.println("Gateway: " + WiFi.gatewayIP().toString());
      Serial.println("Subnet: " + WiFi.subnetMask().toString());
      softAPMode = false;
    } else {
      Serial.println("✗ Failed to connect after " + String(MAX_WIFI_RETRIES) + " attempts");
      Serial.println("Starting SoftAP mode...");
      startSoftAP();
    }
  } else {
    Serial.println("No WiFi credentials found in flash memory");
    Serial.println("Starting SoftAP mode...");
    startSoftAP();
  }
}

void startSoftAP() {
  softAPMode = true;
  eraseFlashMemory(); // Erase flash when starting SoftAP
  WiFi.softAP(softAP_SSID, softAP_Password);
  Serial.println("SoftAP Started");
  Serial.println("SSID: " + String(softAP_SSID));
  Serial.println("Password: " + String(softAP_Password));
  Serial.println("IP Address: " + WiFi.softAPIP().toString());
  Serial.println("Connect to configure WiFi settings");
}

void handleRoot() {
  String currentSSID = ssid.length() > 0 ? ssid : "Not configured";
  String currentIP = ipStr.length() > 0 ? ipStr : "Not configured";
  String currentGateway = gatewayStr.length() > 0 ? gatewayStr : "Not configured";
  String currentSubnet = subnetStr.length() > 0 ? subnetStr : "Not configured";
  
  String currentIPAddress = softAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ESP32 WiFi Configuration</title>
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            padding: 20px;
        }
        
        .container {
            background: white;
            border-radius: 15px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            padding: 40px;
            width: 100%;
            max-width: 500px;
            animation: slideUp 0.5s ease-out;
        }
        
        @keyframes slideUp {
            from {
                opacity: 0;
                transform: translateY(30px);
            }
            to {
                opacity: 1;
                transform: translateY(0);
            }
        }
        
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        
        .header h1 {
            color: #333;
            font-size: 28px;
            margin-bottom: 10px;
        }
        
        .header p {
            color: #666;
            font-size: 16px;
        }
        
        .form-group {
            margin-bottom: 25px;
            position: relative;
        }
        
        .form-group label {
            display: block;
            margin-bottom: 8px;
            color: #333;
            font-weight: 600;
            font-size: 14px;
        }
        
        .current-value {
            font-size: 12px;
            color: #666;
            font-style: italic;
            margin-left: 5px;
        }
        
        .form-group input {
            width: 100%;
            padding: 12px 15px;
            border: 2px solid #e1e5e9;
            border-radius: 8px;
            font-size: 16px;
            transition: all 0.3s ease;
            background: #f8f9fa;
        }
        
        .password-container {
            position: relative;
        }
        
        .password-toggle {
            position: absolute;
            right: 15px;
            top: 50%;
            transform: translateY(-50%);
            cursor: pointer;
            color: #666;
            font-size: 18px;
            user-select: none;
        }
        
        .password-toggle:hover {
            color: #333;
        }
        
        .form-group input:focus {
            outline: none;
            border-color: #667eea;
            background: white;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        
        .form-group input:valid {
            border-color: #28a745;
        }
        
        .section-title {
            color: #667eea;
            font-size: 18px;
            font-weight: 600;
            margin: 30px 0 15px 0;
            padding-bottom: 10px;
            border-bottom: 2px solid #e1e5e9;
        }
        
        .section-title:first-of-type {
            margin-top: 0;
        }
        
        .submit-btn {
            width: 100%;
            padding: 15px;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: white;
            border: none;
            border-radius: 8px;
            font-size: 16px;
            font-weight: 600;
            cursor: pointer;
            transition: all 0.3s ease;
            margin-top: 20px;
        }
        
        .submit-btn:hover {
            transform: translateY(-2px);
            box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
        }
        
        .submit-btn:active {
            transform: translateY(0);
        }
        
        .current-settings {
            background: #f8f9fa;
            border-radius: 8px;
            padding: 20px;
            margin-bottom: 20px;
            border-left: 4px solid #667eea;
        }
        
        .current-settings h3 {
            color: #333;
            margin-bottom: 15px;
            font-size: 16px;
        }
        
        .current-settings p {
            color: #666;
            font-size: 14px;
            margin-bottom: 5px;
        }
        
        .note {
            background: #e3f2fd;
            border: 1px solid #bbdefb;
            border-radius: 8px;
            padding: 15px;
            margin-top: 20px;
            color: #1976d2;
            font-size: 14px;
        }
        
        .nav-links {
            display: flex;
            justify-content: space-between;
            margin-top: 20px;
            gap: 10px;
        }
        
        .nav-link {
            flex: 1;
            text-align: center;
            padding: 10px;
            background: #667eea;
            color: white;
            text-decoration: none;
            border-radius: 6px;
            font-size: 14px;
            transition: all 0.3s ease;
        }
        
        .nav-link:hover {
            background: #5a6fd8;
            transform: translateY(-1px);
        }
        
        @media (max-width: 600px) {
            .container {
                padding: 20px;
                margin: 10px;
            }
            
            .header h1 {
                font-size: 24px;
            }
            
            .nav-links {
                flex-direction: column;
            }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>🌐 WiFi Configuration</h1>
            <p>Configure your ESP32 network settings</p>
        </div>
        
        <div class="current-settings">
            <h3>📋 Current Settings</h3>
            <p><strong>SSID:</strong> )rawliteral" + currentSSID + R"rawliteral(</p>
            <p><strong>Current IP:</strong> )rawliteral" + currentIPAddress + R"rawliteral(</p>
            <p><strong>Static IP:</strong> )rawliteral" + currentIP + R"rawliteral(</p>
            <p><strong>Gateway:</strong> )rawliteral" + currentGateway + R"rawliteral(</p>
            <p><strong>Subnet:</strong> )rawliteral" + currentSubnet + R"rawliteral(</p>
        </div>
        
        <form action="/save" method="post">
            <div class="section-title">
                🔐 WiFi Credentials
            </div>
            
            <div class="form-group">
                <label for="ssid">Network Name (SSID) <span class="current-value">Current: )rawliteral" + currentSSID + R"rawliteral(</span></label>
                <input type="text" id="ssid" name="ssid" placeholder="Enter WiFi network name" required>
            </div>
            
            <div class="form-group">
                <label for="password">WiFi Password <span class="current-value">)rawliteral" + (password.length() > 0 ? "Current: ***********" : "Current: Not set") + R"rawliteral(</span></label>
                <div class="password-container">
                    <input type="password" id="password" name="password" placeholder="Enter WiFi password" required>
                    <span class="password-toggle" onclick="togglePassword()">👁️</span>
                </div>
            </div>
            
            <div class="section-title">
                🌐 Network Configuration (Optional)
            </div>
            
            <div class="form-group">
                <label for="ip">Static IP Address <span class="current-value">Current: )rawliteral" + currentIP + R"rawliteral(</span></label>
                <input type="text" id="ip" name="ip" placeholder="e.g., 192.168.1.100">
            </div>
            
            <div class="form-group">
                <label for="gateway">Gateway Address <span class="current-value">Current: )rawliteral" + currentGateway + R"rawliteral(</span></label>
                <input type="text" id="gateway" name="gateway" placeholder="e.g., 192.168.1.1">
            </div>
            
            <div class="form-group">
                <label for="subnet">Subnet Mask <span class="current-value">Current: )rawliteral" + currentSubnet + R"rawliteral(</span></label>
                <input type="text" id="subnet" name="subnet" placeholder="e.g., 255.255.255.0">
            </div>
            
            <button type="submit" class="submit-btn">
                💾 Save Configuration & Restart
            </button>
        </form>
        
        <div class="nav-links">
            <a href="/data" class="nav-link">📊 Live Data</a>
            <a href="/update" class="nav-link">🔄 OTA Update</a>
        </div>
        
        <div class="note">
            <strong>📝 Note:</strong> Leave IP fields empty for automatic DHCP configuration. SSID and Password are required fields.
        </div>
    </div>
    
    <script>
        function togglePassword() {
            const passwordField = document.getElementById('password');
            const toggle = document.querySelector('.password-toggle');
            
            if (passwordField.type === 'password') {
                passwordField.type = 'text';
                toggle.innerHTML = '🙈';
            } else {
                passwordField.type = 'password';
                toggle.innerHTML = '👁️';
            }
        }
        
        // Add form validation
        document.querySelector('form').addEventListener('submit', function(e) {
            const ssid = document.getElementById('ssid').value.trim();
            const password = document.getElementById('password').value.trim();
            
            if (ssid === '') {
                alert('Please enter a WiFi network name (SSID)');
                e.preventDefault();
                return false;
            }
            
            if (password === '') {
                alert('Please enter a WiFi password');
                e.preventDefault();
                return false;
            }
            
            // Show loading state
            const btn = document.querySelector('.submit-btn');
            btn.innerHTML = '⏳ Saving & Restarting...';
            btn.disabled = true;
        });
        
        // Auto-focus SSID field
        window.addEventListener('load', function() {
            document.getElementById('ssid').focus();
        });
    </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void handleSave() {
  String newSSID = server.arg("ssid");
  String newPassword = server.arg("password");
  String newIP = server.arg("ip");
  String newGateway = server.arg("gateway");
  String newSubnet = server.arg("subnet");
  
  // Trim whitespace manually
  newSSID.trim();
  newPassword.trim();
  newIP.trim();
  newGateway.trim();
  newSubnet.trim();
  
  Serial.println("=== Saving New Settings ===");
  Serial.println("New SSID: " + newSSID);
  Serial.println("New Password: ***********");
  if (newIP.length() > 0) Serial.println("New IP: " + newIP);
  if (newGateway.length() > 0) Serial.println("New Gateway: " + newGateway);
  if (newSubnet.length() > 0) Serial.println("New Subnet: " + newSubnet);
  
  saveWiFiSettings(newSSID, newPassword, newIP, newGateway, newSubnet);
  
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Settings Saved</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            display: flex;
            align-items: center;
            justify-content: center;
            margin: 0;
            padding: 20px;
        }
        .container {
            background: white;
            border-radius: 15px;
            padding: 40px;
            text-align: center;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
            max-width: 400px;
            width: 100%;
        }
        .success-icon {
            font-size: 60px;
            margin-bottom: 20px;
            animation: bounce 1s ease-out;
        }
        @keyframes bounce {
            0%, 20%, 60%, 100% { transform: translateY(0); }
            40% { transform: translateY(-20px); }
            80% { transform: translateY(-10px); }
        }
        h1 {
            color: #28a745;
            margin-bottom: 20px;
            font-size: 24px;
        }
        p {
            color: #666;
            margin-bottom: 20px;
            font-size: 16px;
        }
        .spinner {
            border: 3px solid #f3f3f3;
            border-top: 3px solid #667eea;
            border-radius: 50%;
            width: 30px;
            height: 30px;
            animation: spin 1s linear infinite;
            margin: 20px auto;
        }
        @keyframes spin {
            0% { transform: rotate(0deg); }
            100% { transform: rotate(360deg); }
        }
    </style>
</head>
<body>
    <div class="container">
        <div class="success-icon">✅</div>
        <h1>Settings Saved Successfully!</h1>
        <p>Your ESP32 is restarting with the new configuration...</p>
        <div class="spinner"></div>
        <p><small>This page will automatically redirect in 5 seconds</small></p>
    </div>
    <script>
        setTimeout(function() {
            window.location.href = '/';
        }, 5000);
    </script>
</body>
</html>
  )rawliteral";
  
  server.send(200, "text/html", html);
  Serial.println("Restarting ESP32 in 3 seconds...");
  delay(3000);
  ESP.restart();
}

void handleDataPage() {
  String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>ADS1115 Live Data</title>
    <style>
        body {
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            min-height: 100vh;
            margin: 0;
            padding: 20px;
        }
        .container {
            max-width: 800px;
            margin: 0 auto;
            background: white;
            border-radius: 15px;
            padding: 30px;
            box-shadow: 0 20px 40px rgba(0,0,0,0.1);
        }
        .header {
            text-align: center;
            margin-bottom: 30px;
        }
        .header h1 {
            color: #333;
            font-size: 28px;
            margin-bottom: 10px;
        }
        .voltage-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(180px, 1fr));
            gap: 20px;
            margin-bottom: 30px;
        }
        .voltage-card {
            background: linear-gradient(135deg, #f8f9fa 0%, #e9ecef 100%);
            border-radius: 10px;
            padding: 20px;
            text-align: center;
            border: 2px solid #e1e5e9;
            transition: all 0.3s ease;
        }
        .voltage-card:hover {
            transform: translateY(-5px);
            box-shadow: 0 10px 20px rgba(0,0,0,0.1);
        }
        .channel-name {
            font-size: 14px;
            color: #666;
            margin-bottom: 10px;
            font-weight: 600;
        }
        .voltage-value {
            font-size: 24px;
            font-weight: bold;
            color: #333;
            margin-bottom: 5px;
        }
        .status {
            padding: 10px;
            border-radius: 8px;
            margin-bottom: 20px;
            text-align: center;
        }
        .status.connected {
            background: #d4edda;
            color: #155724;
            border: 1px solid #c3e6cb;
        }
        .status.disconnected {
            background: #f8d7da;
            color: #721c24;
            border: 1px solid #f5c6cb;
        }
        .nav-button {
            display: inline-block;
            padding: 12px 24px;
            background: #667eea;
            color: white;
            text-decoration: none;
            border-radius: 8px;
            transition: all 0.3s ease;
            font-weight: 600;
            margin: 0 10px;
        }
        .nav-button:hover {
            background: #5a6fd8;
            transform: translateY(-2px);
            box-shadow: 0 5px 15px rgba(102, 126, 234, 0.3);
        }
        .button-container {
            text-align: center;
            margin-top: 20px;
        }
    </style>
    <script>
        var ws;
        var wsConnected = false;
        
        function connectWebSocket() {
            ws = new WebSocket("ws://" + location.hostname + ":81/");
            
            ws.onopen = function() {
                wsConnected = true;
                document.getElementById("status").innerHTML = "🟢 Connected - Receiving live data";
                document.getElementById("status").className = "status connected";
            };
            
            ws.onmessage = function(event) {
                var data = JSON.parse(event.data);
                for (let i = 0; i < 4; i++) {
                    document.getElementById("ch" + i).innerText = data[i].toFixed(3) + " V";
                }
            };
            
            ws.onclose = function() {
                wsConnected = false;
                document.getElementById("status").innerHTML = "🔴 Disconnected - Attempting to reconnect...";
                document.getElementById("status").className = "status disconnected";
                setTimeout(connectWebSocket, 3000);
            };
            
            ws.onerror = function() {
                wsConnected = false;
                document.getElementById("status").innerHTML = "🔴 Connection Error - Retrying...";
                document.getElementById("status").className = "status disconnected";
            };
        }
        
        window.onload = function() {
            connectWebSocket();
        };
    </script>
</head>
<body>
    <div class="container">
        <div class="header">
            <h1>📊 ADS1115 Live Voltage Monitor</h1>
            <div id="status" class="status disconnected">🔄 Connecting...</div>
        </div>
        
        <div class="voltage-grid">
            <div class="voltage-card">
                <div class="channel-name">Channel 0</div>
                <div class="voltage-value" id="ch0">0.000 V</div>
            </div>
            <div class="voltage-card">
                <div class="channel-name">Channel 1</div>
                <div class="voltage-value" id="ch1">0.000 V</div>
            </div>
            <div class="voltage-card">
                <div class="channel-name">Channel 2</div>
                <div class="voltage-value" id="ch2">0.000 V</div>
            </div>
            <div class="voltage-card">
                <div class="channel-name">Channel 3</div>
                <div class="voltage-value" id="ch3">0.000 V</div>
            </div>
        </div>
        
        <div class="button-container">
            <a href="/login" class="nav-button">⚙️ WiFi Configuration</a>
            <a href="/update" class="nav-button">🔄 OTA Update</a>
        </div>
    </div>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

// OTA Update Page Handler
void handleOTAPage() {
  if (!server.authenticate(ota_username, ota_password)) {
    return server.requestAuthentication();
  }
  
  String currentIPAddress = softAPMode ? WiFi.softAPIP().toString() : WiFi.localIP().toString();
  
  String html = R"rawliteral(
<!DOCTYPE html>
<html lang="en">
<head>
  <meta charset="UTF-8">
  <meta name="viewport" content="width=device-width, initial-scale=1.0">
  <title>ESP32 OTA Update</title>
  <style>
    body {
      font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      min-height: 100vh;
      margin: 0;
      padding: 20px;
      display: flex;
      align-items: center;
      justify-content: center;
    }
    .container {
      background: white;
      border-radius: 15px;
      padding: 40px;
      box-shadow: 0 20px 40px rgba(0,0,0,0.1);
      max-width: 500px;
      width: 100%;
    }
    h1 {
      color: #333;
      margin-top: 0;
      text-align: center;
      font-size: 28px;
      margin-bottom: 10px;
    }
    .form-group {
      margin-bottom: 20px;
    }
    .btn {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
      border: none;
      padding: 15px 25px;
      border-radius: 8px;
      cursor: pointer;
      font-size: 16px;
      font-weight: 600;
      transition: all 0.3s ease;
      width: 100%;
    }
    .btn:hover {
      transform: translateY(-2px);
      box-shadow: 0 10px 20px rgba(102, 126, 234, 0.3);
    }
    .btn:disabled {
      background: #95a5a6;
      cursor: not-allowed;
      transform: none;
      box-shadow: none;
    }
    #upload-form {
      margin-top: 20px;
    }
    .file-input {
      margin-bottom: 20px;
      width: 100%;
      padding: 12px;
      border: 2px dashed #667eea;
      border-radius: 8px;
      background: #f8f9fa;
      text-align: center;
      transition: all 0.3s ease;
    }
    .file-input:hover {
      border-color: #5a6fd8;
      background: #e3f2fd;
    }
    #progress-container {
      display: none;
      margin-top: 25px;
    }
    .progress-bar {
      height: 25px;
      background-color: #ecf0f1;
      border-radius: 12px;
      margin-bottom: 15px;
      overflow: hidden;
      box-shadow: inset 0 2px 4px rgba(0,0,0,0.1);
    }
    .progress-fill {
      height: 100%;
      background: linear-gradient(135deg, #2ecc71 0%, #27ae60 100%);
      width: 0%;
      transition: width 0.3s ease;
      display: flex;
      align-items: center;
      justify-content: center;
      color: white;
      font-weight: bold;
      font-size: 12px;
    }
    .status {
      font-weight: bold;
      text-align: center;
      padding: 10px;
      border-radius: 8px;
      margin-bottom: 15px;
    }
    .success {
      background: #d4edda;
      color: #155724;
      border: 1px solid #c3e6cb;
    }
    .error {
      background: #f8d7da;
      color: #721c24;
      border: 1px solid #f5c6cb;
    }
    .info {
      background: #e3f2fd;
      border: 1px solid #bbdefb;
      border-radius: 8px;
      padding: 20px;
      margin-bottom: 25px;
      color: #1976d2;
      font-size: 14px;
    }
    .device-info {
      background: #f8f9fa;
      border-radius: 8px;
      padding: 20px;
      margin-top: 30px;
      font-size: 14px;
      color: #666;
    }
    .device-info p {
      margin: 8px 0;
    }
    .back-link {
      display: inline-block;
      margin-top: 25px;
      text-align: center;
      color: #667eea;
      text-decoration: none;
      font-weight: 600;
      width: 100%;
      padding: 12px;
      border: 2px solid #667eea;
      border-radius: 8px;
      transition: all 0.3s ease;
    }
    .back-link:hover {
      background: #667eea;
      color: white;
      transform: translateY(-1px);
    }
    .ota-status {
      padding: 15px;
      border-radius: 8px;
      margin-bottom: 20px;
      font-weight: bold;
      text-align: center;
      font-size: 16px;
    }
    .ota-active {
      background: linear-gradient(135deg, #2ecc71 0%, #27ae60 100%);
      color: white;
      animation: pulse 2s infinite;
    }
    .ota-inactive {
      background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
      color: white;
    }
    @keyframes pulse {
      0% { opacity: 1; }
      50% { opacity: 0.7; }
      100% { opacity: 1; }
    }
    input[type="file"] {
      width: 100%;
      padding: 15px;
      border: 2px dashed #667eea;
      border-radius: 8px;
      background: #f8f9fa;
      cursor: pointer;
      transition: all 0.3s ease;
    }
    input[type="file"]:hover {
      border-color: #5a6fd8;
      background: #e3f2fd;
    }
  </style>
</head>
<body>
  <div class="container">
    <h1>🔄 ESP32 OTA Update</h1>
    
    <div class="ota-status ota-inactive">🚀 OTA Update Ready</div>
    
    <div class="info">
      <p><strong>📁 Upload Instructions:</strong></p>
      <p>• Select a compiled firmware file (.bin format)</p>
      <p>• File will be uploaded and flashed automatically</p>
      <p>• Device will restart after successful update</p>
      <p>• Do not disconnect power during the update process</p>
    </div>
    
    <form id="upload-form" method="POST" action="/update" enctype="multipart/form-data">
      <div class="form-group">
        <input type="file" name="update" id="firmware-file" accept=".bin" required>
      </div>
      <button type="submit" class="btn" id="upload-btn">🚀 Upload & Update Firmware</button>
    </form>
    
    <div id="progress-container">
      <p class="status" id="status-text">Uploading firmware...</p>
      <div class="progress-bar">
        <div class="progress-fill" id="progress">0%</div>
      </div>
      <p id="progress-text" style="text-align: center; color: #666;">0%</p>
    </div>

    <div class="device-info">
      <p><strong>📟 Device Information:</strong></p>
      <p>Device: ESP32 ADS1115 Module</p>
      <p>Current IP: )rawliteral" + currentIPAddress + R"rawliteral(</p>
      <p>Mode: )rawliteral" + (softAPMode ? "SoftAP" : "WiFi Station") + R"rawliteral(</p>
      <p>Free Heap: <span id="heap">Loading...</span> KB</p>
    </div>
    
    <a href="/" class="back-link">⬅️ Back to WiFi Configuration</a>
    <a href="/data" class="back-link" style="margin-top: 10px;">📊 View Live Data</a>
  </div>

  <script>
    // Update heap info
    function updateHeapInfo() {
      fetch('/heap')
        .then(response => response.text())
        .then(data => {
          document.getElementById('heap').textContent = data;
        })
        .catch(() => {
          document.getElementById('heap').textContent = 'N/A';
        });
    }
    
    updateHeapInfo();
    setInterval(updateHeapInfo, 5000);

    document.getElementById('upload-form').addEventListener('submit', function(e) {
      const fileInput = document.getElementById('firmware-file');
      if (!fileInput.files.length) {
        e.preventDefault();
        alert('Please select a firmware file');
        return;
      }
      
      // Validate file extension
      const fileName = fileInput.files[0].name;
      if (!fileName.toLowerCase().endsWith('.bin')) {
        e.preventDefault();
        alert('Please select a valid .bin firmware file');
        return;
      }
      
      document.getElementById('upload-btn').disabled = true;
      document.getElementById('progress-container').style.display = 'block';
      document.querySelector('.ota-status').className = 'ota-status ota-active';
      document.querySelector('.ota-status').textContent = '⚡ OTA Update in Progress';
      
      const xhr = new XMLHttpRequest();
      const formData = new FormData(this);
      
      xhr.upload.addEventListener('progress', function(e) {
        if (e.lengthComputable) {
          const percentComplete = Math.round((e.loaded / e.total) * 100);
          const progressBar = document.getElementById('progress');
          const progressText = document.getElementById('progress-text');
          
          progressBar.style.width = percentComplete + '%';
          progressBar.textContent = percentComplete + '%';
          progressText.textContent = percentComplete + '% (' + 
            Math.round(e.loaded/1024) + ' KB / ' + Math.round(e.total/1024) + ' KB)';
        }
      });
      
      xhr.addEventListener('load', function() {
        if (xhr.status === 200) {
          document.getElementById('status-text').textContent = '✅ Update successful! Device is restarting...';
          document.getElementById('status-text').className = 'status success';
          document.querySelector('.ota-status').textContent = '✅ Update Complete - Restarting';
          
          // Redirect after successful update
          setTimeout(function() {
            window.location.href = '/';
          }, 10000);
        } else {
          document.getElementById('status-text').textContent = '❌ Update failed! Please try again.';
          document.getElementById('status-text').className = 'status error';
          document.getElementById('upload-btn').disabled = false;
          document.querySelector('.ota-status').className = 'ota-status ota-inactive';
          document.querySelector('.ota-status').textContent = '🚀 OTA Update Ready';
        }
      });
      
      xhr.addEventListener('error', function() {
        document.getElementById('status-text').textContent = '❌ Connection error! Please try again.';
        document.getElementById('status-text').className = 'status error';
        document.getElementById('upload-btn').disabled = false;
        document.querySelector('.ota-status').className = 'ota-status ota-inactive';
        document.querySelector('.ota-status').textContent = '🚀 OTA Update Ready';
      });
      
      xhr.open('POST', '/update', true);
      xhr.send(formData);
      e.preventDefault();
    });
  </script>
</body>
</html>
  )rawliteral";
  server.send(200, "text/html", html);
}

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== ESP32 WiFi Configuration System with OTA ===");
  
  // Wire.begin(21, 22);
  // if (!ads.begin()) {
  //   Serial.println("Failed to initialize ADS1115");
  // } else {
  //   Serial.println("ADS1115 initialized successfully");
  // }
  
  // ads.setGain(GAIN_TWOTHIRDS);  // ±6.144V range
  Serial.println("ADS1115 gain set to ±6.144V");
  
  readWiFiSettings();
  connectToWiFi();
  
  // Setup web server routes
// Setup web server routes
server.on("/", handleDataPage);       // ← Changed: Now default page shows readings
server.on("/login", handleRoot);     // ← Changed: WiFi config moved to /config
server.on("/save", HTTP_POST, handleSave);
server.on("/data", handleDataPage);   // ← Keep this for compatibility
  
  // OTA Update routes
  server.on("/update", HTTP_GET, handleOTAPage);
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
  
  // Heap info endpoint
  server.on("/heap", HTTP_GET, []() {
    server.send(200, "text/plain", String(ESP.getFreeHeap() / 1024));
  });
  
  server.begin();
  Serial.println("Web server started on port 80");
  
  // Setup WebSocket server
  webSocket.begin();
  webSocket.onEvent([](uint8_t num, WStype_t type, uint8_t * payload, size_t length) {
    if (type == WStype_CONNECTED) {
      Serial.println("WebSocket client connected: " + String(num));
    } else if (type == WStype_DISCONNECTED) {
      Serial.println("WebSocket client disconnected: " + String(num));
    }
  });
  Serial.println("WebSocket server started on port 81");
  
  Serial.println("=== Setup Complete ===");
  if (WiFi.status() == WL_CONNECTED) {
    Serial.println("Ready! Access the web interface at: http://" + WiFi.localIP().toString());
    Serial.println("Live data page: http://" + WiFi.localIP().toString() + "/data");
    Serial.println("OTA update page: http://" + WiFi.localIP().toString() + "/update");
  } else {
    Serial.println("Ready! Connect to SoftAP and access: http://192.168.4.1");
    Serial.println("Live data page: http://192.168.4.1/data");
    Serial.println("OTA update page: http://192.168.4.1/update");
  }
}

void loop() {
  // Check WiFi connection status and reconnect if needed
  if (!softAPMode && WiFi.status() != WL_CONNECTED) {
    Serial.println("WiFi connection lost. Attempting to reconnect...");
    connectToWiFi();
  }
  
  // // Read ADS1115 channels
  // for (int i = 0; i < 4; i++) {
  //   int16_t val = ads.readADC_SingleEnded(i);
  //   voltages[i] = val * 0.0001875;  // 187.5 µV per bit for GAIN_TWOTHIRDS (±6.144V)
  // }
  
  // Create JSON data for WebSocket
  String json = "[";
  for (int i = 0; i < 4; i++) {
    json += String(voltages[i], 3);
    if (i < 3) json += ",";
  }
  json += "]";
  
  // Send data to all connected WebSocket clients
  webSocket.broadcastTXT(json);
  
  // Handle web server and WebSocket
  server.handleClient();
  webSocket.loop();
  
  delay(100);  // Update every 100ms for better responsiveness
}