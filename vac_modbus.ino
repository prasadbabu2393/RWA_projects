
// Convert float to two 16-bit registers according to IEEE 754
void floatToModbusRegisters(float value, uint16_t *registers) {
    FloatToBytes converter;
    converter.f = value;
    
    // Store in Modbus register format (big-endian)
    registers[0] = (converter.u32 >> 16) & 0xFFFF; // High word
    registers[1] = converter.u32 & 0xFFFF;         // Low word
}


// Calculate Modbus RTU CRC
uint16_t calculateCRC(uint8_t *buffer, int length) {
    uint16_t crc = 0xFFFF;
    
    for (int pos = 0; pos < length; pos++) {
        crc ^= (uint16_t)buffer[pos];
        
        for (int i = 8; i != 0; i--) {
            if ((crc & 0x0001) != 0) {
                crc >>= 1;
                crc ^= 0xA001;
            } else {
                crc >>= 1;
            }
        }
    }
    
    return crc;
}

// Update Modbus registers with current vacuum values
void updateModbusRegisters() {
    // vacuum1 in registers at address 3000-3001
    floatToModbusRegisters(vacuum1, &modbusRegisters[0]); 
    
    // vacuum2 in registers at address 3002-3003
    floatToModbusRegisters(vacuum2, &modbusRegisters[2]); 
}

// Process Modbus RTU request and send response
void processModbusRequest(uint8_t *buffer, int length) {
    // Check minimum length for a Modbus request
    if (length < 8) {
        Serial.println("Invalid Modbus request: too short");
        return;
    }
    
    // Calculate CRC of received message
    uint16_t receivedCRC = (buffer[length-1] << 8) | buffer[length-2];
    uint16_t calculatedCRC = calculateCRC(buffer, length-2);
    
    if (receivedCRC != calculatedCRC) {
        Serial.println("Invalid Modbus request: CRC error");
        return;
    }
    
    // Check if the request is for this slave
    if (buffer[0] != modbus_slave_id) {
        Serial.println("Modbus request not for this slave");
        return;
    }
    
    uint8_t functionCode = buffer[1];
    uint16_t startAddress = (buffer[2] << 8) | buffer[3];
    uint16_t registerCount = (buffer[4] << 8) | buffer[5];
    
    // Update modbus registers with current values
    updateModbusRegisters();
    
    // Process based on function code
    if (functionCode == 0x03 || functionCode == 0x04) { // Read Holding Registers or Read Input Registers
        // Check if address is in our supported range (3000-3003)
        if (startAddress < 3000 || startAddress > 3003) {  
            sendModbusException(functionCode, 0x02); // Illegal data address
            return;
        }
        
        // Calculate internal register offset (0-3)
        uint16_t internalStartAddress = startAddress - 3000;
        
        // Check if requested register count exceeds available registers
        if (internalStartAddress + registerCount > 4) {
            sendModbusException(functionCode, 0x02); // Illegal data address
            return;
        }
        
        // Prepare response
        uint8_t response[3 + (registerCount * 2) + 2]; // Header + data + CRC
        response[0] = modbus_slave_id;      // Slave ID
        response[1] = functionCode;         // Function code
        response[2] = registerCount * 2;    // Byte count
        
        // Copy register values to response
        for (int i = 0; i < registerCount; i++) {
            response[3 + (i * 2)] = (modbusRegisters[internalStartAddress + i] >> 8) & 0xFF; // High byte
            response[3 + (i * 2) + 1] = modbusRegisters[internalStartAddress + i] & 0xFF;    // Low byte
        }
        
        // Add CRC
        uint16_t crc = calculateCRC(response, 3 + (registerCount * 2));
        response[3 + (registerCount * 2)] = crc & 0xFF;       // CRC low byte
        response[3 + (registerCount * 2) + 1] = (crc >> 8) & 0xFF; // CRC high byte
        
        // Set RS485 to transmit mode
        digitalWrite(RE1, HIGH);
        delay(50); // Small delay to ensure the line has stabilized
        
        // Send response
        uart2.write(response, 3 + (registerCount * 2) + 2);
        uart2.flush(); // Wait for transmission to complete
        
        // Set RS485 back to receive mode
        delay(50);
        digitalWrite(RE1, LOW);
        
        for(uint8_t a=0; a< (registerCount * 2) + 5; a++)
        {
         Serial.print(response[a]);
         Serial.print(" ");
        }
        Serial.println();
        Serial.print("Sent Modbus response for address ");
        Serial.print(startAddress);
        Serial.print(", count: ");
        Serial.println(registerCount);
    } else {
        // Unsupported function code
        sendModbusException(functionCode, 0x01);
    }
}

// Send Modbus exception response
void sendModbusException(uint8_t functionCode, uint8_t exceptionCode) {
    uint8_t response[5];
    response[0] = modbus_slave_id;         // Slave ID
    response[1] = functionCode | 0x80;     // Function code with MSB set
    response[2] = exceptionCode;           // Exception code
    
    // Calculate CRC
    uint16_t crc = calculateCRC(response, 3);
    response[3] = crc & 0xFF;       // CRC low byte
    response[4] = (crc >> 8) & 0xFF; // CRC high byte
    
    // Set RS485 to transmit mode
    digitalWrite(RE1, HIGH);
    delay(5);
    
    // Send exception response
    uart2.write(response, 5);
    uart2.flush();
    
    // Set RS485 back to receive mode
    delay(5);
    digitalWrite(RE1, LOW);
    
    Serial.print("Sent Modbus exception: ");
    Serial.println(exceptionCode);
}

