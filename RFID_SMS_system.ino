/*
 * ESP32 RFID SMS Security System - Complete Version with Web UI
 * 
 * CONNECTIONS:
 * EC200U -> ESP32: RX=16, TX=17, PWR=4, VCC=3.3V, GND=GND
 * RFID -> ESP32: SDA=5, RST=22, SCK=18, MISO=19, MOSI=23
 * Relays -> ESP32: Relay1=12, Relay2=13
 */

#include <HardwareSerial.h>
#include <SPI.h>
#include <MFRC522.h>
#include <WiFi.h>
#include <WebServer.h>
#include <ArduinoOTA.h>
#include <ArduinoJson.h>
#include <Preferences.h>
#include <Update.h>

// RFID Pins
#define SS_PIN    5     // SDA pin
#define RST_PIN   22    // Reset pin

// EC200U Pins
#define EC200U_RX_PIN 16
#define EC200U_TX_PIN 17
#define EC200U_POWER_PIN 4

// Relay Pins
#define RELAY1_PIN 12
#define RELAY2_PIN 13

// LED Pin
#define STATUS_LED 2

// ====================================================================
// 📱 CHANGE YOUR PHONE NUMBERS HERE - VERY IMPORTANT!
// ====================================================================
String rfid_num_1 = "+918185861199";        // RFID User 1 phone number (from your serial output)
String rfid_num_2 = "+918185861199";        // RFID User 2 phone number (change if different)
String bench_incharge = "+918185861199";    // Bench incharge phone number (change if different)
// ====================================================================

// RFID card UIDs
String rfid_card_1 = "73F6E8D9";
String rfid_card_2 = "73633BDA";

// SMS messages
String safety_message = "Are you wearing safety things?";
String permission_message = "Permission required";
String opened_message = "Opened";
String closed_message = "Closed";

// System state variables
bool card1_state = false;
bool card2_state = false;
bool relay1_state = false;
bool relay2_state = false;
bool waiting_safety_reply_1 = false;
bool waiting_safety_reply_2 = false;
bool waiting_permission_reply_1 = false;
bool waiting_permission_reply_2 = false;
int relay2_count = 0;

// Create instances
MFRC522 mfrc522(SS_PIN, RST_PIN);
HardwareSerial ec200u(2);
WebServer server(80);
Preferences preferences;

// WiFi credentials
String ssid = "RFID_SMS_System";
String password = "12345678";

// OTA variables
bool otaInProgress = false;
String otaStatus = "Ready";
int otaProgress = 0;

// Function declarations
void initializeModule();
void sendATCommand(String command, int timeout);
void sendSMS(String number, String message);
void readSMS(int index);
void parseSMSContent(String data);
void processSMSReply(String sender, String content);
bool isAuthorizedSender(String sender);
void handleRFID();
void handleSMS();
void handleSerialCommands();
void handleCard1();
void handleCard2();
void setupWiFiAP();
void setupWebServer();
void setupOTA();
void loadSettings();
void saveSettings();
void handleGetSettings();
void handleManualReset();  // Add this line
void handlePostSettings();
void handleGetStatus();
void handleResetCount();
void handleOTAStatus();
void handleFileUpload();
String getMainPage();

void setup() {
  Serial.begin(115200);
  Serial.println("========================================");
  Serial.println("ESP32 RFID SMS System Starting...");
  Serial.println("========================================");
  
  // Initialize pins
  pinMode(RELAY1_PIN, OUTPUT);
  pinMode(RELAY2_PIN, OUTPUT);
  pinMode(STATUS_LED, OUTPUT);
  pinMode(EC200U_POWER_PIN, OUTPUT);
  
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  digitalWrite(STATUS_LED, HIGH);
  digitalWrite(EC200U_POWER_PIN, HIGH);
  
  // Initialize preferences and load settings
  preferences.begin("rfid_sms", false);
  loadSettings();
  
  // Initialize SPI and RFID
  SPI.begin();
  mfrc522.PCD_Init();
  Serial.println("✓ RFID RC522 initialized");
  
  // Initialize EC200U
  ec200u.begin(115200, SERIAL_8N1, EC200U_RX_PIN, EC200U_TX_PIN);
  delay(500);
  initializeModule();
  
  // Setup WiFi AP
  setupWiFiAP();
  
  // Setup web server
  setupWebServer();
  
  // Setup OTA
  setupOTA();
  
  Serial.println("========================================");
  Serial.println("✅ System ready!");
  Serial.println("");
  Serial.println("📶 WiFi Access Point: " + ssid);
  Serial.println("🔑 Password: " + password);
  Serial.println("🌐 Web Interface: http://192.168.4.1");
  Serial.println("");
  Serial.println("📱 CONFIGURED PHONE NUMBERS:");
  Serial.println("   RFID User 1: " + rfid_num_1);
  Serial.println("   RFID User 2: " + rfid_num_2);
  Serial.println("   Bench Incharge: " + bench_incharge);
  Serial.println("");
  Serial.println("🏷️  CONFIGURED RFID CARDS:");
  Serial.println("   Card 1: " + rfid_card_1);
  Serial.println("   Card 2: " + rfid_card_2);
  Serial.println("");
  Serial.println("🚀 Ready for RFID card detection!");
  Serial.println("========================================");
}

