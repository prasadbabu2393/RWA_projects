
#include<HardwareSerial.h>
#include <Preferences.h>
#include <Wire.h>
#include <WiFi.h>
// #include <Adafruit_BME280.h>
// #include <Adafruit_MLX90614.h>
#include <HX711_ADC.h>
#if defined(ESP8266)|| defined(ESP32) || defined(AVR)
#include <EEPROM.h>
#endif

// Adafruit_MLX90614 mlx = Adafruit_MLX90614();
Preferences preferences;

#define I2C_SDA 27  // i2c for 1st sensor
#define I2C_SCL 26   
#define SDA_1 33 // i2c for 2nd sensor 27
#define SCL_1 32  //26
#define RXD2 16  //esp32 Rx for stm32
#define TXD2 17  //esp32 Tx for stm32
#define SEALEVELPRESSURE_HPA (1013.25)

//pins:  12 14
const int HX711_dout = 14; //mcu > HX711 dout pin
const int HX711_sck = 12; //mcu > HX711 sck pin
//HX711 constructor:
HX711_ADC LoadCell(HX711_dout, HX711_sck);

const int LVDT_pin = 35;  // Analog pin connected to LVDT output
const int limit_switch = 2;  // Analog pin connected to LVDT output

const int calVal_eepromAdress = 0;
unsigned long t = 0;

// //HardwareSerial mySerial(1);  // Use UART1 (you can use UART2 if you want another hardware UART)
// TwoWire I2CBME = TwoWire(0); //custom pin for i2c
// TwoWire I2Ctwo = TwoWire(1); // //custom pin for i2c
// Adafruit_BME280 bme;  //object for bme sensor header file

String header = "";
WiFiServer server(80);
unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

String ssid_h = "Tribometer_#07";  // hotpot name and password 
const char* password_h = "12345678";    

bool SAMPLE = 0;
float calibrationValue; // calibration value (see example file "Calibration.ino")
float dividing_factor; // calibration value (see example file "Calibration.ino")
float battery_percent;
float LVDT_voltage;       // Variable to store the voltage reading
float cof = 0;
float meter, radius = 48; //diamter of disc of encoder is 96mm
float pulse_per_mm = 1.956;
int32_t number ;
uint8_t RxData[10];
uint8_t a1 = 0;// for serial.available
int brake_count = 0;
String password_1, network_1, pulses, wifi_name;

union FloatToBytes {
  float value;
  uint8_t bytes[4]; //for converting float value into bytes
};
FloatToBytes converter;

// this function for taking count of A,B from sm32 and converting received bytes into integer(+/-)
int32_t fromBytesToSignedInt(uint8_t* byteArray) {
  return (int32_t)(byteArray[3] | 
                   (byteArray[2] << 8) | 
                   (byteArray[1] << 16) | 
                   (byteArray[0] << 24));
}

void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);
  Serial2.begin(19200, SERIAL_8N1, RXD2, TXD2);  // uart_begin
  pinMode(LVDT_pin, INPUT); // analog pin of esp32 for battrey
  pinMode(limit_switch, INPUT_PULLUP); // brake status pin (0, 1)
  
  preferences.begin("memory", false);

  ssid_h = preferences.getString("wifi_name", ssid_h);
  // Serial.println("ssid_h");
  // Serial.println(ssid_h);

  dividing_factor = preferences.getString("password", "4").toFloat();
  // Serial.println("dividing_factor");
  // Serial.println(dividing_factor);
  
  pulse_per_mm = preferences.getString("pulses", "1.956").toFloat();
  // Serial.println("pulse_per_mm");
  // Serial.println(pulse_per_mm);

 // delay(10);
  LoadCell.begin();
  //LoadCell.setReverseOutput(); //uncomment to turn a negative output value to positive
 // calibrationValue = -67598.00; // uncomment this if you want to set the calibration value in the sketch
  calibrationValue =  preferences.getString("ssid", "67286.00").toFloat();// uncomment this if you want to set the calibration value in the sketch
  // Serial.println("calibrationValue");
  // Serial.println(calibrationValue);  //67286.00
  #if defined(ESP8266)|| defined(ESP32)
  //EEPROM.begin(512); // uncomment this if you use ESP8266/ESP32 and want to fetch the calibration value from eeprom
  #endif
  //EEPROM.get(calVal_eepromAdress, calibrationValue); // uncomment this if you want to fetch the calibration value from eeprom

  unsigned long stabilizingtime = 2000; // preciscion right after power-up can be improved by adding a few seconds of stabilizing time
  boolean _tare = true; //set this to false if you don't want tare to be performed in the next step
  LoadCell.start(stabilizingtime, _tare);

  if (LoadCell.getTareTimeoutFlag()) {
    //Serial.println("Timeout, check MCU>HX711 wiring and pin designations");
    while (1);
  }
  else {
    LoadCell.setCalFactor(calibrationValue); // set calibration value (float)
    //Serial.println("Startup is complete");
  }

   /////////// sensors set up
  // I2CBME.begin(I2C_SDA, I2C_SCL, 100000); //starting the first i2c
  // I2Ctwo.begin(SDA_1, SCL_1, 100000); //starting the second i2c
  
  // mlx.begin(0x5A, &I2Ctwo); 

  // bool status;
  // status = bme.begin(0x76, &I2CBME);   //checking the connection
  
 // Serial.print("Setting AP (Access Point)…");
  WiFi.softAP(ssid_h, password_h); //starting the Acess point
  server.begin(); // starting the server
 // Serial.println(WiFi.softAPIP()); //printing setAP IP address

}