// // Update Modbus registers with current vacuum values
// void updateModbusRegisters() {
//     // vacuum1 in registers at address 3000-3001
//     floatToModbusRegisters(vacuum1, &modbusRegisters[0]); 
    
//     // vacuum2 in registers at address 4000-4001
//     floatToModbusRegisters(vacuum2, &modbusRegisters[2]); 
// }

// // Process Modbus RTU request and send response
// void processModbusRequest(uint8_t *buffer, int length) {
//     // Check minimum length for a Modbus request
//     if (length < 8) {
//         Serial.println("Invalid Modbus request: too short");
//         return;
//     }
    
//     // Calculate CRC of received message
//     uint16_t receivedCRC = (buffer[length-1] << 8) | buffer[length-2];
//     uint16_t calculatedCRC = calculateCRC(buffer, length-2);
    
//     if (receivedCRC != calculatedCRC) {
//         Serial.println("Invalid Modbus request: CRC error");
//         return;
//     }
    
//     // Check if the request is for this slave
//     if (buffer[0] != modbus_slave_id) {
//         Serial.println("Modbus request not for this slave");
//         return;
//     }
    
//     uint8_t functionCode = buffer[1];
//     uint16_t startAddress = (buffer[2] << 8) | buffer[3];
//     uint16_t registerCount = (buffer[4] << 8) | buffer[5];
    
//     // Update modbus registers with current values
//     updateModbusRegisters();
    
//     // Process based on function code
//     if (functionCode == 0x03 || functionCode == 0x04) { // Read Holding Registers or Read Input Registers
//         // Map the request address to our internal register array
//         uint16_t internalStartAddress;
//         uint16_t maxRegisters;
        
//         // Check if address is for vacuum1 (3000) or vacuum2 (4000)
//         if (startAddress >= 3000 && startAddress < 3002) {
//             internalStartAddress = 0; // vacuum1 is at internal address 0-1
//             maxRegisters = 2;
            
//             // Adjust startAddress to be relative to internal address
//             startAddress = startAddress - 3000;
//         } 
//         else if (startAddress >= 4000 && startAddress < 4002) {
//             internalStartAddress = 2; // vacuum2 is at internal address 2-3
//             maxRegisters = 2;
            
//             // Adjust startAddress to be relative to internal address
//             startAddress = startAddress - 4000 + 2;
//         }
//         else {
//             // Address out of supported range
//             sendModbusException(functionCode, 0x02); // Illegal data address
//             return;
//         }
        
//         // Check if number of registers requested is valid
//         if (registerCount > maxRegisters || 
//             (startAddress % 2 != 0 && registerCount > 1)) { // Prevent reading across float boundaries
//             sendModbusException(functionCode, 0x02); // Illegal data address
//             return;
//         }
        
//         // Prepare response
//         uint8_t response[3 + (registerCount * 2) + 2]; // Header + data + CRC
//         response[0] = modbus_slave_id;      // Slave ID
//         response[1] = functionCode;         // Function code
//         response[2] = registerCount * 2;    // Byte count
        
//         // Copy register values to response
//         for (int i = 0; i < registerCount; i++) {
//             response[3 + (i * 2)] = (modbusRegisters[startAddress + i] >> 8) & 0xFF; // High byte
//             response[3 + (i * 2) + 1] = modbusRegisters[startAddress + i] & 0xFF;    // Low byte
//         }
        
//         // Add CRC
//         uint16_t crc = calculateCRC(response, 3 + (registerCount * 2));
//         response[3 + (registerCount * 2)] = crc & 0xFF;       // CRC low byte
//         response[3 + (registerCount * 2) + 1] = (crc >> 8) & 0xFF; // CRC high byte
        
//         // Set RS485 to transmit mode
//         digitalWrite(RE1, HIGH);
//         delay(5); // Small delay to ensure the line has stabilized
        
//         // Send response
//         uart2.write(response, 3 + (registerCount * 2) + 2);
//         uart2.flush(); // Wait for transmission to complete
        
//         // Set RS485 back to receive mode
//         delay(5);
//         digitalWrite(RE1, LOW);
        
//         Serial.print("Sent Modbus response for address ");
//         Serial.println(startAddress + (startAddress < 2 ? 3000 : 4000));
//     } else {
//         // Unsupported function code
//         sendModbusException(functionCode, 0x01);
//     }
// }

// // Send Modbus exception response
// void sendModbusException(uint8_t functionCode, uint8_t exceptionCode) {
//     uint8_t response[5];
//     response[0] = modbus_slave_id;         // Slave ID
//     response[1] = functionCode | 0x80;     // Function code with MSB set
//     response[2] = exceptionCode;           // Exception code
    
//     // Calculate CRC
//     uint16_t crc = calculateCRC(response, 3);
//     response[3] = crc & 0xFF;       // CRC low byte
//     response[4] = (crc >> 8) & 0xFF; // CRC high byte
    
//     // Set RS485 to transmit mode
//     digitalWrite(RE1, HIGH);
//     delay(5);
    
//     // Send exception response
//     uart2.write(response, 5);
//     uart2.flush();
    
//     // Set RS485 back to receive mode
//     delay(5);
//     digitalWrite(RE1, LOW);
    
//     Serial.print("Sent Modbus exception: ");
//     Serial.println(exceptionCode);
// }