void loop() {
  // Handle web server and OTA
  server.handleClient();
  ArduinoOTA.handle();
  
  // Core functionality (keep working as-is)
  if (!otaInProgress) {
    handleRFID();
    handleSMS();
   // handleSerialCommands();
  }
  
  // Status LED blinking
  static unsigned long lastBlink = 0;
  if (millis() - lastBlink > 1000) {
    digitalWrite(STATUS_LED, !digitalRead(STATUS_LED));
    lastBlink = millis();
  }
  
  // Send periodic AT commands to keep connection alive
  static unsigned long lastKeepAlive = 0;
  if (millis() - lastKeepAlive > 30000) {
    sendATCommand("AT", 300000);
    lastKeepAlive = millis();
  }
  
  delay(50);
}

void handleRFID() {
  // Check if a new card is present
  if (!mfrc522.PICC_IsNewCardPresent()) {
    return;
  }
  
  // Verify if the card has been read
  if (!mfrc522.PICC_ReadCardSerial()) {
    return;
  }
  
  String uid = "";
  for (byte i = 0; i < mfrc522.uid.size; i++) {
    uid += String(mfrc522.uid.uidByte[i] < 0x10 ? "0" : "");
    uid += String(mfrc522.uid.uidByte[i], HEX);
  }
  uid.toUpperCase();
  
  //Serial.println("🏷️ RFID Card Detected: " + uid);
  
  if (uid == rfid_card_1) {
    Serial.println("→Detected  Card 1...");
    handleCard1();
  } else if (uid == rfid_card_2) {
    Serial.println("→ Detected Card 2...");
    handleCard2();
  } else {
    Serial.println("❌ Unknown card: " + uid);
  }
  
  // Halt PICC and stop crypto
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  
}

void handleCard1() {

  if (!relay1_state && !relay2_state) {
    // Both relays OFF - start the process

    sendSMS(rfid_num_1, safety_message );
    waiting_safety_reply_1 = true;
    card1_state = true;
    
  } else if (relay1_state && relay2_state && card1_state == true  && card2_state == false) {
    // Both relays ON - close system
    card1_state = false; 
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    relay1_state = false;
    relay2_state = false;
    waiting_safety_reply_1 = false;
    waiting_permission_reply_1 = false;
    
    sendSMS(bench_incharge, closed_message + " RFID User 1 ");

  } else {
    // Any other state (Relay1=ON, Relay2=OFF) - reset and start fresh
   

  }
}

void handleCard2() {

  if (!relay1_state && !relay2_state) {
    // Both relays OFF - start the process

    sendSMS(rfid_num_2, safety_message );
    waiting_safety_reply_2 = true;
    card2_state = true;
    
  } else if (relay1_state && relay2_state && card2_state == true && card1_state == false) {
    // Both relays ON - close system
    card2_state = false;
    digitalWrite(RELAY1_PIN, LOW);
    digitalWrite(RELAY2_PIN, LOW);
    relay1_state = false;
    relay2_state = false;
    waiting_safety_reply_2 = false;
    waiting_permission_reply_2 = false;
    
    sendSMS(bench_incharge, closed_message + " RFID User 2 ");
    
    
  } else {

  }
}

void handleSMS() {
  if (ec200u.available()) {
    String data = ec200u.readString();
    data.trim();
    
    // Print raw data for debugging
    Serial.print("Raw data from EC200U: ");
    Serial.println(data);
    
    // Check for DIRECT SMS notification (+CMT: format) - This is what your module sends!
    if (data.indexOf("+CMT:") != -1) {
     // Serial.println(" Direct SMS received!");
      parseSMSContent(data);
    }
    
    // Also check for standard SMS notification (+CMTI: format)
    else if (data.indexOf("+CMTI:") != -1) {
     // Serial.println(" SMS notification received!");
      int indexStart = data.indexOf(",") + 1;
      int smsIndex = data.substring(indexStart).toInt();
      Serial.println("SMS Index: " + String(smsIndex));
      readSMS(smsIndex);
    }
    
    // Check for SMS content response from AT+CMGR
    else if (data.indexOf("+CMGR:") != -1) {
      //Serial.println("SMS content response received!");
      parseSMSContent(data);
    }
  }
}

