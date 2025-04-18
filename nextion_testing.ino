#include <HardwareSerial.h>
#include <Preferences.h>
#include "SHT2x.h"
#include "Wire.h"

#define f1 33  //for relay1  
#define f2 25  //for relay2
#define f3 26  //for relay3
#define f4 14  //for relay4 33
#define f5 32  //for relay5
#define f6 12  //for relay6

// Initialize SoftwareSerial for Nextion communication
HardwareSerial nextionSerial(2);
Preferences preferences;

SHT2x sht[6];  //array for sensors

// Global Variables
const unsigned long htoms = 3.6e+6;
//const unsigned long minute = 60000;
volatile unsigned long threshold_ctime = 0;
unsigned long elapsedmillis = 0;
uint8_t alreadyrunhours = 0;
uint8_t basehours =0;
uint8_t rxdata[50];
uint8_t hour;
int selectedload;
int mode;
int a1, value;
uint8_t page;
uint8_t cycle_time;  // Default humidity and temperature printing on HMI
int c = 0;
int i = 0, f, r;
uint8_t countdown;
String s = "";
float theshold_temp, theshold_hum;
float theshold_temp11, theshold_hum11;
float default_temp = 20.5, default_hum = 80.0;  //default temperature and humidity
bool comparision;
 float temperature1, humidity1;
int relay[6] = { f1, f2, f3, f4, f5, f6 };  //pins for relays
//add zeros for replaced fans
int fan_array[6][6] = {
  { f3, 0, 0, 0, 0, 0 },
  { f2, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0 }
};

float set_temp[18] = { 55, 55, 55, 55, 55, 55, 50, 50, 50, 50, 45, 45, 45, 45, 45, 45, 50, 40 };
float set_hum[18]  = { 60, 60, 60, 60, 60, 60, 60, 55, 55, 60, 50, 50, 50, 50, 50, 50, 60, 50 };
uint8_t set_time[18]   = { 48, 48, 48, 48, 8, 24, 48, 50, 48, 50, 24, 36, 24, 12, 24, 12, 48, 24 };

String veg_array[18] = { "Mango", "Pineapple", "Chickoo", "Banana", "Guava", "Onion", "Tomato", "Beet root", "Carrot", "Lemon", "Mint", "Ginger", "Rama Tulasi", "Curry Leaf", "moringa", "Coriander", "Chilli", "Rose" };
String zones[6] = { "t5.txt=\"", "t3.txt=\"", "t4.txt=\"", "t6.txt=\"", "t7.txt=\"", "t8.txt=\"" };
String temp[6] = { "t10.txt=\"", "t0.txt=\"", "t9.txt=\"", "t11.txt=\"", "t12.txt=\"", "t13.txt=\"" };
String hum[6] = { "t16.txt=\"", "t14.txt=\"", "t15.txt=\"", "t17.txt=\"", "t18.txt=\"", "t19.txt=\"" };
String fan[6] = { "t22.txt=\"", "t20.txt=\"", "t21.txt=\"", "t23.txt=\"", "t24.txt=\"", "t25.txt=\"" };
String status[6] = { "t28.txt=\"", "t26.txt=\"", "t27.txt=\"", "t29.txt=\"", "t30.txt=\"", "t31.txt=\"" };

String zone_txt[6] = { "TH1", "TH2", "TH3", "TH4", "TH5", "TH6" };  //max char limit 10
String temp_txt[6] = { " ", " ", " ", " ", " ", " " };
String hum_txt[6] = { " ", " ", " ", " ", " ", " " };
String fan_txt[6] = { "FAN1", "FAN2", "FAN3", "FAN4", "FAN5", "FAN6" };
String status_txt[6] = { " ", " ", " ", " ", " ", " " };

