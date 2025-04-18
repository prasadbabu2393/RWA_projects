


#include <HardwareSerial.h>
#include <WiFi.h>
#include <Preferences.h>  // Include Preferences library

Preferences preferences;  // Create a Preferences object
HardwareSerial mySerial(2); // Use UART2 (GPIO16 = RX, GPIO17 = TX)

char* ssid_h = "LCD_COUNTER_#01";  //esp32  hotpot name 
char* password_h = "12345678";     // password with 8 characters

//global variables
WiFiServer server(80);
IPAddress flash_ip;
String header, wifi, inputmode = "input1";    
String network_1, password_1, client_ip, ip;
String ssid, password;
String receivedData= ""; //string varible to store 
uint8_t retry_count = 0; //wifi retry count
int pulseCount = 0; // counting the square wave
int debounce_delay = 300;
uint8_t r1 = 0, r2 = 0, prev_r1 =0; //variables to send relay status to arduino 
bool MODE;

int input = 18, relay = 26;
int switchState = 0;     // Array to store the states of the switches
int lastSwitchState = 0;  // Array to store the last states for comparison
int switchCount = 0;    // Array to store the toggle counts for each switch
String pulse_state = "HIGH", currentstate ="LOW";  // Default state

const int switchPins[8] = {18, 15, 14, 23};  // Pins for the four switches
const int relay1 = 26 , relay2 = 25;

// Set your Static IP addressa
//IPAddress local_IP(192, 168, 0, 114);  // 192.168.0.114
int a = 192, b = 168, c = 0, d = 114;
// Set your Gateway IP address
IPAddress gateway(192, 168, 0, 1);  ///1 -- 3 REPLACED
 IPAddress subnet(255, 255, 255, 0);
// IPAddress primaryDNS(8, 8, 8, 8);   //optional
// IPAddress secondaryDNS(8, 8, 4, 4); //optional


unsigned long currentTime = millis(); //
unsigned long previousTime = 0;
const long timeoutTime = 2000;

//////// string IP address to int 4 variables seperation (.)
void convert(String address)
{
  // Split the IP address string by dots
  int d1 = address.indexOf('.');
  int d2 = address.indexOf('.', d1 + 1);
  int d3 = address.indexOf('.', d2 + 1);

  // Convert each part into an integer
  a = address.substring(0, d1).toInt();
  b = address.substring(d1 + 1, d2).toInt();
  c = address.substring(d2 + 1, d3).toInt(); 32, 33,
  d = address.substring(d3 + 1).toInt();

  // Initialize local_IP with the integers from flash_ip
  //local_IP(a, b, c, d);
}



void relay_fun()
{
  if (pulse_state == "HIGH")  //selected pulse_state is HIGH
  {
    if (digitalRead(input) == HIGH) //reading the input pin high/low
    {
      if (digitalRead(relay) != HIGH)
      {
        r1 = 10;                    
        digitalWrite(relay, HIGH);  //turning ON the relay
        Serial.println("relay ON");
      }
    }
    else  // When input is LOW
    {
      if (digitalRead(relay) != LOW)
      {
        r1 = 20;                     
        digitalWrite(relay, LOW);   //turning OFF the relay
        Serial.println("relay OFF");
      }
    }
  }
  else // When pulse_state is LOW
  {
    if (digitalRead(input) == LOW)  // Check button state
    {
      if (digitalRead(relay) != HIGH)
      {
        r1 = 10;                    
        digitalWrite(relay, HIGH); 
        Serial.println("relay ON (LOW state)");
      }
    }
    else // When IO18 is HIGH
    {
      if (digitalRead(relay) != LOW)
      {
        r1 = 20;                     
        digitalWrite(relay, LOW);   
        Serial.println("relay OFF (LOW state)");
      }
    }
  }
    if(r1 != prev_r1) { //if status changed 
        sendData(pulseCount, r1, r2); //sending status to arduino
  }
  prev_r1 = r1; //storing  the status of relay
}

//function for counting switchstates
void switch_count()
{
    switchState = digitalRead(input);
    //from LOW to HIGH
    if (pulse_state == "HIGH")   
    {
      if(switchState == HIGH && lastSwitchState == LOW)
      {     
          pulseCount++;
          preferences.putInt("pulseCount", pulseCount);
          Serial.print("Pulse Count: ");
          Serial.println(pulseCount);
          sendData(pulseCount, r1, r2);
      }
    } 
    else     
    { //from HIGH to LOW
      if ( switchState == LOW && lastSwitchState == HIGH) 
      {
          pulseCount++;
          preferences.putInt("pulseCount", pulseCount);
          Serial.print("Pulse Count: ");
          Serial.println(pulseCount);
          sendData(pulseCount, r1, r2);
      }
    }
     // Update last switch state
     lastSwitchState = switchState;
     delay(debounce_delay); //debounce delay
}

//function to replace special chars in string of wifi or password etc
String urlDecode(String input) {
              input.replace("%20", " ");
              input.replace("%21", "!");
              input.replace("%22", "\"");
              input.replace("%23", "#");
              input.replace("%24", "$");
              input.replace("%25", "%");
              input.replace("%26", "&");
              input.replace("%27", "'");
              input.replace("%28", "(");
              input.replace("%29", ")");
              input.replace("%2A", "*");
              input.replace("%2B", "+");
              input.replace("%2C", ",");
              input.replace("%2D", "-");
              input.replace("%2E", ".");
              input.replace("%2F", "/");
              input.replace("%40", "@"); 
              // Add more replacements as needed
              return input;
              }   

//function for sending to count, r1, r2 values to arduino
void sendData(int pulsecount, int r1, int r2) 
{
    mySerial.print("P:"); mySerial.print(pulsecount);
    mySerial.print(" R1:"); mySerial.print(r1);
    mySerial.print(" R2:"); mySerial.println(r2); // Newline to mark end of transmission
}

