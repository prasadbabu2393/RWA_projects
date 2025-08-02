/***************************************************************************
 *  ESP32  ▸  RS-485 Camera System ▸  ANTI-SWAP Protection
 *  Board: ESP32 DevKit / WROOM-32
 *  GPIO33: Start cycle → Send Modbus frame → Get REAL-TIME result
 *  GPIO34: End cycle → Control relay based on CURRENT cycle result
 *  COMPREHENSIVE FIX: Prevents 1st→2nd, 2nd→3rd result swapping
 ***************************************************************************/

//------------- RS-485 pins ---------------------------
constexpr uint8_t DE_RE_PIN = 21;   // DE + RE control (HIGH=Tx, LOW=Rx)
constexpr uint8_t TXD2_PIN  = 17;   // UART2 TX
constexpr uint8_t RXD2_PIN  = 16;   // UART2 RX

//------------- Control / status pins ----------------
constexpr uint8_t signalPin = 33;   // Cycle start trigger
constexpr uint8_t signalPin2 = 34;  // Cycle end + relay control trigger
constexpr uint8_t motorPin  = 5;    // HIGH = ON, LOW = OFF
constexpr uint8_t extraPin  = 4;    // mirrors motorPin
constexpr uint8_t statusLed = 2;    // blinks during active cycle

//------------- UART / Modbus constants --------------
constexpr uint32_t BAUD_RATE = 19200;
constexpr uint8_t  SLAVE_ID  = 1;
constexpr uint8_t  FUNC_CODE = 3;

//------------- Timing (ANTI-SWAP Protection) --------
constexpr uint16_t SENSOR_REPLY_TIMEOUT_MS = 3000;  // Camera processing time
constexpr uint16_t DEBOUNCE_DELAY_MS = 300;
constexpr uint16_t RELAY_ON_DURATION_MS = 500;
constexpr uint16_t RS485_TX_DELAY_US = 500;   // Increased for stability
constexpr uint16_t RS485_RX_DELAY_US = 200;   // Increased for stability
constexpr uint16_t FRAME_GAP_MS = 50;         // Increased gap between frames
constexpr uint16_t BUFFER_CLEAR_DELAY_MS = 100; // Time to clear buffer thoroughly

//------------- ANTI-SWAP Protection Variables -------
struct CycleData {
  uint32_t cycleId;
  uint32_t startTime;
  bool resultReceived;
  bool resultValue;
  uint8_t rawResponse[5];
  bool isValid;
};

CycleData currentCycle;
uint32_t globalCycleCounter = 0;

//------------- Cycle control variables --------------
bool cycleActive = false;
bool frameReceived = false;

//------------- Relay control variables --------------
bool relayControlActive = false;
uint32_t relayStartTime = 0;

//------------- Globals ------------------------------
uint32_t lastDebounceTime = 0;
uint32_t lastDebounceTime2 = 0;
uint32_t lastFrameTime = 0;

//================ CRC-16 (Modbus) ===================
uint16_t crc16(const uint8_t* buf, uint8_t len) {
  uint16_t crc = 0xFFFF;
  while (len--) {
    crc ^= *buf++;
    for (uint8_t i = 0; i < 8; ++i)
      crc = (crc & 1) ? (crc >> 1) ^ 0xA001 : crc >> 1;
  }
  return crc;
}

//================ RS-485 direction (Enhanced) =======
inline void setTx() { 
  digitalWrite(DE_RE_PIN, HIGH); 
  delayMicroseconds(RS485_TX_DELAY_US);  // Enhanced delay
}

inline void setRx() { 
  delayMicroseconds(RS485_RX_DELAY_US);  // Enhanced delay
  digitalWrite(DE_RE_PIN, LOW);  
}