void setup() {
  // Initialize Serial for debugging
  Serial.begin(115200);

  // Initialize SoftwareSerial for Nextion
  nextionSerial.begin(115200, SERIAL_8N1, 16, 17);  // Serial2 for Nextion
  for (int j = 0; j < 6; j++) {
    pinMode(relay[j], OUTPUT);  //setting output modes of pins
  }

  delay(1000);  // Wait for Nextion to initialize

  preferences.begin("memory", false);  //preferences for flash memory
 // preferences.clear();
  selectedload = preferences.getInt("selected", 0);
  page = preferences.getInt("page", 1);
  comparision = preferences.getInt("comparision", 0);
  basehours = preferences.getUChar("runhours", 0);
  Serial.println("alreadyrunhours");
  Serial.println(basehours);
  Serial.println(comparision);
  Serial.println(threshold_ctime);
  theshold_temp = set_temp[selectedload];  //comparision threshold temperature
  theshold_hum  = set_hum[selectedload];    //comparision threshold humidity
  cycle_time    = set_time[selectedload];
    Serial.println("theshold_temp");
  Serial.println(selectedload);
  Serial.println(theshold_temp);
  Serial.println(theshold_hum);
  Serial.println(cycle_time);
  for(i=0; i<18; i++)
  {
    set_temp[i] = preferences.getFloat(("set_temp" + (String)i).c_str(), set_temp[i]);
    set_hum[i] = preferences.getFloat(("set_hum" + (String)i).c_str(), set_hum[i]);
    set_time[i] = preferences.getInt(("set_time" + (String)i).c_str(), set_time[i]);
  }
  Wire.begin(21, 22);  //SDA, Scl
  for (i = 0; i < 6; i++) {
    // Init sensor on bus number 2
    TCA9548A(i);
    sht[i].begin();
  }
  if (comparision == 1) 
  {
        page = 2;
        preferences.putInt("page", page);  //storing page value in flash
        delay(100);
        nextionSerial.print("page page2");  //sending HMI to page 2
        nextionSerial.write(0xFF);          // End command
        nextionSerial.write(0xFF);
        nextionSerial.write(0xFF);
  }

}
 