void server_code()
{
WiFiClient client = server.available(); 
if(client)
{
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client.");
    String currentLine = ""; 
    while(client.connected() && currentTime - previousTime <= timeoutTime)
    {
        currentTime = millis();
        if(client.available())
        {
            char c = client.read();
            header += c;
            if(c == '\n')
            {
                if(currentLine.length() == 0)
                {
                    client.println("HTTP/1.1 200 OK");
                    client.println("Content-type:text/html");
                    client.println("Connection: close");
                    client.println();         
                    
                    if(header.indexOf("GET /login") >= 0)
                    {
                        Serial.println("Logging in");

                        // Start HTML
                        client.println("<!DOCTYPE html>");
                        client.println("<html>");
                        client.println("<head>");
                        client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");

                        // CSS Styles
                        client.println("<style>");
                        client.println("body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #74ABE2, #5563DE); color: #fff; text-align: center; padding: 20px; }");
                        client.println(".container { max-width: 400px; background: #ADD8E6;; padding: 20px; margin: 50px auto; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0,0,0,0.3); color: #333; }");
                        client.println("h2 { color: #333; }");
                        client.println("label { display: block; font-weight: bold; margin-top: 10px; text-align: left; }");

                        // Input Fields Styling
                        client.println(".input-group { position: relative; width: 100%; }");
                        client.println(".input-group input { width: 100%; padding-right: 40px; box-sizing: border-box; }");
                        client.println(".eye-icon { position: absolute; right: 10px; top: 50%; transform: translateY(-50%); cursor: pointer; font-size: 18px; color: #555; }");

                        client.println("input, select { width: 100%; padding: 8px; margin: 5px 0 15px; border: 1px solid #ccc; border-radius: 5px; font-size: 16px; }");
                        client.println("input[type=submit] { background-color: #007BFF; color: white; cursor: pointer; border: none; padding: 10px; font-size: 16px; border-radius: 5px; width: 100%; }");
                        client.println("input[type=submit]:hover { background-color: #0056b3; }");
                        client.println("</style>");
                        client.println("</head>");

                        // Start Body
                        client.println("<body>");
                        client.println("<div class='container'>");
                        client.println("<h2>WiFi & Pulse Configuration</h2>");
                        client.println("<form action=\"/action_page.php\">");

                        // WiFi SSID Input
                        client.println("<label for='ssid'>WiFi SSID:</label>");
                        client.println("<input type='text' id='ssid' name='ssid' placeholder='Enter WiFi Name'>");

                        // WiFi Password Input with Eye Icon
                        client.println("<label for='password'>WiFi Password:</label>");
                        client.println("<div class='input-group'>");
                        client.println("<input type='password' id='password' name='password' placeholder='Enter Password'>");
                        client.println("<span class='eye-icon' onclick='togglePassword()'>&#128065;</span>");
                        client.println("</div>");

                        // Static IP Input
                        client.println("<label for='static_IP'>Static IP:</label>");
                        client.println("<input type='text' id='static_IP' name='static_IP' placeholder='e.g., 192.168.0.100'>");

                        // Pulse Selection Dropdown
                        client.println("<label for='pulse'>Pulse State:</label>");
                        client.println("<select id='pulse' name='pulse'>");
                        client.println(pulse_state == "HIGH" ? "<option value='HIGH' selected>HIGH</option>" : "<option value='HIGH'>HIGH</option>");
                        client.println(pulse_state == "LOW" ? "<option value='LOW' selected>LOW</option>" : "<option value='LOW'>LOW</option>");
                        client.println("</select>");

                        // Submit Button
                        client.println("<input type='submit' value='Save Settings'>");
                        client.println("</form>");
                        client.println("</div>");

                        // JavaScript for Password Toggle
                        client.println("<script>");
                        client.println("function togglePassword() {");
                        client.println("  var pwd = document.getElementById('password');");
                        client.println("  if (pwd.type === 'password') {");
                        client.println("    pwd.type = 'text';");
                        client.println("  } else {");
                        client.println("    pwd.type = 'password';");
                        client.println("  }");
                        client.println("}");
                        client.println("</script>");

                        client.println("</body></html>");
                        client.println();
                    }
            else if(header.indexOf("GET /action_page.php") >= 0) 
            {
              int networkStart = header.indexOf("ssid=") + 5;
              int networkEnd = header.indexOf("&", networkStart);
              network_1 = header.substring(networkStart, networkEnd);
              network_1 = urlDecode(network_1);  // Decode SSID
              
              int passwordStart =header.indexOf("password=") + 9;
              int passwordEnd = header.indexOf("&", passwordStart);
              password_1 = header.substring(passwordStart, passwordEnd);
              password_1 = urlDecode(password_1);  // Decode password
              
              int ipStart = header.indexOf("static_IP=") + 10;
              int ipEnd = header.indexOf("&", ipStart);
              client_ip = header.substring(ipStart, ipEnd);

              int pulseStart = header.indexOf("pulse=") + 6;
              int pulseEnd = header.indexOf(" ", pulseStart);
              if (pulseEnd == -1) pulseEnd = header.length();
              pulse_state = header.substring(pulseStart, pulseEnd);


              // Break out of the while loop
              Serial.println("network");
              Serial.println(network_1);
              Serial.println("password");
              Serial.println(password_1);
              Serial.println("client_iP");
              Serial.println(client_ip);
              Serial.println("Pulse State Selected: " + pulse_state);
                    if(network_1 != "") {
                      preferences.putString("ssid", network_1); 
                    }
                    if(password_1 != "") {
                     preferences.putString("password", password_1);
                    }
                    if(client_ip != "")
                    {
                      preferences.putString("staticIP", client_ip); // static_IP storing into flash
                    }
                    if(pulse_state != "")
                    {                     
                      preferences.putString("pulseState", pulse_state);
                    }
                    preferences.putString("flash", "done");
                    delay(100); 
                    Serial.println("credientials Saved");
                    ESP.restart();
            }
//             else if(header.indexOf("GET /ssid") >= 0)
// {
//     Serial.println("SSID Page");

//     // Start HTML
//     client.println("<!DOCTYPE html>");
//     client.println("<html>");
//     client.println("<head>");
//     client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
//     client.println("<style>");
//     client.println("body { font-family: Arial, sans-serif; text-align: center; padding: 20px; }");
//     client.println(".container { max-width: 400px; padding: 20px; margin: 50px auto; border: 1px solid #ccc; border-radius: 10px; }");
//     client.println("input, select { width: 100%; padding: 8px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; }");
//     client.println("input[type=submit] { background-color: #007BFF; color: white; cursor: pointer; border: none; padding: 10px; border-radius: 5px; }");
//     client.println("</style>");
//     client.println("</head>");

//     // Start Body
//     client.println("<body>");
//     client.println("<div class='container'>");
//     client.println("<h2>Enter SSID Name</h2>");
//     client.println("<form action='/submit_ssid'>");
//     client.println("<input type='text' name='ssid_text' placeholder='Enter SSID'><br>");

//     // Dropdown for Input Selection
//     client.println("<label for='input_select'>Select Input:</label>");
//     client.println("<select name='input_select'>");
//     client.println("<option value='input1'>Input 1</option>");
//     client.println("<option value='input2'>Input 2</option>");
//     client.println("</select><br>");

//     client.println("<input type='submit' value='Submit'>");
//     client.println("</form>");
//     client.println("</div>");
//     client.println("</body></html>");
//     client.println();
// }
// else if(header.indexOf("GET /submit_ssid?") >= 0) 
// {
//     int ssidStart = header.indexOf("ssid_text=") + 10;
//     int ssidEnd = header.indexOf("&", ssidStart);
//     if (ssidEnd == -1) ssidEnd = header.length();
//     String ssidEntered = header.substring(ssidStart, ssidEnd);
//     ssidEntered = urlDecode(ssidEntered);

//     int inputStart = header.indexOf("input_select=") + 13;
//     int inputEnd = header.indexOf(" ", inputStart);
//     if (inputEnd == -1) inputEnd = header.length();
//     String inputSelected = header.substring(inputStart, inputEnd);
//     inputSelected = urlDecode(inputSelected);

//     Serial.println("Entered SSID:");
//     Serial.println(ssidEntered);
//     Serial.println("Selected Input:");
//     Serial.println(inputSelected);

//     if(ssidEntered != "" || inputSelected != "")
//     {
//       if(ssidEntered != "") {
//         preferences.putString("wifi", ssidEntered);
//       }
//       if(inputSelected != "") {
//         preferences.putString("input", inputSelected);
//       }
//         Serial.println("SSID and Input selection saved");
//         ESP.restart();
//     }
// }
else if(header.indexOf("GET /ssid") >= 0)
{
    Serial.println("SSID Page");

    // Start HTML
    client.println("<!DOCTYPE html>");
    client.println("<html>");
    client.println("<head>");
    client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
    client.println("<style>");
    client.println("body { font-family: Arial, sans-serif; text-align: center; padding: 20px; }");
    client.println(".container { max-width: 400px; padding: 20px; margin: 50px auto; border: 1px solid #ccc; border-radius: 10px; }");
    client.println("input, select { width: 100%; padding: 8px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; }");
    client.println("input[type=submit] { background-color: #007BFF; color: white; cursor: pointer; border: none; padding: 10px; border-radius: 5px; }");
    client.println("</style>");
    client.println("</head>");

    // Start Body
    client.println("<body>");
    client.println("<div class='container'>");
    client.println("<h2>Enter SSID, Input Selection & Integer</h2>");
    client.println("<form action='/submit_ssid'>");
    client.println("<input type='text' name='ssid_text' placeholder='Enter SSID'><br>");

    // Dropdown for Input Selection
    client.println("<label for='input_select'>Select Input:</label>");
    client.println("<select name='input_select'>");
    client.println("<option value='input1'>Input 1</option>");
    client.println("<option value='input2'>Input 2</option>");
    client.println("</select><br>");

    // Integer Input Field
    client.println("<label for='int_value'>Enter an Integer:</label>");
    client.println("<input type='number' name='int_value' placeholder='Enter a number'><br>");

    client.println("<input type='submit' value='Submit'>");
    client.println("</form>");
    client.println("</div>");
    client.println("</body></html>");
    client.println();
}
else if(header.indexOf("GET /submit_ssid?") >= 0) 
{
    // Extract SSID
    int ssidStart = header.indexOf("ssid_text=") + 10;
    int ssidEnd = header.indexOf("&", ssidStart);
    if (ssidEnd == -1) ssidEnd = header.length();
    String ssidEntered = header.substring(ssidStart, ssidEnd);
    ssidEntered = urlDecode(ssidEntered);

    // Extract Input Selection
    int inputStart = header.indexOf("input_select=") + 13;
    int inputEnd = header.indexOf("&", inputStart);
    if (inputEnd == -1) inputEnd = header.length();
    String inputSelected = header.substring(inputStart, inputEnd);
    inputSelected = urlDecode(inputSelected);

    // Extract Integer Value
    int intStart = header.indexOf("int_value=") + 10;
    int intEnd = header.indexOf(" ", intStart);
    if (intEnd == -1) intEnd = header.length();
    String intString = header.substring(intStart, intEnd);
    int intValue = intString.toInt(); // Convert to integer

    Serial.println("Entered SSID:");
    Serial.println(ssidEntered);
    Serial.println("Selected Input:");
    Serial.println(inputSelected);
    Serial.println("Entered Integer:");
    Serial.println(intValue);

    // Store values in flash memory
    if(ssidEntered != "" || inputSelected != "" || intString != "")
    {
      if(ssidEntered != "") {
        preferences.putString("wifi", ssidEntered);
      }
      if(inputSelected != "") {
        preferences.putString("input", inputSelected);
      }
      if(intString != "") {
        preferences.putInt("stored_integer", intValue);
      }

      Serial.println("SSID, Input selection, and Integer saved");
      delay(1000);
      ESP.restart();
    }
}

          }
          else
          {
            currentLine = "";
          }
        }
        else if (c != '\r')
        {
          currentLine += c;
        }
      }
    }
    header = "";
    client.stop();
  }

}