//================ COMPREHENSIVE Buffer Clear ========
void aggressiveClearUARTBuffer() {
  Serial.println("🧹 AGGRESSIVE buffer clearing started...");
  
  // Multiple clearing passes
  for (int pass = 0; pass < 3; pass++) {
    int cleared = 0;
    uint32_t clearStart = millis();
    
    while (Serial2.available() && (millis() - clearStart < 50)) {
      uint8_t garbage = Serial2.read();
      Serial.print("🗑️ Pass ");
      Serial.print(pass + 1);
      Serial.print(" cleared: 0x");
      if (garbage < 0x10) Serial.print("0");
      Serial.println(garbage, HEX);
      cleared++;
      delay(2);  // Small delay between reads
    }
    
    if (cleared == 0) break;  // No more data to clear
    delay(10);  // Delay between passes
  }
  
  // Final verification
  delay(BUFFER_CLEAR_DELAY_MS);  // Wait for any delayed data
  int finalCheck = 0;
  while (Serial2.available()) {
    Serial2.read();
    finalCheck++;
  }
  
  if (finalCheck > 0) {
    Serial.print("⚠️ Final clear removed ");
    Serial.print(finalCheck);
    Serial.println(" additional bytes");
  }
  
  Serial.println("✅ Buffer clearing completed - UART buffer is clean");
}

//================ Initialize Cycle Data =============
void initializeCycleData() {
  globalCycleCounter++;
  currentCycle.cycleId = globalCycleCounter;
  currentCycle.startTime = millis();
  currentCycle.resultReceived = false;
  currentCycle.resultValue = false;
  currentCycle.isValid = false;
  
  // Clear raw response buffer
  for (int i = 0; i < 5; i++) {
    currentCycle.rawResponse[i] = 0;
  }
  
  Serial.print("🆔 Initialized Cycle Data - ID: ");
  Serial.print(currentCycle.cycleId);
  Serial.print(", Start Time: ");
  Serial.println(currentCycle.startTime);
}

//================ Validate Cycle Integrity ==========
bool validateCycleIntegrity() {
  if (!currentCycle.isValid) {
    Serial.println("❌ Cycle integrity check FAILED - Invalid cycle data");
    return false;
  }
  
  if (!currentCycle.resultReceived) {
    Serial.println("❌ Cycle integrity check FAILED - No result received");
    return false;
  }
  
  uint32_t cycleAge = millis() - currentCycle.startTime;
  if (cycleAge > 10000) {  // 10 second max cycle age
    Serial.println("❌ Cycle integrity check FAILED - Cycle too old");
    return false;
  }
  
  Serial.print("✅ Cycle integrity check PASSED - ID: ");
  Serial.print(currentCycle.cycleId);
  Serial.print(", Age: ");
  Serial.print(cycleAge);
  Serial.println("ms");
  
  return true;
}