void loop() 
{  
   // put your main code here, to run repeatedly:
   server_code(); // to run server in loop 
    static boolean newDataReady = 0;
    const int serialPrintInterval = 0; //increase value to slow down serial print activity

  // check for new data/start next conversion:
  if (LoadCell.update()) newDataReady = true;

  // receive command from serial terminal, send 't' to initiate tare operation:
  if (Serial.available() > 0) 
  {
    char inByte = Serial.read();
    if (inByte == 't') LoadCell.tareNoDelay();
  }

  // check if last tare operation is complete:
  if (LoadCell.getTareStatus() == true) {
    //Serial.println("Tare complete");
  }

   //taking A,B count bytes from stm32 
   if (Serial2.available() >= 4) //change this 4 according to number bytes receiving
   {
       a1 = Serial2.available(); 
       Serial2.readBytes(RxData, a1); // reading bytes
       number = fromBytesToSignedInt(RxData);
        meter = number/pulse_per_mm ;     // disc full rotation mean 600 pulses will come(0.5026 per pulse) //2.933;
   }  
   if(SAMPLE == 1)
   {
     // get smoothed value from the dataset:   
     if (newDataReady) 
     {
       if (millis() > t + serialPrintInterval) 
       {
         float i = LoadCell.getData();
        //  Serial.print("Load_cell output val: ");
        //  Serial.println(i);
         if(i<0)
         {
          i = abs(i); // Convert to absolute value
         }
         //cof = i/4; //weight/5 calculation for cof value in kg's
         cof = i/dividing_factor; //weight/5 calculation for cof value in kg's
        //  Serial.println("cof");
        //  Serial.println(cof);
         newDataReady = 0;
         t = millis();
       }
     }
   } 
}
//tx,rx, io0, io2, gnd, 3v3  //stm32: gnd, dio, clk, 3v3
 ///////////  wifi server code
