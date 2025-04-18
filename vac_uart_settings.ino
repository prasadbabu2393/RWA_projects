
void loadSettings() {
    preferences.begin("uart_config", false);
    uart_baud = preferences.getInt("baud", 115200);
    uart_data_bits = preferences.getInt("data_bits", 8);
    uart_parity = preferences.getInt("parity", 0);
    uart_stop_bits = preferences.getInt("stop_bits", 1);
    modbus_slave_id = preferences.getInt("slave_id", 1);
    preferences.end();
    
    // Initialize UART with the loaded settings
    initUART();
}

void saveSettings() {
    preferences.begin("uart_config", false);
    preferences.putInt("baud", uart_baud);
    preferences.putInt("data_bits", uart_data_bits);
    preferences.putInt("parity", uart_parity);
    preferences.putInt("stop_bits", uart_stop_bits);
    preferences.putInt("slave_id", modbus_slave_id);
    preferences.end();
}

void initUART() {
    // Clean up existing UART instance
    uart2.end();
    
    // Convert parity setting to SerialConfig
    SerialConfig config = SERIAL_8N1; // Default
    
    if (uart_data_bits == 8) {
        if (uart_parity == 0) { // None
            if (uart_stop_bits == 1) config = SERIAL_8N1;
            else config = SERIAL_8N2;
        } else if (uart_parity == 1) { // Odd
            if (uart_stop_bits == 1) config = SERIAL_8O1;
            else config = SERIAL_8O2;
        } else if (uart_parity == 2) { // Even
            if (uart_stop_bits == 1) config = SERIAL_8E1;
            else config = SERIAL_8E2;
        }
    } else if (uart_data_bits == 7) {
        if (uart_parity == 0) { // None
            if (uart_stop_bits == 1) config = SERIAL_7N1;
            else config = SERIAL_7N2;
        } else if (uart_parity == 1) { // Odd
            if (uart_stop_bits == 1) config = SERIAL_7O1;
            else config = SERIAL_7O2;
        } else if (uart_parity == 2) { // Even
            if (uart_stop_bits == 1) config = SERIAL_7E1;
            else config = SERIAL_7E2;
        }
    } else if (uart_data_bits == 6) {
        if (uart_parity == 0) { // None
            if (uart_stop_bits == 1) config = SERIAL_6N1;
            else config = SERIAL_6N2;
        } else if (uart_parity == 1) { // Odd
            if (uart_stop_bits == 1) config = SERIAL_6O1;
            else config = SERIAL_6O2;
        } else if (uart_parity == 2) { // Even
            if (uart_stop_bits == 1) config = SERIAL_6E1;
            else config = SERIAL_6E2;
        }
    } else if (uart_data_bits == 5) {
        if (uart_parity == 0) { // None
            if (uart_stop_bits == 1) config = SERIAL_5N1;
            else config = SERIAL_5N2;
        } else if (uart_parity == 1) { // Odd
            if (uart_stop_bits == 1) config = SERIAL_5O1;
            else config = SERIAL_5O2;
        } else if (uart_parity == 2) { // Even
            if (uart_stop_bits == 1) config = SERIAL_5E1;
            else config = SERIAL_5E2;
        }
    }
    
    // Initialize UART2 with the selected configuration and explicitly set pins
    uart2.begin(uart_baud, config, UART2_RX, UART2_TX);
    
    Serial.print("UART2 initialized: Baud ");
    Serial.print(uart_baud);
    Serial.print(", Data bits "); 
    Serial.print(uart_data_bits);
    Serial.print(", Parity ");  
    Serial.print(uart_parity);
    Serial.print(", Stop bits ");  
    Serial.println(uart_stop_bits);
}