//================ Enhanced Frame Communication ======
bool querySensorWithValidation(bool& sensorStatus) {
  // Ensure minimum gap between frames
  while (millis() - lastFrameTime < FRAME_GAP_MS) {
    delay(5);
  }
  
  // CRITICAL: Aggressive buffer clearing
  aggressiveClearUARTBuffer();
  
  Serial.print("📡 Sending Modbus frame for Cycle ID: ");
  Serial.println(currentCycle.cycleId);
  
  // Build Modbus RTU frame
  uint8_t frame[8] = { SLAVE_ID, FUNC_CODE, 0x00, 0x00, 0x00, 0x01 };
  uint16_t crc = crc16(frame, 6);
  frame[6] = crc & 0xFF;        // CRC Low
  frame[7] = (crc >> 8) & 0xFF; // CRC High

  // Print frame being sent
  Serial.print("📤 TX Frame (Cycle ");
  Serial.print(currentCycle.cycleId);
  Serial.print("): ");
  for (int i = 0; i < 8; i++) {
    Serial.print("0x");
    if (frame[i] < 0x10) Serial.print("0");
    Serial.print(frame[i], HEX);
    Serial.print(" ");
  }
  Serial.println();

  // Send frame with enhanced RS-485 control
  setTx();  // Switch to transmit mode with delay
  
  for (int i = 0; i < 8; i++) {
    Serial2.write(frame[i]);
    delayMicroseconds(50);  // Increased byte delay
  }
  Serial2.flush();  // Wait for transmission to complete
  
  setRx();  // Switch to receive mode with delay
  
  // Additional delay before starting to receive
  delay(10);

  // Receive reply with cycle tracking
  uint8_t reply[5];
  uint8_t idx = 0;
  uint32_t t0 = millis();
  bool receiving = false;
  uint32_t lastProgressTime = millis();
  
  Serial.print("⏱️ Waiting for camera response (Cycle ");
  Serial.print(currentCycle.cycleId);
  Serial.print(", timeout: ");
  Serial.print(SENSOR_REPLY_TIMEOUT_MS);
  Serial.println("ms)");
  Serial.println("📸 Camera is taking photo and processing...");
  
  while (millis() - t0 < SENSOR_REPLY_TIMEOUT_MS) {
    if (Serial2.available()) {
      if (!receiving) {
        receiving = true;
        Serial.print("📥 Camera response started for Cycle ");
        Serial.println(currentCycle.cycleId);
      }
      
      reply[idx] = Serial2.read();
      currentCycle.rawResponse[idx] = reply[idx];  // Store in cycle data
      
      Serial.print("📥 Cycle ");
      Serial.print(currentCycle.cycleId);
      Serial.print(" Byte ");
      Serial.print(idx);
      Serial.print(": 0x");
      if (reply[idx] < 0x10) Serial.print("0");
      Serial.print(reply[idx], HEX);
      Serial.print(" (");
      Serial.print(reply[idx]);
      Serial.println(")");
      
      idx++;
      
      // Break if we have enough bytes
      if (idx >= 3) break;
      
      // Reset timeout when receiving data
      t0 = millis();
    }
    
    // Show progress every second while waiting for camera
    if (millis() - lastProgressTime >= 1000) {
      uint32_t elapsed = millis() - t0;
      Serial.print("📸 Still waiting for Cycle ");
      Serial.print(currentCycle.cycleId);
      Serial.print("... ");
      Serial.print(elapsed);
      Serial.print("ms / ");
      Serial.print(SENSOR_REPLY_TIMEOUT_MS);
      Serial.println("ms");
      lastProgressTime = millis();
    }
    
    delay(10);  // Appropriate delay for camera processing
  }
  
  lastFrameTime = millis();  // Update frame timing
  
  // Validation with cycle tracking
  if (idx == 0) {
    Serial.print("❌ Camera timeout for Cycle ");
    Serial.print(currentCycle.cycleId);
    Serial.println(" - no response received");
    return false;
  }
  
  if (idx < 3) {
    Serial.print("⚠️ Partial camera response for Cycle ");
    Serial.print(currentCycle.cycleId);
    Serial.print(" - only received ");
    Serial.print(idx);
    Serial.println(" bytes (expected 3)");
    return false;
  }
  
  // Validate response format
  if (reply[0] != SLAVE_ID) {
    Serial.print("❌ Wrong slave ID for Cycle ");
    Serial.print(currentCycle.cycleId);
    Serial.print(" - Expected: ");
    Serial.print(SLAVE_ID);
    Serial.print(", Got: ");
    Serial.println(reply[0]);
    return false;
  }
  
  if (reply[1] != FUNC_CODE) {
    Serial.print("❌ Wrong function code for Cycle ");
    Serial.print(currentCycle.cycleId);
    Serial.print(" - Expected: ");
    Serial.print(FUNC_CODE);
    Serial.print(", Got: ");
    Serial.println(reply[1]);
    return false;
  }
  
  // Extract and validate result
  sensorStatus = (reply[2] == 1);
  currentCycle.resultValue = sensorStatus;
  currentCycle.resultReceived = true;
  currentCycle.isValid = true;
  
  Serial.print("✅ REAL-TIME Camera result for Cycle ");
  Serial.print(currentCycle.cycleId);
  Serial.print(" - Image quality: ");
  Serial.println(sensorStatus ? "GOOD (1)" : "BAD (0)");
  Serial.print("📊 Cycle ");
  Serial.print(currentCycle.cycleId);
  Serial.print(" response: [");
  Serial.print(reply[0]); Serial.print(", ");
  Serial.print(reply[1]); Serial.print(", ");
  Serial.print(reply[2]); Serial.println("]");
  
  return true;
}