void setup() 
{
    Serial.begin(115200); // Serial Monitor Debugging
    mySerial.begin(9600, SERIAL_8N1, 16, 17); // RX = GPIO16, TX = GPIO17
   
    // Initialize Preferences and read stored switch states and counts
    preferences.begin("storage", false);  // same namespace for switchs and network, password
  
    String flash_flag = preferences.getString("flash", "");  //  reading a flag
    pulseCount = preferences.getInt("pulseCount", 0);
    pulse_state = preferences.getString("pulseState", pulse_state);
    wifi = preferences.getString("wifi", ssid_h);
    debounce_delay = preferences.getInt("stored_integer", debounce_delay);
    inputmode = preferences.getString("input", inputmode);

    if(inputmode == "input1")
    {
      input = 18; //selecting input1
      relay = 26; //selecting relay1
      pinMode(25, OUTPUT); //due some pins may HIGH in default 
      digitalWrite(relay, LOW);
    }
    else 
    {
       input = 5; //selecting input2
       relay = 25; //selecting relay2
       pinMode(26, OUTPUT); //due some pins may HIGH in default 
       digitalWrite(relay, LOW);
    }

      pinMode(input, INPUT);
      pinMode(relay, OUTPUT); 
      digitalWrite(relay,LOW);
      r1 = 20, r2 = 20;

    if(pulse_state == "HIGH")
    {
      lastSwitchState = 1;  // Array to store the last states for comparison
    } else {
      lastSwitchState = 0;  // Array to store the last states for comparison
    }

     Serial.println("pulse_state");
     Serial.println(pulse_state);
     Serial.println("wifi"); 
     Serial.println(wifi);
          Serial.println("debounce_delay");
     Serial.println(debounce_delay);

   if(flash_flag != "")
   {
     ssid = preferences.getString("ssid", "");
     password = preferences.getString("password", "");
     ip = preferences.getString("staticIP", "");            //IP address

      if(ip != "")
       {
        Serial.println("ip");
        Serial.println(ip);
        convert(ip);
        Serial.print("Initialized IP Address: ");
        IPAddress local_IP(a, b, c, d);   //IPAddress local_IP(a, b, c, d); 
        Serial.println(local_IP); 
       }
    
     IPAddress local_IP(a, b, c, d); 
     Serial.println(ssid);
     Serial.println(password);
     if(ssid != "" && password != "")
     { // Configures static IP address
        if (!WiFi.config(local_IP, gateway, subnet)) 
        {      //, primaryDNS, secondaryDNS
          Serial.println("STA Failed to configure");
        }
         WiFi.begin(ssid.c_str(), password.c_str());

         unsigned long connectionsetup = millis(); //millis started 

         while(WiFi.status() != WL_CONNECTED)
         {    
           while (mySerial.available())
           {
              char c = mySerial.read();
              receivedData += c;
              if (c == '\n') { // End of transmission
                receivedData.trim();
                Serial.println(receivedData);
              }
           }
           if(receivedData == "wifi")
           {
             receivedData = ""; // Clear buffer
             MODE = 0;
             preferences.putString("flash", ""); 
             preferences.putString("ssid", "");
             preferences.putString("password", "");
             preferences.putString("staticIP", "");
             ESP.restart();
           } 
           else if(receivedData == "reset")
           {
              receivedData = ""; // Clear buffer
              pulseCount = 0;
              preferences.putInt("pulseCount", pulseCount);
              ESP.restart();
           }
           else
           {
           }
            switch_count();
            relay_fun();// relay on/off function
            server_code();//function call for wifi server code 

            if (millis() - connectionsetup > 50000) 
            {
               MODE = 0;
               preferences.putString("flash", ""); 
               preferences.putString("ssid", "");
               preferences.putString("password", "");
               preferences.putString("staticIP", "");
               ESP.restart();
            } 
          }

        if (WiFi.status() == WL_CONNECTED) 
        {
           MODE = 1;
           Serial.println("connected");
           Serial.println(ssid);
           Serial.println("IP address: ");
           Serial.println(WiFi.localIP());
           server.begin();
         }
    }
    else
    {
        /////////////////////     set ap
        MODE = 0;
       Serial.print("Setting AP (Access Point)…");
        WiFi.softAP(wifi, password_h);
       IPAddress IP = WiFi.softAPIP();
       Serial.print("AP IP address: ");
       Serial.println(IP); 
       server.begin();
    }
  }
  else
  {
    /////////////////////     set ap
      MODE = 0;
      Serial.println("No values saved for ssid or password");
      Serial.print("Setting AP (Access Point)…");
      WiFi.softAP(wifi, password_h);
      IPAddress IP = WiFi.softAPIP();
      Serial.print("AP IP address: ");
      Serial.println(IP); 
      server.begin();
  }
  sendData(pulseCount, r1, r2);
}