void loop() 
{
  if (nextionSerial.available())          //uart receiving from HMI
  {                                       // Check if data is received from Nextion
    a1 = nextionSerial.available();       // Get number of bytes received           // Buffer to store incoming data
    nextionSerial.readBytes(rxdata, a1);  // Read data into buffer

    if (a1 >= 4)  // Ensure at least 4 bytes are received for valid data
    {
      value = rxdata[3] << 24 | rxdata[2] << 16 | rxdata[1] << 8 | rxdata[0];  // Convert bytes to integer
      if (value > 0) {
        Serial.println("Received value:");
        Serial.println(value);
      }
    }
  }
  
  if (value == 100)  //for page 1
  {
    page = 1;
    preferences.putInt("page", page);  //storing page value in flash
    value = 0;
    //delay(100);
  } 
  else if (value == 300)  //for page 3
  {
    value = 0;
    page = 3;
    preferences.putInt("page", page);  //storing page value in flash
    //delay(100);
  } 
  else if (value == 400)  //for page 4
  {
    value = 0;
    mode = 0;
    page = 4;
    preferences.putInt("page", page);  //storing page value in flash
   // delay(100);
    sendNextionCommand(veg_array[mode], "t0.txt=\""); 
    digit_2(mode, "t104.txt=\""); //intial printing of mode's temperaure humidity, cycle times
    digit_2(set_temp[mode], "t109.txt=\"");
    digit_2(set_hum[mode], "t110.txt=\"");
    digit_2(set_time[mode], "t120.txt=\"");
  } 
  else if (value == 500)  //for page 5
  {
    nextionSerial.print("page page2");  //sending HMI to page 2
    nextionSerial.write(0xFF);          // End command
    nextionSerial.write(0xFF);
    nextionSerial.write(0xFF);
    value = 0;
    page = 2;
    preferences.putInt("page", page);  //storing page value in flash
  }

  if (value >= 101 && value < 120)  //receives the selectedload of fruits
  {
    page = 2;
    preferences.putInt("page", page);  //storing page value in flash

    selectedload = (value % 100) - 1;                 //taking which load/picture is selected starts with 0 so -1.
    Serial.println("selectedload");
    Serial.println(selectedload);
    preferences.putInt("selected", selectedload);
    theshold_temp = set_temp[selectedload];  //comparision threshold temperature
    theshold_hum  = set_hum[selectedload];    //comparision threshold humidity
    cycle_time    = set_time[selectedload];
    alreadyrunhours = 0;
    preferences.putUChar("runhours", alreadyrunhours);  //hours stored in flash
    basehours = preferences.getUChar("runhours", alreadyrunhours);  //hours stored in flash 
    value = 0;
  }


  if (value == 150)  //start button pressed
  {
      comparision = 1;
      alreadyrunhours = 0;
      preferences.putUChar("runhours", alreadyrunhours);  //hours stored in flash
      preferences.putInt("comparision", comparision);
      basehours = preferences.getUChar("runhours", alreadyrunhours);  //hours stored in flash   //
      threshold_ctime = millis();  //started time
      value = 0;  //resetting the value after start pressed
  }





 if (comparision == 1) 
  {
    unsigned long currentmillis = millis();
    elapsedmillis = currentmillis - threshold_ctime;  //button pressed to till now time
    delay(1); // Or yield();
    Serial.println("threshold_ctime");
    Serial.println(threshold_ctime);
    Serial.println("elapsedmillis");
    Serial.println(elapsedmillis / 1000);
    Serial.println("basehours");
    Serial.println(basehours);
    Serial.println("elapsedmillis / htoms");
    Serial.println(elapsedmillis / htoms);

    int totalelapsedhours = basehours + (elapsedmillis / htoms);
    Serial.println("totalelapsedhours");
    Serial.println(totalelapsedhours);
    Serial.println("cycle_time");
    Serial.println(cycle_time);
    Serial.println("already hours");
    Serial.println(alreadyrunhours);
    if (totalelapsedhours > alreadyrunhours) 
    {
      alreadyrunhours = totalelapsedhours;
      Serial.println("completed hours");
      Serial.println(alreadyrunhours);
      preferences.putUChar("runhours", alreadyrunhours);  //hours stored in flash

      if (alreadyrunhours >= cycle_time) 
      {
        comparision = 0;
        alreadyrunhours = 0;
        threshold_ctime = 0;
        preferences.putInt("comparision", comparision);
        preferences.putUChar("runhours", alreadyrunhours);  //hours stored in flash
        basehours =  preferences.getUChar("runhours", alreadyrunhours);
        Serial.println("completed task");
        page = 5;
        preferences.putInt("page", page);  //storing page value in flash
        delay(100);
        nextionSerial.print("page page5");  //sending HMI to page 2
        nextionSerial.write(0xFF);          // End command
        nextionSerial.write(0xFF);
        nextionSerial.write(0xFF);
      }
    }
   for (i = 0; i < 6; i++) {
      sensor_readings(sht[i], i);  //
      //delay(100);
    }
  }

  ////////////////////////
   if(page ==2)
   {
    countdown = (cycle_time - alreadyrunhours);  //taking the countdown time
    String s = (String)countdown;
    sendNextionCommand(s, "t33.txt=\"");  //printing countdown time in textbox
    sendNextionCommand(veg_array[selectedload], "t2.txt=\""); 
    for (i = 0; i < 6; i++) 
    {
      sendNextionCommand(temp_txt[i], temp[i]);  //printing temperaures of all sensors in page2
      delay(100);
      sendNextionCommand(hum_txt[i], hum[i]);    //printing humidties of all sensors in page2
    }
  }
  if (page == 4) 
  {
    switch (value)
    {
      case 15: value = 0; mode++; Serial.println(mode); if(mode >17){mode = 0;}   
                          sendNextionCommand(veg_array[mode], "t0.txt=\""); 
                          digit_2(set_temp[mode], "t109.txt=\"");
                          digit_2(set_hum[mode], "t110.txt=\"");
                          digit_2(set_time[mode], "t120.txt=\""); break;
      case 3 : value = 0; set_temp[mode]++; digit_2(set_temp[mode], "t109.txt=\""); break; 
      case 4 : value = 0; set_hum[mode]++;  digit_2(set_hum[mode], "t110.txt=\"");  break;
      case 18: value = 0; set_time[mode]++; digit_2(set_time[mode], "t120.txt=\""); break;
      case 14: value = 0; Serial.println(mode); mode--; if(mode < 0){mode = 17;}
                          sendNextionCommand(veg_array[mode], "t0.txt=\"");
                          digit_2(set_temp[mode], "t109.txt=\"");
                          digit_2(set_hum[mode], "t110.txt=\"");
                          digit_2(set_time[mode], "t120.txt=\""); break;
      case 1 : value = 0; set_temp[mode]--; digit_2(set_temp[mode], "t109.txt=\""); break; 
      case 2 : value = 0; set_hum[mode]--;  digit_2(set_hum[mode], "t110.txt=\"");  break;
      case 19: value = 0; set_time[mode]--; digit_2(set_time[mode], "t120.txt=\""); break;
      case 17: value = 0; preferences.putFloat(("set_temp" + (String)mode).c_str(), set_temp[mode]);  
                          preferences.putFloat(("set_hum" + (String)mode).c_str(), set_hum[mode]); 
                          preferences.putInt(("set_time" + (String)mode).c_str(), set_time[mode]); 
                break;//save button
      default: break;
    }
  }

  delay(100);  // For stable communication with the Nextion display
}