//================ Relay control update ==============
void updateRelayControl() {
  if (relayControlActive) {
    if (millis() - relayStartTime >= RELAY_ON_DURATION_MS) {
      digitalWrite(motorPin, LOW);
      digitalWrite(extraPin, LOW);
      relayControlActive = false;
      Serial.println("🟢 Relay AUTO-OFF after duration");
      Serial.print("🔌 GPIO5 (motorPin): LOW, GPIO4 (extraPin): LOW at ");
      Serial.println(millis());
    }
  }
}

//================ Relay control with validation =====
void activateRelayControl() {
  if (!relayControlActive) {
    relayControlActive = true;
    relayStartTime = millis();
    digitalWrite(motorPin, HIGH);
    digitalWrite(extraPin, HIGH);
    Serial.print("🔴 Relay ACTIVATED for Cycle ");
    Serial.print(currentCycle.cycleId);
    Serial.println(" (BAD image detected)");
    Serial.print("🔌 GPIO5 (motorPin): HIGH, GPIO4 (extraPin): HIGH at ");
    Serial.println(millis());
  } else {
    Serial.println("⚠️ Relay already active, ignoring trigger");
  }
}

//================ Force end with cleanup ============
void forceEndCycleWithCleanup(const char* reason) {
  if (cycleActive) {
    Serial.print("🛑 FORCE ENDING Cycle ");
    Serial.print(currentCycle.cycleId);
    Serial.print(" - Reason: ");
    Serial.println(reason);
    
    cycleActive = false;
    frameReceived = false;
    digitalWrite(statusLed, LOW);
    
    // Clear any remaining UART data
    aggressiveClearUARTBuffer();
    
    Serial.println("═══════════════════════════════════════\n");
  }
}

//================ Setup =============================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n🚀 ESP32 RS-485 Camera System - ANTI-SWAP Protection");
  Serial.println("📌 COMPREHENSIVE SOLUTION for preventing result swapping:");
  Serial.println("📌 • Aggressive buffer clearing with multiple passes");
  Serial.println("📌 • Cycle ID tracking and validation");
  Serial.println("📌 • Enhanced RS-485 timing and delays");
  Serial.println("📌 • Result integrity verification");
  Serial.println("📌 • GPIO33: Start cycle → Camera photo → REAL-TIME result");
  Serial.println("📌 • GPIO34: End cycle → Validated relay control");
  
  // I/O setup
  pinMode(DE_RE_PIN, OUTPUT);   
  setRx();
  pinMode(statusLed, OUTPUT);   digitalWrite(statusLed, LOW);
  pinMode(motorPin, OUTPUT);    digitalWrite(motorPin, LOW);
  pinMode(extraPin, OUTPUT);    digitalWrite(extraPin, LOW);
  pinMode(signalPin, INPUT_PULLUP);
  pinMode(signalPin2, INPUT_PULLUP);

  // RS-485 UART
  Serial2.begin(BAUD_RATE, SERIAL_8E1, RXD2_PIN, TXD2_PIN);
  delay(100);
  
  // Initial aggressive buffer clearing
  aggressiveClearUARTBuffer();

  Serial.println("⚡ ANTI-SWAP System Ready");
  Serial.print("📌 Buffer Clear Delay: "); Serial.print(BUFFER_CLEAR_DELAY_MS); Serial.println("ms");
  Serial.print("📌 Frame Gap: "); Serial.print(FRAME_GAP_MS); Serial.println("ms");
  Serial.print("📌 RS-485 TX Delay: "); Serial.print(RS485_TX_DELAY_US); Serial.println("μs");
  Serial.print("📌 RS-485 RX Delay: "); Serial.print(RS485_RX_DELAY_US); Serial.println("μs");
  Serial.println("🔄 Waiting for GPIO33 to start first cycle...\n");
  
  lastFrameTime = millis();
}