void loop() 
{ 
   while (mySerial.available())
   { //receiving data from arduino 
    char c = mySerial.read();
    receivedData += c;
    if (c == '\n') { // End of transmission
     receivedData.trim();
      Serial.println(receivedData);
    }
   }

    if(receivedData == "wifi")
    {
       receivedData = ""; // Clear buffer
       MODE = 0;
       preferences.putString("flash", ""); 
       preferences.putString("ssid", "");
       preferences.putString("password", "");
       preferences.putString("staticIP", "");
       ESP.restart();
    } 
     if(receivedData == "reset")
    {
      receivedData = ""; // Clear buffer
      pulseCount = 0;
      preferences.putInt("pulseCount", pulseCount);
      ESP.restart();
    }

 
   // retry for wifi in wifi mode
  if (MODE == 1) 
  {
    unsigned long loopStartTime = millis();  // Store loop start time
    while (WiFi.status() != WL_CONNECTED) 
    {
      Serial.println("loop");
      if ((millis() - loopStartTime) > 50000) 
      {
       Serial.println("50sec");
       MODE = 0;
       preferences.putString("flash", ""); 
       preferences.putString("ssid", "");
       preferences.putString("password", "");
       preferences.putString("staticIP", "");
       ESP.restart();
      }
           relay_fun();// relay on/off function
           switch_count();
           server_code();//function call for wifi server code 

           while (mySerial.available())
           {
              char c = mySerial.read();
              receivedData += c;
              if (c == '\n') { // End of transmission
                receivedData.trim();
                Serial.println(receivedData);
              }
           }
           if(receivedData == "wifi")
           {
             receivedData = ""; // Clear buffer
             MODE = 0;
             preferences.putString("flash", ""); 
             preferences.putString("ssid", "");
             preferences.putString("password", "");
             preferences.putString("staticIP", "");
             ESP.restart();
           } 
           else if(receivedData == "reset")
           {
              receivedData = ""; // Clear buffer
              pulseCount = 0;
              preferences.putInt("pulseCount", pulseCount);
              ESP.restart();
           }
           else
           {
           }
    }
  }
  relay_fun();// relay on/off function
  switch_count();
  server_code();//function call for wifi server code 
}