void server_code()
{
  WiFiClient client = server.available(); 
  if(client)
  {
    currentTime = millis();
    previousTime = currentTime;
  //  Serial.println("New Client.");
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
            if (header.indexOf("GET /data") >= 0) 
            {
                static boolean newDataReady = 0;
                const int serialPrintInterval = 0; //increase value to slow down serial print activity

                // check for new data/start next conversion:
                if (LoadCell.update()) newDataReady = true;

                 // receive command from serial terminal, send 't' to initiate tare operation:
                 if (Serial.available() > 0) 
                 {
                   char inByte = Serial.read();
                   if (inByte == 't') LoadCell.tareNoDelay();
                 }

                 // check if last tare operation is complete:
                 if (LoadCell.getTareStatus() == true) {
                 // Serial.println("Tare complete");
                 }
              client.print("{");
              client.print(cof); //printing cof value calculated by weight`
              client.print(",");
              client.print(meter); //printing the count of AB pulses received from stm32
             // client.print(number); //printing the count of AB pulses received from stm32
              client.print(",");
              client.print(digitalRead(limit_switch)); //printing of status of switch
              client.print(",");
              client.print(brake_count); //printing how times switch pressed
              client.print(",");
              //client.print(mlx.readObjectTempC()); //object temperature in celsius
              client.print(0); //object temperature in celsius
              client.print(",");
              //client.print(bme.readTemperature());// taking temperature value from bme sensor
              client.print(0);// taking temperature value from bme sensor
              client.print(",");
              //client.print(bme.readPressure() / 100.0F); // taking pressure value from bme sensor
              client.print(0); // taking pressure value from bme sensor
              client.print(",");
              //client.print(bme.readAltitude(SEALEVELPRESSURE_HPA));// taking altitude value from bme sensor
              client.print(0);// taking altitude value from bme sensor
              client.print(",");
             // client.print(bme.readHumidity());
              client.print(0);
              client.print(",");
                if(SAMPLE == 1)
                {
                    // get smoothed value from the dataset:   
                   if (newDataReady) 
                   {
                    if (millis() > t + serialPrintInterval) 
                    {
                        client.print(LoadCell.getData());// printing weight from load cell
                    }
                   }
                }
              client.print(",");
              LVDT_voltage = (analogRead(LVDT_pin) / 4096.0) * 3.3;  // Convert to voltage for esp32 (4095)
              client.print(LVDT_voltage);
              client.print("}");
              break;
            }
            // else if(header.indexOf("GET /battery") >= 0)
            // { 
            //    LVDT_voltage = (analogRead(LVDT_pin) / 4096.0) * 3.3;  // Convert to voltage for esp32 (4095)
            //   client.print(LVDT_voltage);
            // }
            else if(header.indexOf("GET /pulses") >= 0)
            {
               meter = number/1.956; 
               client.print("pulses ");
               client.println(number);
               client.print("pulse_per_mm "); //calculated one 
               client.print(pulse_per_mm);
            }
            else if(header.indexOf("GET /start") >= 0)
            { 
              uint8_t x1 = 0;
              Serial2.write(x1); // sending 0 to stm32 to make A,B count = 0 
              LoadCell.tareNoDelay();
              brake_count = 0;
              uint8_t b1 =1;
              Serial2.write(b1); // sending 1 to stm32 for reset 
              SAMPLE = 1;
            }
            else if(header.indexOf("GET /stop") >= 0)
            {
              SAMPLE =0;
            }
            else if(header.indexOf("GET /login") >= 0)
            {
              //Serial.println("Logging in");
              // Display the HTML web page
              client.println("<!DOCTYPE html>");
              client.println("<html>");
              client.println("<body>");
              client.println("<form action=\"/action_page.php\">");

              /////////////  calibrated value and cof dividing factor 
              client.println("<label for=\"load_value\">load_value   : </label>"); //calibrated value
              client.println(" <input type=\"text\" id=\"load_value\" name=\"load_value\"><br><br>");

              client.println("<label for=\"dividing_factor\">dividing_factor: </label>");
              client.println("<input type=\"text\" id=\"dividing_factor\" name=\"dividing_factor\"><br><br>");

              client.println("<label for=\"pulse_per_mm\">pulse_per_mm: </label>");
              client.println("<input type=\"text\" id=\"pulse_per_mm\" name=\"pulse_per_mm\"><br><br>");
              
              client.println("<label for=\"wifi_name\">wifi_name: </label>");
              client.println("<input type=\"text\" id=\"wifi_name\" name=\"wifi_name\"><br><br>");
                            
                       ////////////////   submit button
              client.println("<input type=\"submit\" value=\"Submit\">");
              client.println("</form>");

              client.println("<p>Click on the submit button to submit the form.</p>");
              client.println("</body></html>");
              client.println("calibrationValue:");
              client.println(calibrationValue);
              client.println();
              client.println(" dividing_factor");
              client.println(dividing_factor);
              client.println();
              client.println(" pulse_per_mm:");
              client.println(pulse_per_mm);
              client.println();
              client.println(" wifi_name:");
              client.println(ssid_h);
              client.println();
              // The HTTP response ends with another blank line
              break;
            }
                     ///////////////   after submit seperating details
            else if (header.indexOf("GET /action_page.php") >= 0) 
            {
             
              int networkStart = header.indexOf("load_value=") + 11;
              int networkEnd = header.indexOf("&", networkStart);
              network_1 = header.substring(networkStart, networkEnd);  //calibrated value

               // Extract password
              int passwordStart = header.indexOf("dividing_factor=") + 16;
              int passwordEnd = header.indexOf("&", passwordStart);
              password_1 = header.substring(passwordStart, passwordEnd); //cof dividing value
              
             // Extract pulse_per_mm
              int pulsesStart = header.indexOf("pulse_per_mm=") + 13;
              int pulsesEnd = header.indexOf("&", pulsesStart);
              pulses = header.substring(pulsesStart, pulsesEnd); //cof dividing value

              // Extract wifi_name
              int wifi_nameStart = header.indexOf("wifi_name=") + 10;
              int wifi_nameEnd = header.indexOf(" ", wifi_nameStart);
              wifi_name = header.substring(wifi_nameStart, wifi_nameEnd); //cof dividing value
              
              // Serial.println("network : ");
              // Serial.println(network_1);
              // Serial.println("password : ");
              // Serial.println(password_1);
              // Serial.println("pulses : ");
              // Serial.println(pulses);
              // Serial.println("wifi_name : ");
              // Serial.println(wifi_name);
                if (network_1 != "")
                {
                  preferences.putString("ssid", network_1);  // ssid storing into flash
                }
                if (password_1 != "")
                {
                   preferences.putString("password", password_1);  // paasword storing into flash
                }
                if (pulses != "")
                {
                   preferences.putString("pulses", pulses);  // pulses storing into flash
                }
                if (wifi_name != "")
                {
                   preferences.putString("wifi_name", wifi_name);  // pulses storing into flash
                }
                preferences.end();
                //Serial.println("credientials Saved");
                ESP.restart();
            }
            break;
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


