
void handleRoot() {
    server.sendHeader("Location", "/uart", true);
    server.send(302, "text/plain", "");
}
void handleSaveSSID() {
    if (server.hasArg("ssid") && server.hasArg("password")) {
        String new_ssid = server.arg("ssid");
        String new_password = server.arg("password");
        
        // Validate inputs (optional but recommended)
        if (new_ssid.length() == 0 || new_password.length() < 8) {
            server.send(400, "text/html", "<h3>Invalid inputs. SSID cannot be empty and password must be at least 8 characters.</h3>");
            return;
        }
        
        // Update global variables
        wifi_ssid = new_ssid;
        wifi_password = new_password;
        
        // Save to preferences
        preferences.begin("settings", false);
        if(new_ssid != ""){
          preferences.putString("ssid", new_ssid);
        }
        if(new_password != ""){
        preferences.putString("password", new_password);
        }
        preferences.end();

        // Send success response
        server.send(200, "text/html", "<h3>WiFi settings saved! Restarting...</h3><script>setTimeout(function(){ window.location.href = '/'; }, 5000);</script>");
        
        // // Log changes
        // Serial.println("New WiFi settings:");
        // Serial.println("SSID: " + new_ssid);
        // Serial.println("Password: " + new_password);
        
        // Delay to allow the response to be sent before restarting
        delay(3000);
        ESP.restart(); // Restart ESP32 to apply WiFi settings
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void handleSSIDPage() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>WiFi Configuration</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            max-width: 400px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
            text-align: center;
        }
        h1 {
            color: #333;
        }
        label {
            display: block;
            margin-top: 15px;
            font-weight: bold;
            text-align: left;
        }
        input {
            width: 100%;
            padding: 10px;
            margin-top: 5px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        .password-container {
            position: relative;
            width: 100%;
        }
        .toggle-password {
            position: absolute;
            right: 10px;
            top: 50%;
            transform: translateY(-50%);
            cursor: pointer;
            font-size: 16px;
            background: none;
            border: none;
            color: gray;
        }
        .button {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 12px 20px;
            margin-top: 20px;
            border-radius: 4px;
            cursor: pointer;
            width: 100%;
            font-size: 16px;
        }
        .button:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>WiFi Configuration</h1>
        <form action="/save_wifi" method="post">
            <label for="ssid">SSID:</label>
            <input type="text" id="ssid" name="ssid" value="%SSID%" required>
            
            <label for="password">Password:</label>
            <div class="password-container">
                <input type="password" id="password" name="password" required>
                <button type="button" class="toggle-password" onclick="togglePassword()">Show</button>
            </div>
            
            <button type="submit" class="button">Save & Restart</button>
        </form>
    </div>

    <script>
        function togglePassword() {
            var passField = document.getElementById("password");
            var toggleBtn = document.querySelector(".toggle-password");
            
            if (passField.type === "password") {
                passField.type = "text";
                toggleBtn.textContent = "Hide";
            } else {
                passField.type = "password";
                toggleBtn.textContent = "Show";
            }
        }
    </script>
</body>
</html>
)rawliteral";

    html.replace("%SSID%", wifi_ssid);

    server.send(200, "text/html", html);
}

void handleTemperature() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <title>Sensor Readings</title>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
            text-align: center;
        }
        .container {
            max-width: 400px;
            margin: auto;
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        }
        h1 {
            color: #333;
        }
        .reading {
            font-size: 24px;
            font-weight: bold;
            color: #007bff;
            margin-top: 10px;
        }
    </style>
</head>
<body>
    <div class="container">
        <h1>Sensor Readings</h1>
        <p class="reading">Voltage 1: %VOLTAGE1% mV</p>
        <p class="reading">Voltage 2: %VOLTAGE2% mV</p>
        <p class="reading">Temperature: %TEMP1% &deg;C</p>
    </div>
</body>
</html>
)rawliteral";

    // Replace placeholders with actual sensor values
    html.replace("%VOLTAGE1%", String(voltage_1));
    html.replace("%VOLTAGE2%", String(voltage_2));
    html.replace("%TEMP1%", String(temperature_1, 2));  // Keeps 2 decimal places for temperature

    server.send(200, "text/html", html);
}


void handleSave() {
    if (server.hasArg("baud") &&
        server.hasArg("data_bits") &&
        server.hasArg("parity") &&
        server.hasArg("stop_bits") &&
        server.hasArg("slave_id")) {
        
        uart_baud = server.arg("baud").toInt();
        uart_data_bits = server.arg("data_bits").toInt();
        uart_parity = server.arg("parity").toInt();
        uart_stop_bits = server.arg("stop_bits").toInt();
        modbus_slave_id = server.arg("slave_id").toInt();
        
        // Validate slave ID
        if (modbus_slave_id < 1 || modbus_slave_id > 247) {
            modbus_slave_id = 1; // Reset to default if invalid
        }
        
        saveSettings();
        
        // String response = R"rawliteral(


        // server.send(200, "text/html", response);
        
        // Delay to allow the response to be sent before restarting
        delay(3000);
        ESP.restart();
    } else {
        server.send(400, "text/plain", "Missing parameters");
    }
}