// #include <HardwareSerial.h>
// #include <WiFi.h>
// #include <Preferences.h>  // Include Preferences library

// Preferences preferences;  // Create a Preferences object
// HardwareSerial mySerial(2); // Use UART2 (GPIO16 = RX, GPIO17 = TX)

// char* ssid_h = "LCD_COUNTER_#01";  //esp32  hotpot name 
// char* password_h = "12345678";     // password with 8 characters

// //global variables
// WiFiServer server(80);
// IPAddress flash_ip;
// String header, wifi;    
// String network_1, password_1, client_ip, ip;
// String ssid, password;
// String receivedData= ""; //string varible to store 
// uint8_t retry_count = 0; //wifi retry count
// int pulseCount = 0; // counting the square wave
// uint8_t r1 = 0, r2 = 0; //variables to send relay status to arduino 
// bool MODE;

// int switchState[4] = {0, 0, 0, 0};     // Array to store the states of the switches
// int lastSwitchState[4] = {0, 0, 0, 0};  // Array to store the last states for comparison
// int switchCount[4] = {0, 0, 0, 0};    // Array to store the toggle counts for each switch
// String pulse_state = "HIGH", currentstate ="LOW";  // Default state

// const int switchPins[8] = {18, 15, 14, 23};  // Pins for the four switches
// const int relay1 = 26 , relay2 = 25;

// // Set your Static IP addressa
// //IPAddress local_IP(192, 168, 0, 114);  // 192.168.0.114
// int a = 192, b = 168, c = 0, d = 114;
// // Set your Gateway IP address
// IPAddress gateway(192, 168, 0, 1);  ///1 -- 3 REPLACED
//  IPAddress subnet(255, 255, 255, 0);
// // IPAddress primaryDNS(8, 8, 8, 8);   //optional
// // IPAddress secondaryDNS(8, 8, 4, 4); //optional


// unsigned long currentTime = millis(); //
// unsigned long previousTime = 0;
// const long timeoutTime = 2000;

// //////// string IP address to int 4 variables seperation (.)
// void convert(String address)
// {
//   // Split the IP address string by dots
//   int d1 = address.indexOf('.');
//   int d2 = address.indexOf('.', d1 + 1);
//   int d3 = address.indexOf('.', d2 + 1);

//   // Convert each part into an integer
//   a = address.substring(0, d1).toInt();
//   b = address.substring(d1 + 1, d2).toInt();
//   c = address.substring(d2 + 1, d3).toInt(); 32, 33,
//   d = address.substring(d3 + 1).toInt();

//   // Initialize local_IP with the integers from flash_ip
//   //local_IP(a, b, c, d);
// }



// void relay_fun()
// {
//   if (pulse_state == "HIGH")  
//   {
//     if (digitalRead(18) == HIGH)
//     {
//       if (digitalRead(relay1) != HIGH)
//       {
//         r1 = 10;                    
//         digitalWrite(relay1, HIGH); 
//         Serial.println("relay1 ON");
//       }
//       if (digitalRead(relay2) != HIGH)
//       {
//         r2 = 10;                     
//         digitalWrite(relay2, HIGH); 
//         Serial.println("relay2 ON");
//       }
//     }
//     else  // When IO18 is LOW
//     {
//       if (digitalRead(relay1) != LOW)
//       {
//         r1 = 20;                     
//         digitalWrite(relay1, LOW);   
//         Serial.println("relay1 OFF");
//       }
//       if (digitalRead(relay2) != LOW)
//       {    
//         r2 = 20; 
//         digitalWrite(relay2, LOW); 
//         Serial.println("relay2 OFF");
//       }
//     }
//   }
//   else // When pulse_state is LOW
//   {
//     if (digitalRead(18) == LOW)  // Check button state
//     {
//       if (digitalRead(relay1) != HIGH)
//       {
//         r1 = 10;                    
//         digitalWrite(relay1, HIGH); 
//         Serial.println("relay1 ON (LOW state)");
//       }
//       if (digitalRead(relay2) != HIGH)
//       {
//         r2 = 10;                     
//         digitalWrite(relay2, HIGH); 
//         Serial.println("relay2 ON (LOW state)");
//       }
//     }
//     else // When IO18 is HIGH
//     {
//       if (digitalRead(relay1) != LOW)
//       {
//         r1 = 20;                     
//         digitalWrite(relay1, LOW);   
//         Serial.println("relay1 OFF (LOW state)");
//       }
//       if (digitalRead(relay2) != LOW)
//       {    
//         r2 = 20; 
//         digitalWrite(relay2, LOW); 
//         Serial.println("relay2 OFF (LOW state)");
//       }
//     }
//   }
// }

// void switch_count()
// {
//   // for (int i = 0; i < 4; i++)
//   // { 
//     uint8_t i = 0;
//     switchState[i] = digitalRead(18);