//================ Main loop =============================
void loop() {
  updateRelayControl();

  // Handle GPIO33 (Cycle start) - ANTI-SWAP PROTECTED
  static int lastSig = HIGH;
  int curSig = digitalRead(signalPin);
  
  if (lastSig == HIGH && curSig == LOW) {
    if ((millis() - lastDebounceTime) > DEBOUNCE_DELAY_MS) {
      lastDebounceTime = millis();
      
      // Force end any active cycle
      if (cycleActive) {
        forceEndCycleWithCleanup("New GPIO33 trigger");
      }
      
      // Initialize NEW cycle with anti-swap protection
      cycleActive = true;
      frameReceived = false;
      digitalWrite(statusLed, HIGH);
      
      initializeCycleData();  // Set up cycle tracking
      
      Serial.println("═══════════════════════════════════════");
      Serial.print("🔄 NEW CYCLE STARTED - ID: ");
      Serial.println(currentCycle.cycleId);
      Serial.println("📡 Getting REAL-TIME Camera result with ANTI-SWAP protection...");
      Serial.println("📸 Requesting camera to take photo and analyze...");

      // Get REAL-TIME camera result with validation
      bool tempResult = false;
      if (querySensorWithValidation(tempResult)) {
        frameReceived = true;
        Serial.print("📊 CURRENT cycle (ID: ");
        Serial.print(currentCycle.cycleId);
        Serial.print(") camera result: ");
        Serial.println(tempResult ? "GOOD IMAGE" : "BAD IMAGE");
        Serial.println("⏳ Waiting for GPIO34 to complete cycle...");
      } else {
        Serial.print("❌ Camera communication failed for Cycle ");
        Serial.print(currentCycle.cycleId);
        Serial.println(" - ending cycle");
        forceEndCycleWithCleanup("Camera communication failure");
      }
    }
  }
  lastSig = curSig;

  // Handle GPIO34 (Cycle end) - VALIDATED RELAY CONTROL
  static int lastSig2 = HIGH;
  int curSig2 = digitalRead(signalPin2);
  
  if (lastSig2 == HIGH && curSig2 == LOW) {
    if ((millis() - lastDebounceTime2) > DEBOUNCE_DELAY_MS) {
      lastDebounceTime2 = millis();
      
      if (cycleActive && frameReceived) {
        Serial.print("🏁 GPIO34 TRIGGERED - Ending Cycle ");
        Serial.println(currentCycle.cycleId);
        
        // Validate cycle integrity before relay control
        if (validateCycleIntegrity()) {
          Serial.print("📊 Acting on VALIDATED result for Cycle ");
          Serial.print(currentCycle.cycleId);
          Serial.print(": ");
          Serial.println(currentCycle.resultValue ? "GOOD IMAGE" : "BAD IMAGE");
          
          if (!currentCycle.resultValue) {
            Serial.print("✅ Cycle ");
            Serial.print(currentCycle.cycleId);
            Serial.println(" result is BAD - Activating relay (defective product)");
            activateRelayControl();
          } else {
            Serial.print("🚫 Cycle ");
            Serial.print(currentCycle.cycleId);
            Serial.println(" result is GOOD - No relay activation (product OK)");
          }
        } else {
          Serial.print("❌ Cycle ");
          Serial.print(currentCycle.cycleId);
          Serial.println(" integrity validation FAILED - No relay action");
        }
        
        // End cycle cleanly
        cycleActive = false;
        frameReceived = false;
        digitalWrite(statusLed, LOW);
        Serial.print("🔄 CYCLE ");
        Serial.print(currentCycle.cycleId);
        Serial.println(" COMPLETED");
        Serial.println("═══════════════════════════════════════\n");
        
      } else if (cycleActive && !frameReceived) {
        Serial.println("⚠️ GPIO34 triggered but frame not ready yet - ignored");
      } else {
        Serial.println("⚠️ GPIO34 ignored - No active cycle! Start with GPIO33 first.");
      }
    }
  }
  lastSig2 = curSig2;
}