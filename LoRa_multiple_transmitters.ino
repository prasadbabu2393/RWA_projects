#include <HardwareSerial.h>

// Define pins
#define RXD2 16  // ESP32 Rx
#define TXD2 17  // ESP32 Tx
const int ledPin = 21;
const int buttonPin = 35;
//const int buttonPin = 2;

// Unique ID for this transmitter - change for each device (1-4)
const int TRANSMITTER_ID = 2;  // Change to 2, 3, or 4 for other devices

// Variables
String message = "";
int previousButtonState = HIGH;
unsigned long lastTransmissionTime = 0;
unsigned long transmissionInterval = 2000;  // Base transmission interval

// Add random offset to prevent consistent collisions
unsigned long randomOffset = 0;

void setup() {
  // Setup pins
  pinMode(ledPin, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(buttonPin, INPUT_PULLUP);  // Use INPUT_PULLUP for button
  
  // Initialize serial connections
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  
  Serial.println("LoRa AS32 Transmitter " + String(TRANSMITTER_ID) + " Started");
  
  // Initial LED states
  digitalWrite(ledPin, LOW);
  digitalWrite(32, LOW);
  digitalWrite(33, LOW);
  
  // Random seed based on analog noise
  randomSeed(analogRead(0));
  
  // Calculate random offset (100-500ms) for this transmitter
  randomOffset = 100 + (TRANSMITTER_ID * 100) + random(300);
}

void loop() {
  unsigned long currentTime = millis();
  int buttonState = digitalRead(buttonPin);
  
  // Check if it's time to transmit (based on ID to stagger transmissions)
  if (currentTime - lastTransmissionTime >= (transmissionInterval + randomOffset)) {
    // Prepare message with transmitter ID and status
    if (buttonState == LOW) {
      message = "TX" + String(TRANSMITTER_ID) + "_ON";
      digitalWrite(ledPin, HIGH);  // Visual feedback
    } else {
      message = "TX" + String(TRANSMITTER_ID) + "_OFF";
      digitalWrite(ledPin, LOW);   // Visual feedback
    }
    
    // Send the message
    Serial.println("Sending: " + message);
    Serial2.print(message);
    
    // Record transmission time
    lastTransmissionTime = currentTime;
    
    // Vary the random offset periodically to avoid repeated collisions
    if (random(10) > 7) {  // ~20% chance to change offset
      randomOffset = 100 + (TRANSMITTER_ID * 100) + random(300);
    }
  }
  
  // If button state changed, send immediately (responsive control)
  if (buttonState != previousButtonState) {
    // Small delay to debounce
    delay(50);
    
    // Confirm the button state after debounce
    buttonState = digitalRead(buttonPin);
    
    if (buttonState != previousButtonState) {
      // Prepare message with transmitter ID and status
      if (buttonState == LOW) {
        message = "TX" + String(TRANSMITTER_ID) + "_ON";
        digitalWrite(ledPin, HIGH);  // Visual feedback
      } else {
        message = "TX" + String(TRANSMITTER_ID) + "_OFF";
        digitalWrite(ledPin, LOW);   // Visual feedback
      }
      
      // Send the message
      Serial.println("Button changed - Sending: " + message);
      Serial2.print(message);
      
      // Record transmission time
      lastTransmissionTime = currentTime;
      previousButtonState = buttonState;
    }
  }
}