//     // Check for a valid transition and process only if necessary
//     if ((pulse_state == "HIGH" && switchState[i] == HIGH && lastSwitchState[i] == LOW) || 
//         (pulse_state == "LOW" && switchState[i] == LOW && lastSwitchState[i] == HIGH))
//     {
//       Serial.println("State Changed");
//       switchCount[i]++;

//       if(i == 0)
//       {
//         pulseCount++;
//         preferences.putInt("pulseCount", pulseCount);
//         Serial.print("Pulse Count: ");
//         Serial.println(pulseCount);
//         sendData(pulseCount, r1, r2);
//       }

//       // Update last switch state only when a change happens
//       lastSwitchState[i] = switchState[i];
//     }
//  // }
// }

// // //function for counting switchstates
// // void switch_count()
// // {
// //   for (int i = 0; i < 4; i++)
// //   { 
// //     switchState[i] = digitalRead(switchPins[i]);
// //     //from LOW to HIGH
// //     if (pulse_state == "HIGH" && switchState[i] == HIGH && lastSwitchState[i] == LOW) 
// //     {
// //       Serial.println("he");
// //          switchCount[i]++;
// //          if(i == 0)
// //          {
// //           pulseCount++;
// //           preferences.putInt("pulseCount", pulseCount);
// //           Serial.print("Pulse Count: ");
// //           Serial.println(pulseCount);
// //           sendData(pulseCount, r1, r2);
// //          }
// //     } 
// //     //from HIGH to LOW
// //     else if (pulse_state == "LOW" && switchState[i] == LOW && lastSwitchState[i] == HIGH) 
// //     {
// //       Serial.println("henh");
// //          switchCount[i]++;
// //          if(i == 0)
// //          {
// //           pulseCount++;
// //           preferences.putInt("pulseCount", pulseCount);
// //           Serial.print("Pulse Count: ");
// //           Serial.println(pulseCount);
// //           sendData(pulseCount, r1, r2);
// //          }
// //     }
// //     // else{
// //     //   Serial.println("else");
// //     // }
// //      // Update last switch state
// //      lastSwitchState[i] = switchState[i];
// //   }
// // }


// //function to replace special chars in string of wifi or password etc
// String urlDecode(String input) {
//               input.replace("%20", " ");
//               input.replace("%21", "!");
//               input.replace("%22", "\"");
//               input.replace("%23", "#");
//               input.replace("%24", "$");
//               input.replace("%25", "%");
//               input.replace("%26", "&");
//               input.replace("%27", "'");
//               input.replace("%28", "(");
//               input.replace("%29", ")");
//               input.replace("%2A", "*");
//               input.replace("%2B", "+");
//               input.replace("%2C", ",");
//               input.replace("%2D", "-");
//               input.replace("%2E", ".");
//               input.replace("%2F", "/");
//               input.replace("%40", "@"); 
//               // Add more replacements as needed
//               return input;
//               }   

// //function for sending to count, r1, r2 values to arduino
// void sendData(int pulsecount, int r1, int r2) 
// {
//   Serial.println("pulsec");
//   Serial.println(pulsecount);
//     mySerial.print("P:"); mySerial.print(pulsecount);
//     mySerial.print(" R1:"); mySerial.print(r1);
//     mySerial.print(" R2:"); mySerial.println(r2); // Newline to mark end of transmission
// }

// void server_code()
// {
// WiFiClient client = server.available(); 
// if(client)
// {
//     currentTime = millis();
//     previousTime = currentTime;
//     Serial.println("New Client.");
//     String currentLine = ""; 
//     while(client.connected() && currentTime - previousTime <= timeoutTime)
//     {
//         currentTime = millis();
//         if(client.available())
//         {
//             char c = client.read();
//             header += c;
//             if(c == '\n')
//             {
//                 if(currentLine.length() == 0)
//                 {
//                     client.println("HTTP/1.1 200 OK");
//                     client.println("Content-type:text/html");
//                     client.println("Connection: close");
//                     client.println();         
                    
//                     if(header.indexOf("GET /login") >= 0)
//                     {
//                         Serial.println("Logging in");

//                         // Start HTML
//                         client.println("<!DOCTYPE html>");
//                         client.println("<html>");
//                         client.println("<head>");
//                         client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");

//                         // CSS Styles
//                         client.println("<style>");
//                         client.println("body { font-family: Arial, sans-serif; background: linear-gradient(135deg, #74ABE2, #5563DE); color: #fff; text-align: center; padding: 20px; }");
//                         client.println(".container { max-width: 400px; background: #ADD8E6;; padding: 20px; margin: 50px auto; border-radius: 10px; box-shadow: 0px 0px 10px rgba(0,0,0,0.3); color: #333; }");
//                         client.println("h2 { color: #333; }");
//                         client.println("label { display: block; font-weight: bold; margin-top: 10px; text-align: left; }");

//                         // Input Fields Styling
//                         client.println(".input-group { position: relative; width: 100%; }");
//                         client.println(".input-group input { width: 100%; padding-right: 40px; box-sizing: border-box; }");
//                         client.println(".eye-icon { position: absolute; right: 10px; top: 50%; transform: translateY(-50%); cursor: pointer; font-size: 18px; color: #555; }");

//                         client.println("input, select { width: 100%; padding: 8px; margin: 5px 0 15px; border: 1px solid #ccc; border-radius: 5px; font-size: 16px; }");
//                         client.println("input[type=submit] { background-color: #007BFF; color: white; cursor: pointer; border: none; padding: 10px; font-size: 16px; border-radius: 5px; width: 100%; }");
//                         client.println("input[type=submit]:hover { background-color: #0056b3; }");
//                         client.println("</style>");
//                         client.println("</head>");

//                         // Start Body
//                         client.println("<body>");
//                         client.println("<div class='container'>");
//                         client.println("<h2>WiFi & Pulse Configuration</h2>");
//                         client.println("<form action=\"/action_page.php\">");

//                         // WiFi SSID Input
//                         client.println("<label for='ssid'>WiFi SSID:</label>");
//                         client.println("<input type='text' id='ssid' name='ssid' placeholder='Enter WiFi Name'>");