void parseSMSContent(String data) {
  Serial.println("\n=== SMS PARSING ===");
  
  String sender = "";
  String content = "";
  String timestamp = "";
  
  // Method 1: Parse +CMT: format (Direct SMS - what your module sends!)
  if (data.indexOf("+CMT:") != -1) {
  
    // Extract sender from first quotes: +CMT: "+918185861199",,"25/08/24,12:12:31+22"
    int firstQuote = data.indexOf('"');
    int secondQuote = data.indexOf('"', firstQuote + 1);
    
    if (firstQuote != -1 && secondQuote != -1) {
      sender = data.substring(firstQuote + 1, secondQuote);
    }
    
    // Extract timestamp from last quotes
    int lastQuoteStart = data.lastIndexOf('"', data.length() - 2);
    int lastQuoteEnd = data.lastIndexOf('"');
    if (lastQuoteStart != -1 && lastQuoteEnd != -1) {
      timestamp = data.substring(lastQuoteStart + 1, lastQuoteEnd);
    }
    
    // Extract message content (next line after +CMT:)
    int contentStart = data.indexOf('\n');
    if (contentStart != -1) {
      content = data.substring(contentStart + 1);
      content.replace("OK", "");
      content.replace("\r", "");
      content.replace("\n", "");
      content.trim();
    }
  }
  
  // Method 2: Parse +CMGR: format (SMS read response)
  else if (data.indexOf("+CMGR:") != -1) {
   
    // Use original parsing method for +CMGR
    int firstNewline = data.indexOf('\n');
    int secondNewline = data.indexOf('\n', firstNewline + 1);
    
    if (firstNewline != -1 && secondNewline != -1) {
      String header = data.substring(0, secondNewline);
      content = data.substring(secondNewline + 1);
      
      // Parse sender from header
      int quoteStart = header.indexOf('"', header.indexOf('"') + 1) + 1;
      int quoteEnd = header.indexOf('"', quoteStart);
      if (quoteStart > 0 && quoteEnd > quoteStart) {
        sender = header.substring(quoteStart, quoteEnd);
      }
      
      // Clean content
      content.replace("OK", "");
      content.trim();
    }
  }
  
  // Display the parsed SMS
  if (sender.length() > 0 && content.length() > 0) {
    Serial.println("From: " + sender);
    Serial.println("Time: " + timestamp);
    Serial.println("Message: " + content);
    Serial.println("==================");
    
    // Security check - only process SMS from configured numbers
    if (isAuthorizedSender(sender)) {
      Serial.println("✅ Authorized sender confirmed");
      
      // Case-insensitive processing
      String lowerContent = content;
      lowerContent.toLowerCase();
      lowerContent.trim();
      
      if (lowerContent == "yes") {
        Serial.println("✅ Valid 'YES' reply received");
        processSMSReply(sender, lowerContent);
      } else {
        Serial.println("ℹ️ SMS ignored - not a 'yes' reply: '" + content + "'");
      }
    } else {
      Serial.println("❌ SMS ignored - unauthorized sender: " + sender);
      Serial.println("   Authorized: " + rfid_num_1 + ", " + rfid_num_2 + ", " + bench_incharge);
    }
  } else {
    Serial.println("❌ Could not parse SMS data:");
    Serial.println(data);
  }
}

// Check if sender is authorized
bool isAuthorizedSender(String sender) {
  sender.trim();
  return (sender == rfid_num_1 || sender == rfid_num_2 || sender == bench_incharge);
}

void processSMSReply(String sender, String content) {
   
  if (content == "yes") {
    // RFID User 1 safety confirmation
    if (sender == rfid_num_1 && waiting_safety_reply_1) {
      //Serial.println(" RFID User 1 safety confirmed - Turning ON Relay 1");
      digitalWrite(RELAY1_PIN, HIGH);
      relay1_state = true;
      waiting_safety_reply_1 = false;
      
      Serial.println(" Sending permission request to bench incharge: " + bench_incharge);
      sendSMS(bench_incharge, permission_message  + " RFID User 1 ");
      waiting_permission_reply_1 = true;
      //Serial.println(" Waiting for bench incharge permission...");
      
    // RFID User 2 safety confirmation  
    } else if (sender == rfid_num_2 && waiting_safety_reply_2) {
     // Serial.println(" RFID User 2 safety confirmed - Turning ON Relay 1");
      digitalWrite(RELAY1_PIN, HIGH);
      relay1_state = true;
      waiting_safety_reply_2 = false;
      
      Serial.println(" Sending permission request to bench incharge: " + bench_incharge);
      sendSMS(bench_incharge, permission_message  + " RFID User 2 ");
      waiting_permission_reply_2 = true;
      //Serial.println("Waiting for bench incharge permission...");
      
    // Bench incharge permission
    } else if (sender == bench_incharge && (waiting_permission_reply_1 || waiting_permission_reply_2)) {
      Serial.println(" Bench incharge permission granted - Turning ON Relay 2");
      digitalWrite(RELAY2_PIN, HIGH);
      relay2_state = true;
      relay2_count++;
      preferences.putInt("relay2_count", relay2_count); // Save count to flash
      waiting_permission_reply_1 = false;
      waiting_permission_reply_2 = false;
      
      Serial.println("🎉 SYSTEM FULLY OPENED! Count: " + String(relay2_count));
      sendSMS(bench_incharge, opened_message + " (Count: " + String(relay2_count) + ")");
      
    } else {
      Serial.println("❌ 'YES' reply ignored - wrong sender or state");
    }
    
  } else {
    // Handle "no" or any other message - reset the process

  }
}

void initializeModule() {
  Serial.println("Initializing EC200U module...");
  
  // Power cycle the module
  digitalWrite(EC200U_POWER_PIN, LOW);
  delay(2000);
  digitalWrite(EC200U_POWER_PIN, HIGH);
  delay(5000);
  
  // Clear any existing data
  while (ec200u.available()) {
    ec200u.read();
  }
  
  // Wait for module ready signals (from your working reference)
  unsigned long timeout = millis() + 30000;
  bool moduleReady = false;
  String response = "";
  
  Serial.println("⏳ Waiting for module ready...");
  while (millis() < timeout && !moduleReady) {
    if (ec200u.available()) {
      char c = ec200u.read();
      response += c;
      if (response.indexOf("RDY") != -1 || response.indexOf("+CFUN: 1") != -1) {
        moduleReady = true;
        Serial.println("Module ready!");
        delay(1000);
        break;
      }
    }
    delay(100);
  }
  
  // Clear buffer
  while (ec200u.available()) {
    ec200u.read();
  }
  
  // Use EXACT same initialization as your working reference code
  Serial.println("📡 Configuring  Module...");
  sendATCommand("AT", 2000);
  sendATCommand("ATE0", 2000);                    // Disable echo
  sendATCommand("AT+CPIN?", 3000);                // Check SIM status
  sendATCommand("AT+CREG?", 3000);                // Check network registration
  sendATCommand("AT+COPS?", 5000);                // Check operator
  sendATCommand("AT+CMGF=1", 2000);               // Set SMS text mode
  sendATCommand("AT+CNMI=1,2,0,0,0", 2000);       // Enable SMS notifications (your working setting)
  sendATCommand("AT+CPMS=\"SM\",\"SM\",\"SM\"", 2000); // SMS storage (ignore ERROR if not supported)
  
  Serial.println(" EC200U initialized");
}

