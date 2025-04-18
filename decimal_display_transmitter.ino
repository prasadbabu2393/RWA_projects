
// #include <HardwareSerial.h>

// // Define pins
// #define RE_DE_PIN 21  // RS485 Direction control pin

// // Define UART2 pins for ESP32
// #define RX2_PIN 16
// #define TX2_PIN 17

// // Buffer for Modbus communication
// uint8_t modbusBuffer[256];

// // Function to calculate Modbus RTU CRC
// uint16_t calculateCRC(uint8_t *buffer, int length) {
//   uint16_t crc = 0xFFFF;
  
//   for (int pos = 0; pos < length; pos++) {
//     crc ^= (uint16_t)buffer[pos];
    
//     for (int i = 8; i != 0; i--) {
//       if ((crc & 0x0001) != 0) {
//         crc >>= 1;
//         crc ^= 0xA001;
//       } else {
//         crc >>= 1;
//       }
//     }
//   }
  
//   return crc;
// }

// // Send a Modbus request frame
// void sendModbusRequest(uint8_t slaveId, uint8_t functionCode, uint16_t startAddress, uint16_t quantity) {
//   // Create a Modbus RTU request frame
//   uint8_t requestFrame[8];
//   requestFrame[0] = slaveId;                 // Slave ID
//   requestFrame[1] = functionCode;            // Function code
//   requestFrame[2] = highByte(startAddress);  // Start address high byte
//   requestFrame[3] = lowByte(startAddress);   // Start address low byte
//   requestFrame[4] = highByte(quantity);      // Quantity high byte
//   requestFrame[5] = lowByte(quantity);       // Quantity low byte
  
//   // Calculate CRC
//   uint16_t crc = calculateCRC(requestFrame, 6);
//   requestFrame[6] = lowByte(crc);            // CRC low byte
//   requestFrame[7] = highByte(crc);           // CRC high byte
  
//   // Clear any existing data in the buffer
//   while (Serial2.available()) Serial2.read();
  
//   // Set RS485 to transmit mode
//   digitalWrite(RE_DE_PIN, HIGH);
//   delay(5); // Small delay to ensure the line has stabilized
  
//   // Send the frame
//   Serial2.write(requestFrame, 8);
//   Serial2.flush(); // Wait for transmission to complete
  
//   // Set RS485 back to receive mode
//   delay(5);
//   digitalWrite(RE_DE_PIN, LOW);
  
//   Serial.print("Sent Modbus request: Slave ID=");
//   Serial.print(slaveId);
//   Serial.print(", Function=0x");
//   Serial.print(functionCode, HEX);
//   Serial.print(", Address=");
//   Serial.print(startAddress);
//   Serial.print(", Quantity=");
//   Serial.println(quantity);
// }

// // Read Modbus response with timeout
// int readModbusResponse(uint8_t *buffer, unsigned long timeout) {
//   unsigned long startTime = millis();
//   int bytesRead = 0;
  
//   while (millis() - startTime < timeout) {
//     if (Serial2.available()) {
//       // Reset timeout when data starts arriving
//       if (bytesRead == 0) {
//         startTime = millis();
//       }
      
//       // Read data into buffer
//       buffer[bytesRead++] = Serial2.read();
      
//       // Prevent buffer overflow
//       if (bytesRead >= 256) {
//         break;
//       }
//     }
    
//     // Simple check for complete frame - actual validation happens outside this function
//     if (bytesRead >= 5) {  // Minimum valid frame size
//       if (millis() - startTime > 50 && !Serial2.available()) {
//         // If no new data for 50ms, assume frame is complete
//         break;
//       }
//     }
    
//     yield(); // Allow other tasks to run
//   }
  
//   return bytesRead;
// }

// // Process received Modbus response
// bool processModbusResponse(uint8_t *buffer, int length, uint8_t expectedSlaveId, uint8_t expectedFunction) {
//   // Check minimum length for a Modbus response
//   if (length < 5) {
//     Serial.println("Invalid response: too short");
//     return false;
//   }
  
//   // Validate CRC
//   uint16_t receivedCRC = (buffer[length-1] << 8) | buffer[length-2];
//   uint16_t calculatedCRC = calculateCRC(buffer, length-2);
  
//   if (receivedCRC != calculatedCRC) {
//     Serial.println("Invalid response: CRC error");
//     return false;
//   }
  
//   // Check slave ID
//   if (buffer[0] != expectedSlaveId) {
//     Serial.println("Response from unexpected slave");
//     return false;
//   }
  
//   // Check for exception response
//   if (buffer[1] == (expectedFunction | 0x80)) {
//     Serial.print("Exception response received. Code: 0x");
//     Serial.println(buffer[2], HEX);
//     return false;
//   }
  
//   // Check function code
//   if (buffer[1] != expectedFunction) {
//     Serial.println("Unexpected function code in response");
//     return false;
//   }
  
//   return true;
// }

// // Extract register values from response
// void extractRegisterValues(uint8_t *buffer, uint16_t *registers, int registerCount) {
//   for (int i = 0; i < registerCount; i++) {
//     registers[i] = (buffer[3 + (i * 2)] << 8) | buffer[4 + (i * 2)];
//   }
// }

