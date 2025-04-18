//  #include <HardwareSerial.h>

// #define RXD2 16  //esp32 Rx
// #define TXD2 17  //esp32 Tx
// const int ledpin = 21;
// // #include <SoftwareSerial.h>
// // SoftwareSerial AS32(3, 2);  // RX, TX for the LoRa AS32 module
// String s = "laralong10";

// void setup() {
//   pinMode(ledpin, OUTPUT);
//   pinMode(32, OUTPUT);
//   pinMode(33, OUTPUT);
//   pinMode(25, INPUT);
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
//   if(digitalRead(25) == LOW)
//   {
//     Serial.println("sending");
//      s = "lara_yes";
//      Serial2.print(s);
//      delay(2000);
//   }
//   else {
//     Serial.println("sending");
//     s = "lara_no";
//      Serial2.print(s);
//      delay(2000);
//   }
// }



#include <HardwareSerial.h>
#define RXD2 16  // ESP32 Rx
#define TXD2 17  // ESP32 Tx
const int ledpin = 21;
const int buttonPin = 25;  // Button input pin

// Unique ID for each transmitter (change for each transmitter: 1, 2, 3, or 4)
const int TRANSMITTER_ID = 4;  // Change this for each transmitter

void setup() {
  pinMode(ledpin, OUTPUT);
  pinMode(32, OUTPUT);
  pinMode(33, OUTPUT);
  pinMode(buttonPin, INPUT);
  
  Serial.begin(115200);       // Serial monitor baud rate
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);
  Serial.println("LoRa AS32 Transmitter " + String(TRANSMITTER_ID) + " Started");
  
  digitalWrite(ledpin, LOW);
  digitalWrite(32, LOW);
  digitalWrite(33, LOW);
}

void loop() {
  String message = "";
  
  if(digitalRead(buttonPin) == LOW) {
    message = "TX" + String(TRANSMITTER_ID) + "_YES";
    digitalWrite(ledpin, HIGH);  // Visual indicator
  } 
  else {
    message = "TX" + String(TRANSMITTER_ID) + "_NO";
    digitalWrite(ledpin, LOW);   // Visual indicator
  }
  
  Serial.println("Sending: " + message);
  Serial2.print(message);
  
  // Wait before sending the next message to avoid flooding
  delay(2000);
}