void sendATCommand(String command, int timeout) {
  // Same as your working reference code
 // Serial.println("Sending: " + command);
  ec200u.println(command);
  
  long int time = millis();
  while ((time + timeout) > millis()) {
    if (ec200u.available()) {
      String response = ec200u.readString();
      response.trim();
     // Serial.println("Response: " + response);
      break;
    }
  }
}

void readSMS(int index) {
  String command = "AT+CMGR=" + String(index);
  ec200u.println(command);
}

void sendSMS(String number, String message) {
  if (number.length() < 10) {
    Serial.println("❌ ERROR: Phone number too short - " + number);
    return;
  }
  
  // Same method as your working reference code
  ec200u.println("AT+CMGS=\"" + number + "\"");
  delay(1000);
  ec200u.println(message);
  delay(1000);
  ec200u.write(0x1A); // Send Ctrl+Z to send SMS
  
}

// ============================================================================
// WEB SERVER AND FLASH STORAGE FUNCTIONS
// ============================================================================

void setupWiFiAP() {
  WiFi.mode(WIFI_AP);
  WiFi.softAP(ssid.c_str(), password.c_str());
  
  IPAddress IP = WiFi.softAPIP();
  Serial.print("AP IP address: ");
  Serial.println(IP);
}

void handleManualReset() {
  // Reset all system states
  digitalWrite(RELAY1_PIN, LOW);
  digitalWrite(RELAY2_PIN, LOW);
  relay1_state = false;
  relay2_state = false;
  waiting_safety_reply_1 = false;
  waiting_safety_reply_2 = false;
  waiting_permission_reply_1 = false;
  waiting_permission_reply_2 = false;
  card1_state = false;  // Reset card states too
  card2_state = false;
  
  Serial.println(" Manual system reset performed via web interface");
  
  server.send(200, "application/json", "{\"success\":true}");
}

void setupWebServer() {
  server.on("/", HTTP_GET, []() {
    server.send(200, "text/html", getMainPage());
  });
  
  server.on("/api/settings", HTTP_GET, handleGetSettings);
  server.on("/api/settings", HTTP_POST, handlePostSettings);
  server.on("/api/status", HTTP_GET, handleGetStatus);
  server.on("/api/reset-count", HTTP_POST, handleResetCount);
  server.on("/api/manual-reset", HTTP_POST, handleManualReset);
  server.on("/api/ota-status", HTTP_GET, handleOTAStatus);
  server.on("/api/upload", HTTP_POST, 
    []() { 
      server.send(200, "text/plain", otaInProgress ? "Upload completed" : "Upload failed"); 
    },
    handleFileUpload
  );
  
  server.begin();
  Serial.println("Web server started");
}

void setupOTA() {
  ArduinoOTA.setHostname("RFID-SMS-System");
  
  ArduinoOTA.onStart([]() {
    otaInProgress = true;
    otaStatus = "Starting...";
    otaProgress = 0;
    Serial.println("OTA Start");
  });
  
  ArduinoOTA.onEnd([]() {
    otaInProgress = false;
    otaStatus = "Completed";
    otaProgress = 100;
    Serial.println("OTA End");
  });
  
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    otaProgress = (progress / (total / 100));
    otaStatus = "Progress: " + String(otaProgress) + "%";
  });
  
  ArduinoOTA.onError([](ota_error_t error) {
    otaInProgress = false;
    if (error == OTA_AUTH_ERROR) otaStatus = "Auth Failed";
    else if (error == OTA_BEGIN_ERROR) otaStatus = "Begin Failed";
    else if (error == OTA_CONNECT_ERROR) otaStatus = "Connect Failed";
    else if (error == OTA_RECEIVE_ERROR) otaStatus = "Receive Failed";
    else if (error == OTA_END_ERROR) otaStatus = "End Failed";
  });
  
  ArduinoOTA.begin();
}

void loadSettings() {
  rfid_num_1 = preferences.getString("rfid_num_1", "+918185861199");
  rfid_num_2 = preferences.getString("rfid_num_2", "+918185861199");
  bench_incharge = preferences.getString("bench_incharge", "+918185861199");
  rfid_card_1 = preferences.getString("rfid_card_1", "73F6E8D9");
  rfid_card_2 = preferences.getString("rfid_card_2", "73633BDA");
  safety_message = preferences.getString("safety_msg", "Are you wearing safety things?");
  permission_message = preferences.getString("permission_msg", "Permission required");
  opened_message = preferences.getString("opened_msg", "Opened");
  closed_message = preferences.getString("closed_msg", "Closed");
  relay2_count = preferences.getInt("relay2_count", 0);
  
  Serial.println("Settings loaded from flash");
}

void saveSettings() {
  preferences.putString("rfid_num_1", rfid_num_1);
  preferences.putString("rfid_num_2", rfid_num_2);
  preferences.putString("bench_incharge", bench_incharge);
  preferences.putString("rfid_card_1", rfid_card_1);
  preferences.putString("rfid_card_2", rfid_card_2);
  preferences.putString("safety_msg", safety_message);
  preferences.putString("permission_msg", permission_message);
  preferences.putString("opened_msg", opened_message);
  preferences.putString("closed_msg", closed_message);
  preferences.putInt("relay2_count", relay2_count);
  
  Serial.println("Settings saved to flash memory");
}