void handleLogin() {
    String html = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <h1> MODBUS Configuration </h1>
    <meta name="viewport" content="width=device-width, initial-scale=1">
    <style>
        body {
            font-family: Arial, sans-serif;
            margin: 0;
            padding: 20px;
            background-color: #f5f5f5;
        }
        .container {
            max-width: 600px;
            margin: 0 auto;
            background-color: white;
            padding: 20px;
            border-radius: 8px;
            box-shadow: 0 2px 10px rgba(0, 0, 0, 0.1);
        }
        h1 {
            color: #333;
            text-align: center;
        }
        label {
            display: block;
            margin-top: 15px;
            font-weight: bold;
        }
        select, input[type="number"] {
            width: 100%;
            padding: 10px;
            margin-top: 5px;
            border: 1px solid #ddd;
            border-radius: 4px;
            box-sizing: border-box;
        }
        .button {
            background-color: #4CAF50;
            color: white;
            border: none;
            padding: 12px 20px;
            margin-top: 20px;
            border-radius: 4px;
            cursor: pointer;
            width: 100%;
            font-size: 16px;
        }
        .button:hover {
            background-color: #45a049;
        }
    </style>
</head>
<body>
    <div class="container">
        <form action="/save" method="post">            
            <label for="baud">Baud Rate:</label>
            <select id="baud" name="baud">
                <option value="1200" %BAUD1200_SELECTED%>1200</option>
                <option value="2400" %BAUD2400_SELECTED%>2400</option>
                <option value="4800" %BAUD4800_SELECTED%>4800</option>
                <option value="9600" %BAUD9600_SELECTED%>9600</option>
                <option value="19200" %BAUD19200_SELECTED%>19200</option>
                <option value="38400" %BAUD38400_SELECTED%>38400</option>
                <option value="57600" %BAUD57600_SELECTED%>57600</option>
                <option value="115200" %BAUD115200_SELECTED%>115200</option>
                <option value="230400" %BAUD230400_SELECTED%>230400</option>
                <option value="460800" %BAUD460800_SELECTED%>460800</option>
                <option value="921600" %BAUD921600_SELECTED%>921600</option>
            </select>
            
            <label for="data_bits">Data Bits:</label>
            <select id="data_bits" name="data_bits">
                <option value="5" %DATA5_SELECTED%>5</option>
                <option value="6" %DATA6_SELECTED%>6</option>
                <option value="7" %DATA7_SELECTED%>7</option>
                <option value="8" %DATA8_SELECTED%>8</option>
            </select>
            
            <label for="parity">Parity:</label>
            <select id="parity" name="parity">
                <option value="0" %PARITY_NONE_SELECTED%>None</option>
                <option value="1" %PARITY_ODD_SELECTED%>Odd</option>
                <option value="2" %PARITY_EVEN_SELECTED%>Even</option>
            </select>
            
            <label for="stop_bits">Stop Bits:</label>
            <select id="stop_bits" name="stop_bits">
                <option value="1" %STOP1_SELECTED%>1</option>
                <option value="2" %STOP2_SELECTED%>2</option>
            </select>
            
            <label for="slave_id">Modbus Slave ID (1-247):</label>
            <input type="number" id="slave_id" name="slave_id" min="1" max="247" value="%SLAVE_ID%" required>
            
            <label for="com_port">COM Port:</label>
            <select id="com_port" name="com_port" disabled>
                <option value="COM1">COM1</option>
            </select>
            <button type="submit" class="button">Save Configuration & Restart</button>
        </form>
    </div>
</body>
</html>
)rawliteral";

    // Replace placeholders with current settings
    html.replace("%BAUD1200_SELECTED%", (uart_baud == 1200) ? "selected" : "");
    html.replace("%BAUD2400_SELECTED%", (uart_baud == 2400) ? "selected" : "");
    html.replace("%BAUD4800_SELECTED%", (uart_baud == 4800) ? "selected" : "");
    html.replace("%BAUD9600_SELECTED%", (uart_baud == 9600) ? "selected" : "");
    html.replace("%BAUD19200_SELECTED%", (uart_baud == 19200) ? "selected" : "");
    html.replace("%BAUD38400_SELECTED%", (uart_baud == 38400) ? "selected" : "");
    html.replace("%BAUD57600_SELECTED%", (uart_baud == 57600) ? "selected" : "");
    html.replace("%BAUD115200_SELECTED%", (uart_baud == 115200) ? "selected" : "");
    html.replace("%BAUD230400_SELECTED%", (uart_baud == 230400) ? "selected" : "");
    html.replace("%BAUD460800_SELECTED%", (uart_baud == 460800) ? "selected" : "");
    html.replace("%BAUD921600_SELECTED%", (uart_baud == 921600) ? "selected" : "");

    html.replace("%DATA5_SELECTED%", (uart_data_bits == 5) ? "selected" : "");
    html.replace("%DATA6_SELECTED%", (uart_data_bits == 6) ? "selected" : "");
    html.replace("%DATA7_SELECTED%", (uart_data_bits == 7) ? "selected" : "");
    html.replace("%DATA8_SELECTED%", (uart_data_bits == 8) ? "selected" : "");

    html.replace("%PARITY_NONE_SELECTED%", (uart_parity == 0) ? "selected" : "");
    html.replace("%PARITY_ODD_SELECTED%", (uart_parity == 1) ? "selected" : "");
    html.replace("%PARITY_EVEN_SELECTED%", (uart_parity == 2) ? "selected" : "");

    html.replace("%STOP1_SELECTED%", (uart_stop_bits == 1) ? "selected" : "");
    html.replace("%STOP2_SELECTED%", (uart_stop_bits == 2) ? "selected" : "");
    
    html.replace("%SLAVE_ID%", String(modbus_slave_id));

    server.send(200, "text/html", html);
}