// // Print buffer contents in HEX format
// void printBuffer(uint8_t *buffer, int length) {
//   for (int i = 0; i < length; i++) {
//     Serial.print("0x");
//     if (buffer[i] < 16) Serial.print("0"); // Add leading zero for values less than 0x10
//     Serial.print(buffer[i], HEX);
//     Serial.print(" ");
//   }
//   Serial.println();
// }

// // Function to convert two 16-bit registers into a float (MSB first)
// float registersToFloat(uint16_t regHigh, uint16_t regLow) {
//   uint32_t combined = ((uint32_t)regHigh << 16) | regLow; // Combine registers
//   float value;
//   memcpy(&value, &combined, sizeof(value)); // Interpret bits as float
//   return value;
// }

// void setup() {
//   // Initialize USB serial for debugging
//   Serial.begin(115200);
//   while (!Serial) delay(10);  // Wait for serial port to connect
  
//   Serial.println("\nRS485 Modbus RTU Communication");
  
//   // Initialize RS485 direction control pin
//   pinMode(RE_DE_PIN, OUTPUT);
//   digitalWrite(RE_DE_PIN, LOW);  // Start in receive mode
  
//   // Initialize UART2 for Modbus communication
//   Serial2.begin(115200, SERIAL_8N1, RX2_PIN, TX2_PIN);
  
//   Serial.println("UART initialized with 115200 baud, 8-N-1");
//   Serial.println("RS485 ready for communication");
// }

// void loop() {
//   uint8_t slaveId = 1;  // Target device ID
//   uint16_t registers[4];  // Buffer for register values
  
//   // Read vacuum1 from address 3000 (Function code 0x03 - Read Holding Registers)
//   Serial.println("\n--- Reading Vacuum1 (Address 3000-3001) ---");
//   sendModbusRequest(slaveId, 0x03, 3001, 2);  // Read 2 registers for the float value
  
//   // Wait for and process response
//   int bytesRead = readModbusResponse(modbusBuffer, 1000);  // 1 second timeout
  
//   if (bytesRead > 0) {
//     Serial.print("Received ");
//     Serial.print(bytesRead);
//     Serial.println(" bytes:");
//     printBuffer(modbusBuffer, bytesRead);
    
//     if (processModbusResponse(modbusBuffer, bytesRead, slaveId, 0x03)) {
//       // Extract register values if it's a valid response
//       int registerCount = modbusBuffer[2] / 2;  // Byte count divided by 2 gives register count
//       extractRegisterValues(modbusBuffer, registers, registerCount);
      
//       // Convert registers to float
//       float vacuum1 = registersToFloat(registers[0], registers[1]);
//       Serial.print("Vacuum1 Value: ");
//       Serial.println(vacuum1, 6);  // Print with 6 digits of precision
//     }
//   } else {
//     Serial.println("No response received for Vacuum1");
//   }
  
  
//   Serial.println("\n--- Reading Both Vacuum Values (Address 3000-3003) ---");
//   sendModbusRequest(slaveId, 0x03, 3000, 4);  // Read 4 registers for both float values
  
//   bytesRead = readModbusResponse(modbusBuffer, 1000);  // 1 second timeout
  
//   if (bytesRead > 0) {
//     Serial.print("Received ");
//     Serial.print(bytesRead);
//     Serial.println(" bytes:");
//     printBuffer(modbusBuffer, bytesRead);
    
//     if (processModbusResponse(modbusBuffer, bytesRead, slaveId, 0x03)) {
//       // Extract register values if it's a valid response
//       int registerCount = modbusBuffer[2] / 2;  // Byte count divided by 2 gives register count
//       extractRegisterValues(modbusBuffer, registers, registerCount);
      
//       // Convert registers to float
//       float vacuum1 = registersToFloat(registers[0], registers[1]);
//       float vacuum2 = registersToFloat(registers[2], registers[3]);
      
//       Serial.print("Combined Read - Vacuum1: ");
//       Serial.print(vacuum1, 6);
//       Serial.print(", Vacuum2: ");
//       Serial.println(vacuum2, 6);
//     }
//   } else {
//     Serial.println("No response received for combined read");
//   }
  
//   // Wait before next communication cycle
//   delay(4000);
// }


#include <HardwareSerial.h>

#define RXD2 16  // ESP32 UART2 RX pin
#define TXD2 17  // ESP32 UART2 TX pin
#define DE_RE_pin 21  // RS485 DE & RE Pin (Direction control)

HardwareSerial Modbus(2);  // Use UART2 for Modbus

// Request: Slave ID = 1, Function = 0x03, Start Addr = 3000 (0x0BB8), Read 2 registers
byte TxData[8] = { 0x01, 0x03, 0x0B, 0xBA, 0x00, 0x02 };
uint8_t numRegs = TxData[5];
byte RxData[15];  // Buffer for response