void handleGetSettings() {
  DynamicJsonDocument doc(1024);
  doc["rfid_num_1"] = rfid_num_1;
  doc["rfid_num_2"] = rfid_num_2;
  doc["bench_incharge"] = bench_incharge;
  doc["rfid_card_1"] = rfid_card_1;
  doc["rfid_card_2"] = rfid_card_2;
  doc["safety_message"] = safety_message;
  doc["permission_message"] = permission_message;
  doc["opened_message"] = opened_message;
  doc["closed_message"] = closed_message;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handlePostSettings() {
  if (server.hasArg("plain")) {
    DynamicJsonDocument doc(1024);
    deserializeJson(doc, server.arg("plain"));
    
    if (doc.containsKey("rfid_num_1")) rfid_num_1 = doc["rfid_num_1"].as<String>();
    if (doc.containsKey("rfid_num_2")) rfid_num_2 = doc["rfid_num_2"].as<String>();
    if (doc.containsKey("bench_incharge")) bench_incharge = doc["bench_incharge"].as<String>();
    if (doc.containsKey("rfid_card_1")) rfid_card_1 = doc["rfid_card_1"].as<String>();
    if (doc.containsKey("rfid_card_2")) rfid_card_2 = doc["rfid_card_2"].as<String>();
    if (doc.containsKey("safety_message")) safety_message = doc["safety_message"].as<String>();
    if (doc.containsKey("permission_message")) permission_message = doc["permission_message"].as<String>();
    if (doc.containsKey("opened_message")) opened_message = doc["opened_message"].as<String>();
    if (doc.containsKey("closed_message")) closed_message = doc["closed_message"].as<String>();
    
    saveSettings();
    server.send(200, "application/json", "{\"success\":true}");
  } else {
    server.send(400, "application/json", "{\"error\":\"No data\"}");
  }
}

void handleGetStatus() {
  DynamicJsonDocument doc(512);
  doc["relay1_state"] = relay1_state;
  doc["relay2_state"] = relay2_state;
  doc["relay2_count"] = relay2_count;
  doc["waiting_safety_1"] = waiting_safety_reply_1;
  doc["waiting_safety_2"] = waiting_safety_reply_2;
  doc["waiting_permission_1"] = waiting_permission_reply_1;
  doc["waiting_permission_2"] = waiting_permission_reply_2;
  doc["wifi_clients"] = WiFi.softAPgetStationNum();
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleResetCount() {
  relay2_count = 0;
  preferences.putInt("relay2_count", relay2_count);
  server.send(200, "application/json", "{\"success\":true}");
}

void handleOTAStatus() {
  DynamicJsonDocument doc(256);
  doc["status"] = otaStatus;
  doc["progress"] = otaProgress;
  doc["in_progress"] = otaInProgress;
  
  String response;
  serializeJson(doc, response);
  server.send(200, "application/json", response);
}

void handleFileUpload() {
  HTTPUpload& upload = server.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("OTA Update Start: %s\n", upload.filename.c_str());
    otaInProgress = true;
    otaStatus = "Starting upload...";
    otaProgress = 0;
    
    // Stop other processes during OTA
    digitalWrite(STATUS_LED, LOW); // Turn off blinking LED
    
    if (!Update.begin()) {
      Serial.println("OTA Update Begin Failed");
      otaStatus = "Update failed to begin - insufficient space";
      otaInProgress = false;
      return;
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Serial.println("OTA Update Write Failed");
      otaStatus = "Write failed";
      otaInProgress = false;
    } else {
      otaProgress = (Update.progress() * 100) / Update.size();
      otaStatus = "Uploading: " + String(otaProgress) + "%";
      Serial.printf("OTA Progress: %d%%\n", otaProgress);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("OTA Update Success: %u bytes\n", upload.totalSize);
      otaStatus = "Update Success! Rebooting in 3 seconds...";
      otaProgress = 100;
      
      // Send final status before reboot
      server.send(200, "text/plain", "Upload completed successfully");
      
      delay(3000);
      ESP.restart();
    } else {
      Serial.printf("OTA Update Failed: %s\n", Update.errorString());
      otaStatus = "Update failed: " + String(Update.errorString());
      otaInProgress = false;
    }
  } 
  else if (upload.status == UPLOAD_FILE_ABORTED) {
    Serial.println("OTA Update Aborted");
    otaStatus = "Upload aborted";
    otaInProgress = false;
    Update.abort();
  }
}


String getMainPage() {
  return R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>RFID SMS Security System</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        * {
            margin: 0;
            padding: 0;
            box-sizing: border-box;
        }
        
        body { 
            font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif; 
            background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
            color: #333;
            min-height: 100vh;
            padding: 20px;
        }
        
        .container { 
            max-width: 1200px; 
            margin: 0 auto; 
            background: rgba(255,255,255,0.95); 
            padding: 40px; 
            border-radius: 20px; 
            box-shadow: 0 20px 60px rgba(0,0,0,0.3);
            backdrop-filter: blur(10px);
        }
        
        .header { 
            text-align: center; 
            color: #2c3e50; 
            margin-bottom: 40px; 
            font-size: 2.5em;
            font-weight: 700;
        }
        
        .header::after {
            content: '';
            display: block;
            width: 100px;
            height: 4px;
            background: linear-gradient(45deg, #667eea, #764ba2);
            margin: 20px auto;
            border-radius: 2px;
        }
        
        .section { 
            margin-bottom: 30px; 
            padding: 25px; 
            border-radius: 15px; 
            background: linear-gradient(145deg, #ffffff 0%, #f8f9fa 100%);
            box-shadow: 0 8px 25px rgba(0,0,0,0.08);
        }
        
        .section h3 {
            margin-bottom: 20px;
            color: #2c3e50;
            font-size: 1.4em;
            font-weight: 600;
        }
        
        .status-grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(250px, 1fr));
            gap: 20px;
            margin-bottom: 20px;
        }
        
        .status-card { 
            display: flex; 
            justify-content: space-between; 
            align-items: center; 
            padding: 20px;
            background: linear-gradient(145deg, #fff 0%, #f1f3f4 100%);
            border-radius: 12px;
            border-left: 4px solid #667eea;
            box-shadow: 0 4px 12px rgba(0,0,0,0.05);
        }
        
        .status-indicator { 
            width: 24px; 
            height: 24px; 
            border-radius: 50%; 
            margin-right: 12px; 
            border: 3px solid #ddd;
            box-shadow: 0 2px 8px rgba(0,0,0,0.2);
        }
        
        .on { 
            background: linear-gradient(45deg, #28a745, #20c997); 
            border-color: #28a745; 
            box-shadow: 0 0 15px rgba(40, 167, 69, 0.4);
        }
        .off { 
            background: linear-gradient(45deg, #dc3545, #c82333); 
            border-color: #dc3545; 
        }
        .waiting { 
            background: linear-gradient(45deg, #ffc107, #ff8f00); 
            border-color: #ffc107; 
            animation: pulse 2s infinite;
        }
        
        @keyframes pulse {
            0% { box-shadow: 0 0 15px rgba(255, 193, 7, 0.4); }
            50% { box-shadow: 0 0 25px rgba(255, 193, 7, 0.8); }
            100% { box-shadow: 0 0 15px rgba(255, 193, 7, 0.4); }
        }
        
        .grid {
            display: grid;
            grid-template-columns: repeat(auto-fit, minmax(400px, 1fr));
            gap: 25px;
        }
        
        .input-group {
            margin-bottom: 20px;
        }
        
        .input-group label {
            display: block;
            margin-bottom: 8px;
            font-weight: 600;
            color: #2c3e50;
        }
        
        input, textarea { 
            width: 100%; 
            padding: 15px; 
            border: 2px solid #e1e5e9; 
            border-radius: 10px; 
            font-size: 14px;
            transition: all 0.3s ease;
            background: #fff;
        }
        
        input:focus, textarea:focus {
            border-color: #667eea;
            outline: none;
            box-shadow: 0 0 0 3px rgba(102, 126, 234, 0.1);
        }
        
        button { 
            background: linear-gradient(45deg, #667eea, #764ba2); 
            color: white; 
            padding: 15px 30px; 
            border: none; 
            border-radius: 10px; 
            cursor: pointer; 
            margin: 8px; 
            font-size: 14px;
            font-weight: 600;
            transition: all 0.3s ease;
            box-shadow: 0 6px 20px rgba(102, 126, 234, 0.3);
        }
        
        button:hover { 
            transform: translateY(-3px);
            box-shadow: 0 10px 30px rgba(102, 126, 234, 0.4);
        }
        
        .btn-success {
            background: linear-gradient(45deg, #28a745, #20c997);
            box-shadow: 0 6px 20px rgba(40, 167, 69, 0.3);
        }
        
        .btn-danger { 
            background: linear-gradient(45deg, #dc3545, #c82333); 
            box-shadow: 0 6px 20px rgba(220, 53, 69, 0.3);
        }
        
        .btn-info {
            background: linear-gradient(45deg, #17a2b8, #138496);
            box-shadow: 0 6px 20px rgba(23, 162, 184, 0.3);
        }
        
        .upload-area { 
            border: 3px dashed #667eea; 
            padding: 40px; 
            text-align: center; 
            margin: 20px 0; 
            border-radius: 15px;
            transition: all 0.3s ease;
            background: linear-gradient(145deg, #f8f9ff 0%, #e6eaff 100%);
        }
        
        .upload-area:hover {
            border-color: #5a67d8;
            background: linear-gradient(145deg, #f0f4ff 0%, #dde4ff 100%);
        }
        
        .progress { 
            width: 100%; 
            background: #e9ecef; 
            border-radius: 12px; 
            overflow: hidden; 
            margin: 20px 0; 
            height: 30px;
        }
        
        .progress-bar { 
            height: 100%; 
            background: linear-gradient(45deg, #28a745, #20c997); 
            transition: width 0.5s ease; 
            display: flex;
            align-items: center;
            justify-content: center;
            color: white;
            font-weight: 700;
        }
        
        .count-display {
            background: linear-gradient(45deg, #667eea, #764ba2);
            color: white;
            padding: 10px 20px;
            border-radius: 25px;
            font-weight: 700;
            font-size: 1.2em;
        }
        
        .notification {
            position: fixed;
            top: 20px;
            right: 20px;
            padding: 15px 25px;
            border-radius: 10px;
            color: white;
            font-weight: 600;
            z-index: 1000;
            transform: translateX(400px);
            transition: transform 0.3s ease;
        }
        
        .notification.show {
            transform: translateX(0);
        }
        
        .notification.success {
            background: linear-gradient(45deg, #28a745, #20c997);
        }
        
        .notification.error {
            background: linear-gradient(45deg, #dc3545, #c82333);
        }
        
        @media (max-width: 768px) {
            .grid {
                grid-template-columns: 1fr;
            }
            .container {
                padding: 25px;
                margin: 10px;
            }
        }
    </style>
</head>
<body>
    <div id="notification" class="notification"></div>
    
    <div class="container">
        <h1 class="header">RFID SMS Security System</h1>
        
        <div class="section">
            <h3>System Status Dashboard</h3>
            <div class="status-grid">
                <div class="status-card">
                    <span><strong>Relay 1:</strong></span>
                    <div style="display: flex; align-items: center;">
                        <div class="status-indicator" id="relay1"></div>
                        <span id="relay1-text">OFF</span>
                    </div>
                </div>
                <div class="status-card">
                    <span><strong>Relay 2:</strong></span>
                    <div style="display: flex; align-items: center;">
                        <div class="status-indicator" id="relay2"></div>
                        <span id="relay2-text">OFF</span>
                    </div>
                </div>
                <div class="status-card">
                    <span><strong>Usage Count:</strong></span>
                    <div style="display: flex; align-items: center; gap: 15px;">
                        <span class="count-display" id="count">0</span>
                        <button onclick="resetCount()" class="btn-danger" style="padding: 8px 15px; font-size: 12px;">Reset</button>
                    </div>
                </div>

             
            </div>
        </div>
        
        <div class="grid">
            <div class="section">
                <h3>Phone Number Configuration</h3>
                <div class="input-group">
                    <label>RFID User 1 Number:</label>
                    <input type="tel" id="rfid_num_1" placeholder="+918185861199">
                </div>
                
                <div class="input-group">
                    <label>RFID User 2 Number:</label>
                    <input type="tel" id="rfid_num_2" placeholder="+918185861199">
                </div>
                
                <div class="input-group">
                    <label>Bench Incharge Number:</label>
                    <input type="tel" id="bench_incharge" placeholder="+918185861199">
                </div>
            </div>
            
            <div class="section">
                <h3>RFID Card Configuration</h3>
                <div class="input-group">
                    <label>Card 1 UID:</label>
                    <input type="text" id="rfid_card_1" placeholder="73F6E8D9" style="text-transform: uppercase;">
                </div>
                
                <div class="input-group">
                    <label>Card 2 UID:</label>
                    <input type="text" id="rfid_card_2" placeholder="73633BDA" style="text-transform: uppercase;">
                </div>
                
                <div style="margin-top: 20px; padding: 15px; background: #e8f5e8; border-radius: 10px; border-left: 4px solid #28a745;">
                    <strong>Note:</strong> Tap RFID cards on reader to get UIDs from serial monitor
                </div>
            </div>
        </div>
        
        <div class="section">
            <h3>SMS Message Templates</h3>
            <div class="grid">
                <div>
                    <div class="input-group">
                        <label>Safety Confirmation Message:</label>
                        <textarea id="safety_message" rows="2">Are you wearing safety things?</textarea>
                    </div>
                    
                    <div class="input-group">
                        <label>Permission Request Message:</label>
                        <textarea id="permission_message" rows="2">Permission required</textarea>
                    </div>
                </div>
                <div>
                    <div class="input-group">
                        <label>System Opened Message:</label>
                        <textarea id="opened_message" rows="2">Opened</textarea>
                    </div>
                    
                    <div class="input-group">
                        <label>System Closed Message:</label>
                        <textarea id="closed_message" rows="2">Closed</textarea>
                    </div>
                </div>
            </div>
        </div>
        
        <div class="section" style="text-align: center;">
            <button onclick="saveSettings()" class="btn-success">Save All Settings</button>
            <button onclick="loadSettings()" class="btn-info">Reload Settings</button>
            <button onclick="manualReset()" class="btn-danger">Reset System</button>
        </div>
        
        <div class="section">
            <h3>Firmware Update (OTA)</h3>
            <div class="upload-area">
                <input type="file" id="firmware" accept=".bin" style="margin: 15px;">
                <br><br>
                <button onclick="uploadFirmware()" class="btn-success">Upload Firmware</button>
            </div>
            <div class="progress">
                <div class="progress-bar" id="progress" style="width: 0%">0%</div>
            </div>
            <div id="ota-status" style="text-align: center; margin-top: 15px; font-weight: 600;">Ready for upload</div>
        </div>
        
        <div class="section">
            <h3>System Information</h3>
            <div style="background: #f8f9fa; padding: 20px; border-radius: 10px;">
                <div style="margin-bottom: 15px;"><strong>WiFi Network:</strong> RFID_SMS_System</div>
                <div style="margin-bottom: 15px;"><strong>Password:</strong> 12345678</div>
                <div style="margin-bottom: 15px;"><strong>IP Address:</strong> 192.168.4.1</div>
               
            </div>
        </div>
    </div>

    <script>
        function showNotification(message, type = 'success') {
            const notification = document.getElementById('notification');
            notification.textContent = message;
            notification.className = `notification ${type}`;
            notification.classList.add('show');
            
            setTimeout(() => {
                notification.classList.remove('show');
            }, 3000);
        }

        function loadSettings() {
            fetch('/api/settings')
                .then(response => response.json())
                .then(data => {
                    Object.keys(data).forEach(key => {
                        const element = document.getElementById(key);
                        if (element) {
                            element.value = data[key];
                        }
                    });
                    showNotification('Settings loaded successfully', 'success');
                })
                .catch(error => {
                    showNotification('Error loading settings', 'error');
                });
        }

        function saveSettings() {
            const settings = {};
            ['rfid_num_1', 'rfid_num_2', 'bench_incharge', 'rfid_card_1', 'rfid_card_2',
             'safety_message', 'permission_message', 'opened_message', 'closed_message'].forEach(id => {
                const element = document.getElementById(id);
                if (element) {
                    settings[id] = element.value.trim();
                }
            });

            fetch('/api/settings', {
                method: 'POST',
                headers: {'Content-Type': 'application/json'},
                body: JSON.stringify(settings)
            })
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showNotification('Settings saved to flash memory', 'success');
                } else {
                    showNotification('Error saving settings', 'error');
                }
            })
            .catch(error => {
                showNotification('Error saving settings', 'error');
            });
        }

        function updateStatus() {
            fetch('/api/status')
                .then(response => response.json())
                .then(data => {
                    // Update relay indicators
                    document.getElementById('relay1').className = 'status-indicator ' + (data.relay1_state ? 'on' : 'off');
                    document.getElementById('relay1-text').textContent = data.relay1_state ? 'ON' : 'OFF';
                    
                    document.getElementById('relay2').className = 'status-indicator ' + (data.relay2_state ? 'on' : 'off');
                    document.getElementById('relay2-text').textContent = data.relay2_state ? 'ON' : 'OFF';
                    
                    // Update count and clients
                    document.getElementById('count').textContent = data.relay2_count;
                    document.getElementById('clients').textContent = data.wifi_clients;
                    
                    // Update system state
                    let systemState = 'Ready';
                    let stateColor = '#28a745';
                    
                    if (data.waiting_safety_1 || data.waiting_safety_2) {
                        systemState = 'Waiting Safety Reply';
                        stateColor = '#ffc107';
                    } else if (data.waiting_permission_1 || data.waiting_permission_2) {
                        systemState = 'Waiting Permission';
                        stateColor = '#ffc107';
                    } else if (data.relay1_state && data.relay2_state) {
                        systemState = 'System Active';
                        stateColor = '#28a745';
                    } else if (data.relay1_state) {
                        systemState = 'Safety Confirmed';
                        stateColor = '#17a2b8';
                    }
                    
                    const systemStateElement = document.getElementById('system-state');
                    systemStateElement.textContent = systemState;
                    systemStateElement.style.color = stateColor;
                })
                .catch(error => {
                    console.error('Error updating status:', error);
                });
        }

        function resetCount() {
            if (confirm('Reset usage count to 0?')) {
                fetch('/api/reset-count', {method: 'POST'})
                    .then(response => response.json())
                    .then(data => {
                        if (data.success) {
                            showNotification('Count reset successfully', 'success');
                            updateStatus();
                        } else {
                            showNotification('Error resetting count', 'error');
                        }
                    })
                    .catch(error => {
                        showNotification('Error resetting count', 'error');
                    });
            }
        }

function manualReset() {
    if (confirm('Reset entire system state? This will turn off all relays and clear waiting states.')) {
        fetch('/api/manual-reset', {method: 'POST'})
            .then(response => response.json())
            .then(data => {
                if (data.success) {
                    showNotification('System reset successfully', 'success');
                    updateStatus(); // Refresh the status display
                } else {
                    showNotification('Error resetting system', 'error');
                }
            })
            .catch(error => {
                showNotification('Error resetting system', 'error');
            });
    }
}

        function uploadFirmware() {
            const fileInput = document.getElementById('firmware');
            const file = fileInput.files[0];
            
            if (!file) {
                showNotification('Please select a firmware file', 'error');
                return;
            }

            if (!file.name.endsWith('.bin')) {
                showNotification('Please select a valid .bin file', 'error');
                return;
            }

            const formData = new FormData();
            formData.append('firmware', file);

            const xhr = new XMLHttpRequest();
            
            xhr.upload.addEventListener('progress', function(e) {
                if (e.lengthComputable) {
                    const percentComplete = Math.round((e.loaded / e.total) * 100);
                    const progressBar = document.getElementById('progress');
                    progressBar.style.width = percentComplete + '%';
                    progressBar.textContent = percentComplete + '%';
                }
            });

            xhr.onload = function() {
                if (xhr.status === 200) {
                    document.getElementById('ota-status').textContent = 'Upload completed! Device rebooting...';
                    showNotification('Firmware update completed', 'success');
                } else {
                    document.getElementById('ota-status').textContent = 'Upload failed';
                    showNotification('Upload failed', 'error');
                }
            };

            xhr.onerror = function() {
                document.getElementById('ota-status').textContent = 'Upload error';
                showNotification('Upload error', 'error');
            };

            document.getElementById('ota-status').textContent = 'Uploading firmware...';
            showNotification('Starting firmware upload...', 'success');
            xhr.open('POST', '/api/upload');
            xhr.send(formData);
        }

        function checkOTAStatus() {
            fetch('/api/ota-status')
                .then(response => response.json())
                .then(data => {
                    if (data.in_progress) {
                        const progressBar = document.getElementById('progress');
                        progressBar.style.width = data.progress + '%';
                        progressBar.textContent = data.progress + '%';
                        document.getElementById('ota-status').textContent = data.status;
                    }
                })
                .catch(error => {
                    console.error('Error checking OTA status:', error);
                });
        }

        // Initialize page
        document.addEventListener('DOMContentLoaded', function() {
            loadSettings();
            updateStatus();
            
            // Auto-refresh status every 3 seconds
            setInterval(updateStatus, 3000);
            
            // Check OTA status during upload
            setInterval(checkOTAStatus, 1000);
        });
    </script>
</body>
</html>
)rawliteral";
}