//                         // WiFi Password Input with Eye Icon
//                         client.println("<label for='password'>WiFi Password:</label>");
//                         client.println("<div class='input-group'>");
//                         client.println("<input type='password' id='password' name='password' placeholder='Enter Password'>");
//                         client.println("<span class='eye-icon' onclick='togglePassword()'>&#128065;</span>");
//                         client.println("</div>");

//                         // Static IP Input
//                         client.println("<label for='static_IP'>Static IP:</label>");
//                         client.println("<input type='text' id='static_IP' name='static_IP' placeholder='e.g., 192.168.0.100'>");

//                         // Pulse Selection Dropdown
//                         client.println("<label for='pulse'>Pulse State:</label>");
//                         client.println("<select id='pulse' name='pulse'>");
//                         client.println(pulse_state == "HIGH" ? "<option value='HIGH' selected>HIGH</option>" : "<option value='HIGH'>HIGH</option>");
//                         client.println(pulse_state == "LOW" ? "<option value='LOW' selected>LOW</option>" : "<option value='LOW'>LOW</option>");
//                         client.println("</select>");

//                         // Submit Button
//                         client.println("<input type='submit' value='Save Settings'>");
//                         client.println("</form>");
//                         client.println("</div>");

//                         // JavaScript for Password Toggle
//                         client.println("<script>");
//                         client.println("function togglePassword() {");
//                         client.println("  var pwd = document.getElementById('password');");
//                         client.println("  if (pwd.type === 'password') {");
//                         client.println("    pwd.type = 'text';");
//                         client.println("  } else {");
//                         client.println("    pwd.type = 'password';");
//                         client.println("  }");
//                         client.println("}");
//                         client.println("</script>");

//                         client.println("</body></html>");
//                         client.println();
//                     }
//             else if(header.indexOf("GET /action_page.php") >= 0) 
//             {
//               int networkStart = header.indexOf("ssid=") + 5;
//               int networkEnd = header.indexOf("&", networkStart);
//               network_1 = header.substring(networkStart, networkEnd);
//               network_1 = urlDecode(network_1);  // Decode SSID
              
//               int passwordStart =header.indexOf("password=") + 9;
//               int passwordEnd = header.indexOf("&", passwordStart);
//               password_1 = header.substring(passwordStart, passwordEnd);
//               password_1 = urlDecode(password_1);  // Decode password
              
//               int ipStart = header.indexOf("static_IP=") + 10;
//               int ipEnd = header.indexOf("&", ipStart);
//               client_ip = header.substring(ipStart, ipEnd);

//               int pulseStart = header.indexOf("pulse=") + 6;
//               int pulseEnd = header.indexOf(" ", pulseStart);
//               if (pulseEnd == -1) pulseEnd = header.length();
//               pulse_state = header.substring(pulseStart, pulseEnd);


//               // Break out of the while loop
//               Serial.println("network");
//               Serial.println(network_1);
//               Serial.println("password");
//               Serial.println(password_1);
//               Serial.println("client_iP");
//               Serial.println(client_ip);
//               Serial.println("Pulse State Selected: " + pulse_state);
//                     if(network_1 != "") {
//                       preferences.putString("ssid", network_1); 
//                     }
//                     if(password_1 != "") {
//                      preferences.putString("password", password_1);
//                     }
//                     if(client_ip != "")
//                     {
//                       preferences.putString("staticIP", client_ip); // static_IP storing into flash
//                     }
//                     if(pulse_state != "")
//                     {                     
//                       preferences.putString("pulseState", pulse_state);
//                     }
//                     preferences.putString("flash", "done");
//                     delay(100); 
//                     Serial.println("credientials Saved");
//                     ESP.restart();
//             }
//             else if(header.indexOf("GET /ssid") >= 0)
//                     {
//                         Serial.println("SSID Page");

//                         // Start HTML
//                         client.println("<!DOCTYPE html>");
//                         client.println("<html>");
//                         client.println("<head>");
//                         client.println("<meta name='viewport' content='width=device-width, initial-scale=1.0'>");
//                         client.println("<style>");
//                         client.println("body { font-family: Arial, sans-serif; text-align: center; padding: 20px; }");
//                         client.println(".container { max-width: 400px; padding: 20px; margin: 50px auto; border: 1px solid #ccc; border-radius: 10px; }");
//                         client.println("input { width: 100%; padding: 8px; margin: 10px 0; border: 1px solid #ccc; border-radius: 5px; }");
//                         client.println("input[type=submit] { background-color: #007BFF; color: white; cursor: pointer; border: none; padding: 10px; border-radius: 5px; }");
//                         client.println("</style>");
//                         client.println("</head>");
                        
//                         // Start Body
//                         client.println("<body>");
//                         client.println("<div class='container'>");
//                         client.println("<h2>Enter SSID Name</h2>");
//                         client.println("<form action='/submit_ssid'>");
//                         client.println("<input type='text' name='ssid_text' placeholder='Enter SSID'><br>");
//                         client.println("<input type='submit' value='Submit'>");
//                         client.println("</form>");
//                         client.println("</div>");
//                         client.println("</body></html>");
//                         client.println();
//                     }
//                     else if(header.indexOf("GET /submit_ssid?") >= 0) 
//                     {
//                         int ssidStart = header.indexOf("ssid_text=") + 10;
//                         int ssidEnd = header.indexOf(" ", ssidStart);
//                         if (ssidEnd == -1) ssidEnd = header.length();
//                         String ssidEntered = header.substring(ssidStart, ssidEnd);
//                         ssidEntered = urlDecode(ssidEntered);

//                         Serial.println("Entered SSID:");
//                         Serial.println(ssidEntered);
//                         if(ssidEntered != "")
//                         {
//                           preferences.putString("wifi", ssidEntered);
//                           Serial.println("ssid saved");
//                           ESP.restart();
//                         }
//                     }
//           }
//           else
//           {
//             currentLine = "";
//           }
//         }
//         else if (c != '\r')
//         {
//           currentLine += c;
//         }
//       }
//     }
//     header = "";
//     client.stop();
//   }

