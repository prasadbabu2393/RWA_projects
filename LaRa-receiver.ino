 
// #include <HardwareSerial.h> // transistor receiver (receiver relay vin out1)

// #define RXD2 16  //esp32 Rx
// #define TXD2 17  //esp32 Tx
// const int ledpin = 21;
// // #include <SoftwareSerial.h>
// // SoftwareSerial AS32(3, 2);  // RX, TX for the LoRa AS32 module
// String s = "laralong10";
// String received = "";
// void setup() {
//   pinMode(ledpin, OUTPUT);
//   pinMode(32, OUTPUT);
//   pinMode(33, OUTPUT);
//   pinMode(26, OUTPUT);
//     //  digitalWrite(ledpin, HIGH);
//   Serial.begin(9600);       // Serial monitor baud rate
//   //AS32.begin(9600);         // AS32 module baud rate
//   Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
//   Serial.println("LoRa AS32 Communication Started");
//   digitalWrite(ledpin, LOW);
//   digitalWrite(32, LOW);
//   digitalWrite(33, LOW);
// }

// void loop() 
// {
//   // // Forward data from AS32 to Serial
//   while (Serial2.available()) 
//   {
//     received = Serial2.readString();
//     Serial.println("received");
//     Serial.println(received);
//     received.trim();  // Remove any leading/trailing whitespace
//     if(received == "lara_yes")
//     {
//     digitalWrite(ledpin, HIGH);
//     digitalWrite(26, HIGH);
//     }
//     else if(received == "lara_no")
//     {
//     digitalWrite(ledpin, LOW);
//     digitalWrite(26, LOW);
//     }
//   }
// }



#include <HardwareSerial.h>
#define RXD2 16  // ESP32 Rx
#define TXD2 17  // ESP32 Tx

// LED and output pins for each transmitter's status
const int ledpin = 21;    // General status LED
const int tx1OutputPin = 26;  // Output pin for Transmitter 1
const int tx2OutputPin = 27;  // Output pin for Transmitter 2
const int tx3OutputPin = 25;  // Output pin for Transmitter 3
const int tx4OutputPin = 23;  // Output pin for Transmitter 4

// Status tracking for each transmitter
bool tx1Status = false;
bool tx2Status = false;
bool tx3Status = false;
bool tx4Status = false;

// Last received time from each transmitter (for timeout detection)
unsigned long lastRxTime[4] = {0, 0, 0, 0};
const unsigned long TIMEOUT_MS = 20000;  // 20 seconds timeout

// Message buffering system
#define MAX_QUEUE_SIZE 20
String messageQueue[MAX_QUEUE_SIZE];
int queueHead = 0;
int queueTail = 0;
bool queueEmpty = true;

// Buffer for collecting incoming data
String incomingBuffer = "";
unsigned long lastCharTime = 0;
const unsigned long CHAR_TIMEOUT = 100; // 100ms timeout between characters

void setup() {
  pinMode(ledpin, OUTPUT);
  pinMode(tx1OutputPin, OUTPUT);
  pinMode(tx2OutputPin, OUTPUT);
  pinMode(tx3OutputPin, OUTPUT);
  pinMode(tx4OutputPin, OUTPUT);
  
  digitalWrite(ledpin, LOW);
  digitalWrite(tx1OutputPin, LOW);
  digitalWrite(tx2OutputPin, LOW);
  digitalWrite(tx3OutputPin, LOW);
  digitalWrite(tx4OutputPin, LOW);
   
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  digitalWrite(32, LOW);
  digitalWrite(33, LOW); 

  Serial.begin(115200);       // Higher baud rate for monitoring
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("LoRa AS32 Enhanced Receiver Started - Monitoring 4 Transmitters");
  
  // Flash LED to indicate successful startup
  for (int i = 0; i < 3; i++) {
    digitalWrite(ledpin, HIGH);
    delay(100);
    digitalWrite(ledpin, LOW);
    delay(100);
  }
}

void loop() {
  // Check for timeout on each transmitter
  checkTimeouts();
  
  // Read any available data from Serial2 into buffer
  readIncomingData();
  
  // Process any complete messages in the queue
  processMessageQueue();
}

void readIncomingData() {
  // Check if characters are available
  while (Serial2.available()) {
    char c = Serial2.read();
    lastCharTime = millis();
    
    // Add character to buffer
    incomingBuffer += c;
    
    // If we detect an end of message, add to queue
    if (c == '\n' || c == '\r') {
      if (incomingBuffer.length() > 2) {  // Ignore empty messages
        addToQueue(incomingBuffer);
        incomingBuffer = "";
      }
    }
  }
  
  // If we have data in buffer but no new characters for a while, consider it a complete message
  if (incomingBuffer.length() > 0 && (millis() - lastCharTime > CHAR_TIMEOUT)) {
    addToQueue(incomingBuffer);
    incomingBuffer = "";
  }
}