//taking readins of all sensors and comparing with threshold values
void sensor_readings(SHT2x sensor, int bus) 
{
  r = bus;                                       //bus is sensor number like sensor0, sensor1 etc..
  TCA9548A(bus);                                 //must give fun call of multiplexer before reading of sensor
  sensor.read();                                 //must give fun call of sht2x libraray before reading of sensor
  temperature1 = sensor.getTemperature();  //taking temperature
  humidity1 = sensor.getHumidity();        //taking humidity
  if (temperature1 > 0 || humidity1 > 0) 
  {
    temp_txt[bus] = (String)temperature1;  //storing the temperature values temperature textbox array for HMI
    hum_txt[bus] = (String)humidity1;      //storing the humidity values humidity textbox array for HMI
  } 
  else if (temperature1 < 0 || humidity1 < 0) 
  {
    temp_txt[bus] = "ERR";  //storing the temperature values temperature textbox array for HMI
    hum_txt[bus] = "ERR";   //storing the humidity values humidity textbox array for HMI
  } 
  else 
  {
   // Serial.println("else");
  }

  Serial.println("thesholds");
  Serial.println(theshold_temp);
  Serial.println(theshold_hum);
   theshold_temp11 = theshold_temp - 0.5;  //default temperature
   theshold_hum11 = theshold_hum - 0.5;    //default humidity
  //comparing sensor readings with threshold values
    if (humidity1 < theshold_hum11)
    {
      if(temperature1 > theshold_temp11) 
      {
       for (int j = 0; j < 6; j++) 
       {
         if (fan_array[r][j] != 0)  //for 1 sensor different fans should ON
         {
           digitalWrite(fan_array[r][j], HIGH);  //relays will ON
           if(page == 2)
           sendNextionCommand("ON", status[r]);
           // Serial.println(fan_array[r][j]);
         }
        }
      }
      else if (temperature1 <= theshold_temp11) 
      {
        for (int j = 0; j < 6; j++) 
        {
         if (fan_array[r][j] != 0) 
         {
          digitalWrite(fan_array[r][j], LOW);  //relays will OFF
          if(page ==2)
          sendNextionCommand("OFF", status[r]);
         }
       } 
     }
    }
    else if (humidity1 >= theshold_hum11)
    { 
      if  (temperature1 > 28)
      {
        for (int j = 0; j < 6; j++)
        {
          if (fan_array[r][j] != 0)  //for 1 sensor different fans should ON
          {
            digitalWrite(fan_array[r][j], HIGH);  //relays will ON
            if(page ==2)
            sendNextionCommand("ON", status[r]);
          }
        }
      }
      else if (temperature1 <= 28)
      {
        for (int j = 0; j < 6; j++) 
        {
          if (fan_array[r][j] != 0)  //for 1 sensor different fans should ON
          {
            digitalWrite(fan_array[r][j], LOW);  //relays will OFF
             if(page ==2)
            sendNextionCommand("OFF", status[r]);
          }
        }
      }
    }
    if((digitalRead(f2) == LOW) && (digitalRead(f3) == LOW))
     {
       if(digitalRead(f1) != LOW)
       {
          digitalWrite(f1, LOW);  //common fan relays will OFF
       }
     }
    if((digitalRead(f2) == HIGH) || (digitalRead(f3) == HIGH))
     {
       if(digitalRead(f1) != HIGH)
       {
          digitalWrite(f1, HIGH);  //common fan relays will ON
       }
     }
 }


// Select I2C BUS
void TCA9548A(uint8_t bus)  //selecting bus for TCA multiplexer
{
  Wire.beginTransmission(0x70);  // TCA9548A address 0x70 because A0, A1,A2 pins of multiplexer are connected to ground
  Wire.write(1 << bus);          // send byte to select bus
  Wire.endTransmission();
}

// Function to send Nextion commands
void sendNextionCommand(String command, String txt_box) {
  nextionSerial.print(txt_box);  // Select text box
  nextionSerial.print(command);
  nextionSerial.print("\"");  // End of string
  nextionSerial.write(0xFF);  // End of command
  nextionSerial.write(0xFF);
  nextionSerial.write(0xFF);
}


// Function to update temperature and humidity values
void digit_2(int num, String txtbox) {
  num = num % 100;              // Take the last 2 digits
  num = abs(num);               // Ensure non-negative
  String num1 = String(num);    // Convert to string
  nextionSerial.print(txtbox);  // Select text box
  nextionSerial.print(num1);    // Display number
  nextionSerial.print("\"");    // End of string
  nextionSerial.write(0xFF);    // End of command
  nextionSerial.write(0xFF);
  nextionSerial.write(0xFF);
}