// }

// void setup() 
// {
//     Serial.begin(115200); // Serial Monitor Debugging
//     mySerial.begin(9600, SERIAL_8N1, 16, 17); // RX = GPIO16, TX = GPIO17

//     // Initialize the switch pins as inputs
//     for (int i = 0; i < 4; i++) {
//        pinMode(switchPins[i], INPUT);  // Using INPUT_PULLUP to avoid needing external resistors
//     }

//     pinMode(relay1, OUTPUT); //
//     pinMode(relay2, OUTPUT); // 
//     r1 = 20, r2 = 20;
//     digitalWrite(relay1,LOW);
//     digitalWrite(relay2,LOW);
    
//     // Initialize Preferences and read stored switch states and counts
//     preferences.begin("storage", false);  // same namespace for switchs and network, password
  
//     String flash_flag = preferences.getString("flash", "");  //  reading a flag
//     pulseCount = preferences.getInt("pulseCount", 0);
//     pulse_state = preferences.getString("pulseState", pulse_state);
//     wifi = preferences.getString("wifi", ssid_h);
//     //  Serial.println("pulse_state");
//     //  Serial.println(pulse_state);
//      Serial.println("wifi");
//      Serial.println(wifi);

//    if(flash_flag != "")
//    {
//      ssid = preferences.getString("ssid", "");
//      password = preferences.getString("password", "");
//      ip = preferences.getString("staticIP", "");            //IP address

//       if(ip != "")
//        {
//         Serial.println("ip");
//         Serial.println(ip);
//         convert(ip);
//         Serial.print("Initialized IP Address: ");
//         IPAddress local_IP(a, b, c, d);   //IPAddress local_IP(a, b, c, d); 
//         Serial.println(local_IP); 
//        }
    
//      IPAddress local_IP(a, b, c, d); 
//      Serial.println(ssid);
//      Serial.println(password);
//      if(ssid != "" && password != "")
//      { // Configures static IP address
//         if (!WiFi.config(local_IP, gateway, subnet)) 
//         {      //, primaryDNS, secondaryDNS
//           Serial.println("STA Failed to configure");
//         }
//          WiFi.begin(ssid.c_str(), password.c_str());
//          while(WiFi.status() != WL_CONNECTED)
//          {
//            Serial.println("connecting....");
//            delay(1000);     
//            switch_count();
//            while (mySerial.available())
//            {
//               char c = mySerial.read();
//               receivedData += c;
//               if (c == '\n') { // End of transmission
//                 receivedData.trim();
//                 Serial.println(receivedData);
//               }
//            }
//            if(receivedData == "wifi")
//            {
//              receivedData = ""; // Clear buffer
//              MODE = 0;
//              preferences.putString("flash", ""); 
//              preferences.putString("ssid", "");
//              preferences.putString("password", "");
//              preferences.putString("staticIP", "");
//              ESP.restart();
//            } 
//            else if(receivedData == "reset")
//            {
//               receivedData = ""; // Clear buffer
//               switchCount[0] = 0;
//               pulseCount = 0;
//               preferences.putInt("pulseCount", pulseCount);
//            }
//            else
//            {
//            }
//             retry_count++;
//             if (retry_count > 50) 
//             {
//                MODE = 0;
//                preferences.putString("flash", ""); 
//                preferences.putString("ssid", "");
//                preferences.putString("password", "");
//                preferences.putString("staticIP", "");
//                ESP.restart();
//             } 
//           }

//         if (WiFi.status() == WL_CONNECTED) 
//         {
//            MODE = 1;
//            Serial.println("connected");
//            Serial.println(ssid);
//            Serial.println("IP address: ");
//            Serial.println(WiFi.localIP());
//            server.begin();
//          }
//     }
//     else
//     {
//         /////////////////////     set ap
//         MODE = 0;
//        Serial.print("Setting AP (Access Point)…");
//         WiFi.softAP(wifi, password_h);
//        IPAddress IP = WiFi.softAPIP();
//        Serial.print("AP IP address: ");
//        Serial.println(IP); 
//        server.begin();
//     }
//   }
//   else
//   {
//     /////////////////////     set ap
//       MODE = 0;
//       Serial.println("No values saved for ssid or password");
//       Serial.print("Setting AP (Access Point)…");
//       WiFi.softAP(wifi, password_h);
//       IPAddress IP = WiFi.softAPIP();
//       Serial.print("AP IP address: ");
//       Serial.println(IP); 
//       server.begin();
//   }
// }



// void loop() 
// {
//    relay_fun();// relay on/off function
//    switch_count();
   
//    while (mySerial.available())
//    { //receiving data from arduino 
//     char c = mySerial.read();
//     receivedData += c;
//     if (c == '\n') { // End of transmission
//      receivedData.trim();
//       Serial.println(receivedData);
//     }
//    }

//     if(receivedData == "wifi")
//     {
//        receivedData = ""; // Clear buffer
//        MODE = 0;
//        preferences.putString("flash", ""); 
//        preferences.putString("ssid", "");
//        preferences.putString("password", "");
//        preferences.putString("staticIP", "");
//        ESP.restart();
//     } 
//     else if(receivedData == "reset")
//     {
//       receivedData = ""; // Clear buffer
//       switchCount[0] = 0;
//       pulseCount = 0;
//       preferences.putInt("pulseCount", pulseCount);
//     }
//     else
//     {
//     }
 
//    // retry for wifi in wifi mode
//   if (MODE == 1) 
//   {
//     while (WiFi.status() != WL_CONNECTED) 
//     {
//       Serial.println("connecting...");
//       delay(1000);
//       retry_count++;
//       if (retry_count > 50) 
//       {
//        MODE = 0;
//        preferences.putString("flash", ""); 
//        preferences.putString("ssid", "");
//        preferences.putString("password", "");
//        preferences.putString("staticIP", "");
//        ESP.restart();
//       }
//     }
//   }

//   server_code();//function call for wifi server code 
// }