void addToQueue(String message) {
  message.trim();  // Remove whitespace
  
  // Check if the message is valid before adding to queue
  if (message.length() < 3 || !message.startsWith("TX")) {
    Serial.println("Ignored invalid message: " + message);
    return;
  }
  
  // Add to queue if not full
  if ((queueHead != queueTail) || queueEmpty) {
    messageQueue[queueTail] = message;
    queueTail = (queueTail + 1) % MAX_QUEUE_SIZE;
    queueEmpty = false;
    
    Serial.println("Queued: " + message + " (Queue size: " + getQueueSize() + ")");
  } else {
    Serial.println("WARNING: Message queue full, dropped: " + message);
  }
}

int getQueueSize() {
  if (queueEmpty) return 0;
  return (queueTail - queueHead + MAX_QUEUE_SIZE) % MAX_QUEUE_SIZE;
}

void processMessageQueue() {
  // Process one message per loop cycle
  if (!queueEmpty) {
    String message = messageQueue[queueHead];
    queueHead = (queueHead + 1) % MAX_QUEUE_SIZE;
    queueEmpty = (queueHead == queueTail);
    
    Serial.println("Processing: " + message);
    processMessage(message);
  }
}

void processMessage(String message) {
  // Parse the message to determine which transmitter and what status
  // Format: "TXn_ON" or "TXn_OFF" where n is the transmitter ID (1-4)
  
  // Extract transmitter ID and status
  if (message.startsWith("TX")) {
    int txID = 0;
    
    // Try to extract transmitter ID
    if (message.length() >= 3 && isDigit(message.charAt(2))) {
      txID = message.substring(2, 3).toInt();
    }
    
    if (txID >= 1 && txID <= 4) {
      // Check for proper message format
      if (message.indexOf("_ON") > 0) {
        // Update last received time
        lastRxTime[txID-1] = millis();
        updateTransmitterStatus(txID, true);
        indicateReceived();
      } 
      else if (message.indexOf("_OFF") > 0) {
        // Update last received time
        lastRxTime[txID-1] = millis();
        updateTransmitterStatus(txID, false);
        indicateReceived();
      }
      else {
        Serial.println("Invalid message format: " + message);
      }
    } else {
      Serial.println("Invalid transmitter ID in message: " + message);
    }
  } else {
    Serial.println("Message doesn't start with TX: " + message);
  }
}

void indicateReceived() {
  // Visual feedback that a message was processed
  digitalWrite(ledpin, HIGH);
  delay(50);
  digitalWrite(ledpin, LOW);
}

void updateTransmitterStatus(int txID, bool isYes) {
  int outputPin = ledpin;  // Default to status LED if ID is invalid
  
  // Select the correct output pin based on transmitter ID
  switch (txID) {
    case 1:
      outputPin = tx1OutputPin;
      tx1Status = isYes;
      break;
    case 2:
      outputPin = tx2OutputPin;
      tx2Status = isYes;
      break;
    case 3:
      outputPin = tx3OutputPin;
      tx3Status = isYes;
      break;
    case 4:
      outputPin = tx4OutputPin;
      tx4Status = isYes;
      break;
    default:
      Serial.println("Invalid transmitter ID");
      return;
  }
  
  // Update the output pin
  digitalWrite(outputPin, isYes ? HIGH : LOW);
  
  // Log the status change
  Serial.println("TX" + String(txID) + " status: " + (isYes ? "YES" : "NO"));
}

void checkTimeouts() {
  unsigned long currentTime = millis();
  
  // Check each transmitter for timeout
  for (int i = 0; i < 4; i++) {
    // If we haven't heard from this transmitter in a while and it's marked as active
    if ((currentTime - lastRxTime[i] > TIMEOUT_MS) && lastRxTime[i] != 0) {
      // Turn off the corresponding output
      int outputPin;
      switch (i+1) {
        case 1: outputPin = tx1OutputPin; tx1Status = false; break;
        case 2: outputPin = tx2OutputPin; tx2Status = false; break;
        case 3: outputPin = tx3OutputPin; tx3Status = false; break;
        case 4: outputPin = tx4OutputPin; tx4Status = false; break;
      }
      
      digitalWrite(outputPin, LOW);
      Serial.println("TX" + String(i+1) + " timed out - no signal received");
      
      // Reset last receive time to avoid continuous timeout messages
      lastRxTime[i] = 0;
    }
  }
}