float bytesToFloat(uint8_t *data) {
  uint32_t value = 0;

  // Combine 4 bytes into a 32-bit unsigned integer (Big Endian: MSB first)
  value |= ((uint32_t)data[0] << 24);
  value |= ((uint32_t)data[1] << 16);
  value |= ((uint32_t)data[2] << 8);
  value |= ((uint32_t)data[3]);

  // Convert uint32_t to float
  float result;
  memcpy(&result, &value, sizeof(result)); // Copy bit pattern safely
  return result;
}


void setup() {
  Serial.begin(115200);  // For debug output
  Modbus.begin(115200, SERIAL_8N1, RXD2, TXD2);  // Modbus Serial Port
  pinMode(DE_RE_pin, OUTPUT);
  digitalWrite(DE_RE_pin, LOW);  // Set to receive mode
  Serial.println("Modbus RTU Master Initialized for Address 3000");

   delay(100);  // Wait for response

       uint16_t crc = ModRTU_CRC(TxData, sizeof(TxData));  // Calculate CRC

     digitalWrite(DE_RE_pin, HIGH);  // Enable TX mode
     delay(20);  // Small delay for stable TX mode

     Serial.println("sending");

     // Send request
    //  TxData[6] = (crc & 0xFF); 
    //  TxData[7] = (crc >> 8) & 0xFF; 
    //       Modbus.write(TxData, 8);
    for(uint8_t a =0; a<6; a++)
     {
      Serial.print(TxData[a]);
      Serial.print(" ");
     }
     Serial.println();
     Serial.println(crc & 0xFF);  // CRC Low byte
     Serial.println((crc >> 8) & 0xFF);  // CRC High byte

     Modbus.write(TxData, sizeof(TxData));
     Modbus.write(crc & 0xFF);  // CRC Low byte
     Modbus.write((crc >> 8) & 0xFF);  // CRC High byte
      Modbus.flush();  // Ensure all data sent

     digitalWrite(DE_RE_pin, LOW);  // Enable RX mode

}

void loop() {
 

  // Read response
  uint8_t len = Modbus.available();
  if (len > 0) 
  {
     Serial.println("Receiving");
    for (uint8_t i = 0; i < len && i < sizeof(RxData); i++) {
      RxData[i] = Modbus.read();
    }

    Serial.println("Received Data:");
    for (uint8_t i = 0; i < len; i++) {
      Serial.print(RxData[i], HEX);
      Serial.print(" ");
    }
    Serial.println();

    uint8_t receivedBytes[4] = {RxData[3], RxData[4], RxData[5], RxData[6]};

    float value1 = bytesToFloat(receivedBytes);
    Serial.println("value1");
    Serial.println(value1);

    // CRC check
    uint16_t crcReceived = (RxData[len - 1] << 8) | RxData[len - 2];
    uint16_t crcCalc = ModRTU_CRC(RxData, len - 2);

    if (crcReceived == crcCalc) {
      Serial.println("CRC Valid!");

      // Extract 32-bit value (two 16-bit registers)
      uint32_t highWord = (RxData[3] << 8) | RxData[4];
      uint32_t lowWord = (RxData[5] << 8) | RxData[6];
      uint32_t value = ((uint32_t)highWord << 16) | lowWord;

      Serial.print("32-bit Value from Address 3000: ");
      Serial.println(value);

     delay(1000);  // Wait for response

       uint16_t crc = ModRTU_CRC(TxData, sizeof(TxData));  // Calculate CRC

     digitalWrite(DE_RE_pin, HIGH);  // Enable TX mode
     delay(20);  // Small delay for stable TX mode

     Serial.println("sending");

     // Send request
    //       TxData[6] = (crc & 0xFF); 
    //  TxData[7] = (crc >> 8) & 0xFF; 
    //  Modbus.write(TxData, 8);
     for(uint8_t a =0; a<6; a++)
     {
      Serial.print(TxData[a]);
      Serial.print(" ");
     }
     Serial.println(crc & 0xFF);  // CRC Low byte
     Serial.println((crc >> 8) & 0xFF);  // CRC High byte
     Serial.println();

     Modbus.write(TxData, sizeof(TxData));
     Modbus.write(crc & 0xFF);  // CRC Low byte
     Modbus.write((crc >> 8) & 0xFF);  // CRC High byte
     Modbus.flush();  // Ensure all data sent

     digitalWrite(DE_RE_pin, LOW);  // Enable RX mode


    } else {
      Serial.println("CRC Invalid!");
    }
  } else {
   // Serial.println("No response from slave");
  }

 // delay(1000);  // Query every second
}

// CRC Calculation Function
uint16_t ModRTU_CRC(byte *buf, int len) {
  uint16_t crc = 0xFFFF;

  for (int pos = 0; pos < len; pos++) {
    crc ^= (uint16_t)buf[pos];  // XOR byte into least sig. byte of crc
    for (int i = 8; i != 0; i--) {  // Loop over each bit
      if ((crc & 0x0001) != 0) {  // If the LSB is set
        crc >>= 1;  // Shift right and XOR 0xA001
        crc ^= 0xA001;
      } else
        crc >>= 1;  // Just shift right
    }
  }
  return crc;
}
