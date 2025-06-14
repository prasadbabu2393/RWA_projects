#include <HardwareSerial.h>
#include "stdint.h"
#include <WiFi.h>
#include <Preferences.h>
#include <Update.h>
#include <WebServer.h>

// Use separate preferences for WiFi and register configurations
Preferences wifiPrefs;
Preferences registerPrefs;

WebServer webServer(80);

#define RXD2 16  //esp32 Rx
#define TXD2 17  //esp32 Tx

IPAddress flash_ip;
bool MODE;
String ssid, password, ip, baudrate;
String network_1, password_1, client_ip, baudrate_1;
String voltage, current, pf, kva, kwh;
String Add_pre[5], Add_flash[5];

// New arrays for data type and byte order configuration
String DataType_flash[5];  // "float" or "int64"
String ByteOrder_flash[5]; // "msb" or "lsb"

const char *ssid_h = "ENERGYMETER";
const char *password_h = "12345678";

int tx_buadrate = 19200;
int DE_RE_pin = 21;   // DE and RE pin
int ledpin = 5;       // led pin
const int resetPin = 19;  // Set your reset button pin
int programState = 0, buttonState;
long buttonMillis = 0;
int retry_count = 0;

// Add these global variables at the top of your code (after other global variables)
unsigned long lastWiFiCheck = 0;
const unsigned long wifiCheckInterval = 5000; // Check WiFi every 5 seconds
int reconnectAttempts = 0;
bool isReconnecting = false;

unsigned long currentTime = millis();
unsigned long previousTime = 0;
const long timeoutTime = 2000;

//Address order: vol, curr, pf, kwh, kva
uint32_t MHeaders[5];
uint8_t Transmitidx = 0;
uint16_t Maddr;

String header;

////////////     static IP
int a = 192, b = 168, c = 0, d = 118;
IPAddress gateway(192, 168, 0, 1);
IPAddress subnet(255, 255, 255, 0);

///////////////   function declarations ///////////
uint16_t crc16(uint8_t *buffer, uint16_t buffer_length);
void Transmit(uint16_t addr);
void setupOTA();
void handleOTAUpload();
void handleOTAForm();
float convertToFloat(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6);
int64_t convertToInt64(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6);
float convertToPF(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6);  // ADD THIS LINE

///////////////////////////////    reading holding 32 bit registers     ///////////////////////////////////
byte TxData[] = { 1, 3, 0x01, 0xF4, 0x00, 0x02 };
uint8_t numRegs = TxData[5];
byte RxData[15];

//////////////////    creating Data stucture
typedef struct arr_strc  
{
  uint8_t id;
  int64_t kwh;
  int64_t kva;
  float data[6];
} strc_arr;

volatile strc_arr dataStruct[1];

/* Table of CRC values for high-order byte */
static const uint8_t table_crc_hi[] = {
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
  0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
  0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40, 0x00, 0xC1, 0x81, 0x40,
  0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0, 0x80, 0x41, 0x00, 0xC1,
  0x81, 0x40, 0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41,
  0x00, 0xC1, 0x81, 0x40, 0x01, 0xC0, 0x80, 0x41, 0x01, 0xC0,
  0x80, 0x41, 0x00, 0xC1, 0x81, 0x40
};

/* Table of CRC values for low-order byte */
static const uint8_t table_crc_lo[] = {
  0x00, 0xC0, 0xC1, 0x01, 0xC3, 0x03, 0x02, 0xC2, 0xC6, 0x06,
  0x07, 0xC7, 0x05, 0xC5, 0xC4, 0x04, 0xCC, 0x0C, 0x0D, 0xCD,
  0x0F, 0xCF, 0xCE, 0x0E, 0x0A, 0xCA, 0xCB, 0x0B, 0xC9, 0x09,
  0x08, 0xC8, 0xD8, 0x18, 0x19, 0xD9, 0x1B, 0xDB, 0xDA, 0x1A,
  0x1E, 0xDE, 0xDF, 0x1F, 0xDD, 0x1D, 0x1C, 0xDC, 0x14, 0xD4,
  0xD5, 0x15, 0xD7, 0x17, 0x16, 0xD6, 0xD2, 0x12, 0x13, 0xD3,
  0x11, 0xD1, 0xD0, 0x10, 0xF0, 0x30, 0x31, 0xF1, 0x33, 0xF3,
  0xF2, 0x32, 0x36, 0xF6, 0xF7, 0x37, 0xF5, 0x35, 0x34, 0xF4,
  0x3C, 0xFC, 0xFD, 0x3D, 0xFF, 0x3F, 0x3E, 0xFE, 0xFA, 0x3A,
  0x3B, 0xFB, 0x39, 0xF9, 0xF8, 0x38, 0x28, 0xE8, 0xE9, 0x29,
  0xEB, 0x2B, 0x2A, 0xEA, 0xEE, 0x2E, 0x2F, 0xEF, 0x2D, 0xED,
  0xEC, 0x2C, 0xE4, 0x24, 0x25, 0xE5, 0x27, 0xE7, 0xE6, 0x26,
  0x22, 0xE2, 0xE3, 0x23, 0xE1, 0x21, 0x20, 0xE0, 0xA0, 0x60,
  0x61, 0xA1, 0x63, 0xA3, 0xA2, 0x62, 0x66, 0xA6, 0xA7, 0x67,
  0xA5, 0x65, 0x64, 0xA4, 0x6C, 0xAC, 0xAD, 0x6D, 0xAF, 0x6F,
  0x6E, 0xAE, 0xAA, 0x6A, 0x6B, 0xAB, 0x69, 0xA9, 0xA8, 0x68,
  0x78, 0xB8, 0xB9, 0x79, 0xBB, 0x7B, 0x7A, 0xBA, 0xBE, 0x7E,
  0x7F, 0xBF, 0x7D, 0xBD, 0xBC, 0x7C, 0xB4, 0x74, 0x75, 0xB5,
  0x77, 0xB7, 0xB6, 0x76, 0x72, 0xB2, 0xB3, 0x73, 0xB1, 0x71,
  0x70, 0xB0, 0x50, 0x90, 0x91, 0x51, 0x93, 0x53, 0x52, 0x92,
  0x96, 0x56, 0x57, 0x97, 0x55, 0x95, 0x94, 0x54, 0x9C, 0x5C,
  0x5D, 0x9D, 0x5F, 0x9F, 0x9E, 0x5E, 0x5A, 0x9A, 0x9B, 0x5B,
  0x99, 0x59, 0x58, 0x98, 0x88, 0x48, 0x49, 0x89, 0x4B, 0x8B,
  0x8A, 0x4A, 0x4E, 0x8E, 0x8F, 0x4F, 0x8D, 0x4D, 0x4C, 0x8C,
  0x44, 0x84, 0x85, 0x45, 0x87, 0x47, 0x46, 0x86, 0x82, 0x42,
  0x43, 0x83, 0x41, 0x81, 0x80, 0x40
};

// URL decode function
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
  return input;
}

///////////////   Transmit function
void Transmit(uint16_t addr) {
  TxData[2] = byte((addr >> 8) & 0xFF);
  TxData[3] = byte(addr & 0xFF);
 
  if (DataType_flash[Transmitidx] == "int64") { //need 4 register data bytes
    TxData[5] = 0x04;
    numRegs = 0x04;
  }else {
    TxData[5] = 0x02;
    numRegs = 0x02;
  }
  
  uint16_t abc = crc16(TxData, 6);
  TxData[6] = byte(abc & 0xFF);
  TxData[7] = byte((abc >> 8) & 0xFF);

  digitalWrite(DE_RE_pin, HIGH);
  Serial.println("transmitting");
  for(int s=0; s<8; s++) {
    Serial.println(TxData[s]);
  }
  Serial2.write(TxData, 8);
  Serial2.flush();

  digitalWrite(DE_RE_pin, LOW);
}

///////////////////////          ReadRegs function - Enhanced with data type and byte order
void readRegs(uint8_t *Data_rx, uint8_t j, uint8_t Regs) {
  if (Data_rx[0] == TxData[0]) {
    if (Data_rx[1] == TxData[1]) {
      if (Data_rx[2] == 0x04 || Data_rx[2] == 0x08) {
        // Get data type and byte order for current register
        String dataType = DataType_flash[Transmitidx];
        String byteOrder = ByteOrder_flash[Transmitidx];

          Serial.println("Processing register " + String(Transmitidx) + " with data type: " + dataType + ", byte order: " + byteOrder);
           // Arrange bytes according to byte order
          uint8_t byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7;

        if(dataType == "int64")
        {
          if (byteOrder == "lsb") {
             // LSB first: Data_rx[6], Data_rx[5], Data_rx[4], Data_rx[3]
            byte0 = Data_rx[10];
            byte1 = Data_rx[9];
            byte2 = Data_rx[8];
            byte3 = Data_rx[7];
            byte4 = Data_rx[6];
            byte5 = Data_rx[5];
            byte6 = Data_rx[4];
            byte7 = Data_rx[3];

           } else {
             // MSB first (default): Data_rx[3], Data_rx[4], Data_rx[5], Data_rx[6]
             byte0 = Data_rx[3];
             byte1 = Data_rx[4];
             byte2 = Data_rx[5];
             byte3 = Data_rx[6];
             byte4 = Data_rx[7];
             byte5 = Data_rx[8];
             byte6 = Data_rx[9];
             byte7 = Data_rx[10];
           }
        }
        else 
        {      
        //   // Arrange bytes according to byte order
        //  uint8_t byte0, byte1, byte2, byte3;
         if (byteOrder == "lsb") {
          // LSB first: Data_rx[6], Data_rx[5], Data_rx[4], Data_rx[3]
          byte0 = Data_rx[6];
          byte1 = Data_rx[5];
          byte2 = Data_rx[4];
          byte3 = Data_rx[3];
         } else {
          // MSB first (default): Data_rx[3], Data_rx[4], Data_rx[5], Data_rx[6]
          byte0 = Data_rx[3];
          byte1 = Data_rx[4];
          byte2 = Data_rx[5];
          byte3 = Data_rx[6];
         }
        }
        // Convert based on data type
        if (dataType == "int64") {
          int64_t int_value = convertToInt64(byte0, byte1, byte2, byte3, byte4, byte5, byte6, byte7);
          dataStruct[0].data[Transmitidx] = (float)int_value;
          Serial.println("Int64 value: " + String((long long)int_value));
        } else if (dataType == "pf") {
          float pf_value = convertToPF(byte0, byte1, byte2, byte3);
          dataStruct[0].data[Transmitidx] = pf_value;
          Serial.println("PF value: " + String(pf_value));
        } else {
          // Default to float
          float float_value = convertToFloat(byte0, byte1, byte2, byte3);
          dataStruct[0].data[Transmitidx] = float_value;
          Serial.println("Float value: " + String(float_value));
        }
      }
    }
  }
  
  Transmitidx++;
  if (Transmitidx <= 4) {
    Maddr = MHeaders[Transmitidx];
    Transmit(Maddr);
  } else {
    Transmitidx = 0;
    Maddr = MHeaders[Transmitidx];
    Serial.println("All data received:");
    for (int k = 0; k < 5; k++) {
      Serial.println("Register " + String(k) + ": " + String(dataStruct[0].data[k]));
    }
  }
}

/////////////////////////   convertToPF function
float convertToPF(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6) {
  float pf = 0.0, regval = 0.0;
  regval = convertToFloat(Data3, Data4, Data5, Data6);
  if (regval > 1.0) {
    pf = 2 - regval;
  } else if (regval < -1.0) {
    pf = -2 - regval;
  } else if (abs(regval) == 1) {
    pf = regval;
  } else {
    pf = regval;
  }
  return pf;
}

/////////////////////////   convertToFloat function
float convertToFloat(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6) {
  Serial.println("Converting to float");
  Serial.println("Bytes: " + String(Data3) + ", " + String(Data4) + ", " + String(Data5) + ", " + String(Data6));
  
  uint8_t sign = Data3 & 0x80;
  uint8_t expo = (Data3 << 1) | (Data4 >> 7);
  uint32_t m = ((uint32_t)(Data4 & 0x7F) << 16) | ((uint32_t)Data5 << 8) | Data6;
  float val = 0;

  for (int p = 22; p >= 0; --p) {
    if (m & (1 << p)) {
      val += pow(2, p - 23);
    }
  }
  val += 1;
  val = val * (pow(2, expo - 127));
  if (sign) {
    val = -val;
  }
  Serial.println("Float result: " + String(val));
  return val;
}

// /////////////////////////   convertToInt64 function - NEW
// int64_t convertToInt64(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6) {
//   Serial.println("Converting to int64");
//   Serial.println("Bytes: " + String(Data3) + ", " + String(Data4) + ", " + String(Data5) + ", " + String(Data6));
  
//   // Combine 4 bytes into a 32-bit integer
//   uint32_t combined = ((uint32_t)Data3 << 24) | ((uint32_t)Data4 << 16) | ((uint32_t)Data5 << 8) | Data6;
  
//   // Convert to signed 64-bit integer
//   int64_t result = (int64_t)combined;
  
//   Serial.println("Int64 result: " + String((long long)result));
//   return result;
// }
int64_t convertToInt64(uint8_t Data3, uint8_t Data4, uint8_t Data5, uint8_t Data6, uint8_t Data7, uint8_t Data8, uint8_t Data9, uint8_t Data10) {

  int64_t val_64 = ((int64_t)Data3 << 56) | ((int64_t)Data4 << 48) | ((int64_t)Data5 << 40) | ((int64_t)Data6 << 32) | ((int64_t)Data7 << 24) | ((int64_t)Data8 << 16) | ((int64_t)Data9 << 8) | (int64_t)Data10;
  return val_64;
}


/////////////////////////   crc16 function
uint16_t crc16(uint8_t *buffer, uint16_t buffer_length) {
  uint8_t crc_hi = 0xFF;
  uint8_t crc_lo = 0xFF;
  unsigned int i;

  while (buffer_length--) {
    i = crc_lo ^ *buffer++;
    crc_lo = crc_hi ^ table_crc_hi[i];
    crc_hi = table_crc_lo[i];
  }

  return (crc_hi << 8 | crc_lo);
}

//////// string IP to int function
void convert(String address) {
  int d1 = address.indexOf('.');
  int d2 = address.indexOf('.', d1 + 1);
  int d3 = address.indexOf('.', d2 + 1);

  a = address.substring(0, d1).toInt();
  b = address.substring(d1 + 1, d2).toInt();
  c = address.substring(d2 + 1, d3).toInt();
  d = address.substring(d3 + 1).toInt();
}

// OTA Setup Function
void setupOTA() {
  // Handle OTA update form
  webServer.on("/ota", HTTP_GET, handleOTAForm);
  
  // Handle file upload
  webServer.on("/update", HTTP_POST, []() {
    webServer.sendHeader("Connection", "close");
    if (Update.hasError()) {
      webServer.send(500, "text/plain", "Update Failed!");
    } else {
      webServer.send(200, "text/plain", "Update Successful! Rebooting...");
      delay(1000);
      ESP.restart();
    }
  }, handleOTAUpload);
  
  Serial.println("OTA server ready");
}

// Handle OTA Form
void handleOTAForm() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>ESP32 OTA Update</title>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f0f0f0; }";
  html += ".container { max-width: 600px; margin: 0 auto; background: white; padding: 20px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; }";
  html += ".form-group { margin: 20px 0; }";
  html += "label { display: block; margin-bottom: 5px; font-weight: bold; }";
  html += "input[type='file'] { width: 100%; padding: 10px; border: 1px solid #ddd; border-radius: 5px; }";
  html += ".btn { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; width: 100%; }";
  html += ".btn:hover { background-color: #45a049; }";
  html += ".info { background-color: #e7f3ff; padding: 15px; border-radius: 5px; margin: 20px 0; }";
  html += ".warning { background-color: #fff3cd; padding: 15px; border-radius: 5px; margin: 20px 0; border-left: 4px solid #ffc107; }";
  html += "#progress { width: 100%; background-color: #f0f0f0; border-radius: 5px; margin: 20px 0; display: none; }";
  html += "#progress-bar { width: 0%; height: 30px; background-color: #4CAF50; border-radius: 5px; text-align: center; line-height: 30px; color: white; }";
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>ESP32 OTA Firmware Update</h1>";
  
  html += "<div class='info'>";
  html += "<strong>Device Information:</strong><br>";
  html += "MAC Address: " + WiFi.macAddress() + "<br>";
  html += "Current IP: " + WiFi.localIP().toString() + "<br>";
  html += "<strong>UART Configuration:</strong><br>";
  html += "Baud Rate: " + String(tx_buadrate) + "<br>";
  html += "Data Format: 8E1 (8 data bits, Even parity, 1 stop bit)<br>";
  html += "RX Pin: GPIO " + String(RXD2) + "<br>";
  html += "TX Pin: GPIO " + String(TXD2) + "<br>";
  html += "DE/RE Pin: GPIO " + String(DE_RE_pin) + "<br>";
  html += "</div>";
  
  html += "<div class='warning'>";
  html += "<strong>Warning:</strong> Only upload valid .bin firmware files. Invalid files may brick your device!";
  html += "</div>";
  
  html += "<form method='POST' action='/update' enctype='multipart/form-data' id='upload-form'>";
  html += "<div class='form-group'>";
  html += "<label for='update'>Select Firmware File (.bin):</label>";
  html += "<input type='file' name='update' id='file-input' accept='.bin' required>";
  html += "</div>";
  html += "<input type='submit' value='Update Firmware' class='btn' id='upload-btn'>";
  html += "</form>";
  
  html += "<div id='progress'>";
  html += "<div id='progress-bar'>0%</div>";
  html += "</div>";
  
  html += "<div style='margin-top: 20px; text-align: center;'>";
  html += "<a href='/login' style='color: #007bff; text-decoration: none;'>← Back to Configuration</a>";
  html += "</div>";
  
  html += "</div>";
  
  // JavaScript for upload progress
  html += "<script>";
  html += "document.getElementById('upload-form').addEventListener('submit', function(e) {";
  html += "  var fileInput = document.getElementById('file-input');";
  html += "  if (!fileInput.files[0]) {";
  html += "    alert('Please select a file');";
  html += "    e.preventDefault();";
  html += "    return;";
  html += "  }";
  html += "  if (!fileInput.files[0].name.endsWith('.bin')) {";
  html += "    alert('Please select a .bin file');";
  html += "    e.preventDefault();";
  html += "    return;";
  html += "  }";
  html += "  document.getElementById('upload-btn').disabled = true;";
  html += "  document.getElementById('upload-btn').value = 'Uploading...';";
  html += "  document.getElementById('progress').style.display = 'block';";
  html += "  simulateProgress();";
  html += "});";
  
  html += "function simulateProgress() {";
  html += "  var progress = 0;";
  html += "  var interval = setInterval(function() {";
  html += "    progress += 2;";
  html += "    if (progress > 90) progress = 90;";
  html += "    document.getElementById('progress-bar').style.width = progress + '%';";
  html += "    document.getElementById('progress-bar').textContent = progress + '%';";
  html += "    if (progress >= 90) clearInterval(interval);";
  html += "  }, 200);";
  html += "}";
  html += "</script>";
  
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

// Handle OTA Upload
void handleOTAUpload() {
  HTTPUpload& upload = webServer.upload();
  
  if (upload.status == UPLOAD_FILE_START) {
    Serial.printf("Update: %s\n", upload.filename.c_str());
    
    // Start update process
    if (!Update.begin(UPDATE_SIZE_UNKNOWN)) {
      Update.printError(Serial);
    }
  } 
  else if (upload.status == UPLOAD_FILE_WRITE) {
    // Write firmware data
    if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
      Update.printError(Serial);
    } else {
      Serial.printf("Written: %d bytes\n", upload.currentSize);
    }
  } 
  else if (upload.status == UPLOAD_FILE_END) {
    if (Update.end(true)) {
      Serial.printf("Update Success: %u bytes\nRebooting...\n", upload.totalSize);
    } else {
      Update.printError(Serial);
    }
  }
}

//for sending modbus frames to slave
void handle_readdata(){
  // Trigger fresh reading
  Transmitidx = 0;
  Maddr = MHeaders[Transmitidx];
  Transmit(Maddr);
  
  // Return current data as JSON
  String json = "{";
  json += "\"status\":\"reading_triggered\",";
  json += "\"voltage\":" + String(dataStruct[0].data[0], 2) + ",";
  json += "\"current\":" + String(dataStruct[0].data[1], 2) + ",";
  json += "\"power_factor\":" + String(dataStruct[0].data[2], 2) + ",";
  json += "\"kwh\":" + String(dataStruct[0].data[3], 2) + ",";
  json += "\"kva\":" + String(dataStruct[0].data[4], 2);
  json += "}";
  
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", json);
}

//for displaying the raw values of data like voltage, current etc
void handle_getdata(){
  String response = "{";
  response += String(dataStruct[0].data[0], 0) + ",";  // Voltage (no decimals)
  response += String(dataStruct[0].data[1], 0) + ",";  // Current (no decimals)
  response += String(dataStruct[0].data[2], 0) + ",";  // Power Factor (no decimals)
  response += String(dataStruct[0].data[3], 0) + ",";  // KWH (no decimals)
  response += String(dataStruct[0].data[4], 0);        // KVA (no decimals)
  response += "}";
  
  webServer.send(200, "text/plain", response);
}

// Enhanced Configuration page handler with data type and byte order dropdowns
void handleLogin() {
  String html = "<!DOCTYPE html><html><head>";
  html += "<title>Energy Meter Configuration</title>";
  html += "<meta name='viewport' content='width=device-width, initial-scale=1.0'>";
  html += "<style>";
  html += "body { font-family: Arial, sans-serif; margin: 20px; background-color: #f5f5f5; }";
  html += ".container { max-width: 900px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 15px rgba(0,0,0,0.1); }";
  html += "h1 { color: #333; text-align: center; margin-bottom: 30px; }";
  html += ".form-group { margin: 20px 0; }";
  html += "label { display: block; margin-bottom: 5px; font-weight: bold; color: #555; }";
  html += "input[type='text'], input[type='password'], select { width: 100%; padding: 12px; border: 1px solid #ddd; border-radius: 5px; font-size: 16px; }";
  html += "select { background-color: white; }";
  html += ".btn { background-color: #4CAF50; color: white; padding: 12px 20px; border: none; border-radius: 5px; cursor: pointer; font-size: 16px; margin: 10px 5px; }";
  html += ".btn:hover { background-color: #45a049; }";
  html += ".btn-secondary { background-color: #007bff; }";
  html += ".btn-secondary:hover { background-color: #0056b3; }";
  html += ".info { background-color: #e7f3ff; padding: 15px; border-radius: 5px; margin: 20px 0; }";
  html += ".current-readings { background-color: #f8f9fa; padding: 20px; border-radius: 5px; margin: 20px 0; }";
  html += ".reading-item { display: inline-block; margin: 10px 15px; padding: 10px; background: white; border-radius: 5px; box-shadow: 0 2px 4px rgba(0,0,0,0.1); }";
  
  // Enhanced styles for register configuration
  html += ".register-config { background-color: #f8f9fa; padding: 20px; border-radius: 8px; margin: 15px 0; border-left: 4px solid #4CAF50; }";
  html += ".register-row { display: grid; grid-template-columns: 2fr 1fr 1fr; gap: 15px; align-items: end; margin-bottom: 15px; }";
  html += ".register-label { font-weight: bold; color: #333; margin-bottom: 10px; }";
  html += ".dropdown-label { font-size: 14px; margin-bottom: 5px; color: #666; }";
  
  // Add password visibility toggle styles
  html += ".password-container { position: relative; }";
  html += ".password-toggle { position: absolute; right: 12px; top: 50%; transform: translateY(-50%); cursor: pointer; user-select: none; padding: 5px; }";
  html += ".password-toggle:hover { color: #4CAF50; }";
  html += ".eye-icon { width: 20px; height: 20px; fill: #666; }";
  html += ".eye-icon:hover { fill: #4CAF50; }";
  
  html += "</style></head><body>";
  
  html += "<div class='container'>";
  html += "<h1>Energy Meter Configuration</h1>";
  
  // Current readings display
  html += "<div class='current-readings'>";
  html += "<h3>Current Readings:</h3>";
  html += "<div class='reading-item'><strong>Voltage:</strong> " + String(dataStruct[0].data[0], 2) + " V</div>";
  html += "<div class='reading-item'><strong>Current:</strong> " + String(dataStruct[0].data[1], 2) + " A</div>";
  html += "<div class='reading-item'><strong>Power Factor:</strong> " + String(dataStruct[0].data[2], 2) + "</div>";
  html += "<div class='reading-item'><strong>KWH:</strong> " + String(dataStruct[0].data[3], 2) + " kWh</div>";
  html += "<div class='reading-item'><strong>KVA:</strong> " + String(dataStruct[0].data[4], 2) + " kVA</div>";
  html += "</div>";
  
  html += "<div class='info'>";
  html += "<strong>Device Information:</strong><br>";
  html += "MAC Address: " + WiFi.macAddress() + "<br>";
  html += "Current IP: " + WiFi.localIP().toString() + "<br>";
  html += "</div>";
  
  html += "<form method='POST' action='/save'>";
  
  html += "<h3>WiFi Configuration:</h3>";
  html += "<div class='form-group'>";
  html += "<label for='ssid'>WiFi Network Name (SSID):</label>";
  html += "<input type='text' name='ssid' value='" + ssid + "' placeholder='Enter WiFi network name (leave empty to keep current)'>";
  html += "</div>";
  
  html += "<div class='form-group'>";
  html += "<label for='password'>WiFi Password:</label>";
  html += "<div class='password-container'>";
  html += "<input type='password' name='password' id='password' value='" + password + "' placeholder='Enter WiFi password (leave empty to keep current)'>";
  html += "<span class='password-toggle' onclick='togglePassword()'>";
  // SVG eye icon
  html += "<svg class='eye-icon' id='eye-icon' xmlns='http://www.w3.org/2000/svg' viewBox='0 0 24 24'>";
  html += "<path id='eye-open' d='M12 4.5C7 4.5 2.73 7.61 1 12c1.73 4.39 6 7.5 11 7.5s9.27-3.11 11-7.5c-1.73-4.39-6-7.5-11-7.5zM12 17c-2.76 0-5-2.24-5-5s2.24-5 5-5 5 2.24 5 5-2.24 5-5 5zm0-8c-1.66 0-3 1.34-3 3s1.34 3 3 3 3-1.34 3-3-1.34-3-3-3z'/>";
  html += "<path id='eye-closed' style='display:none' d='M12 7c2.76 0 5 2.24 5 5 0 .65-.13 1.26-.36 1.83l2.92 2.92c1.51-1.26 2.7-2.89 3.43-4.75-1.73-4.39-6-7.5-11-7.5-1.4 0-2.74.25-3.98.7l2.16 2.16C10.74 7.13 11.35 7 12 7zM2 4.27l2.28 2.28.46.46C3.08 8.3 1.78 10.02 1 12c1.73 4.39 6 7.5 11 7.5 1.55 0 3.03-.3 4.38-.84l.42.42L19.73 22 21 20.73 3.27 3 2 4.27zM7.53 9.8l1.55 1.55c-.05.21-.08.43-.08.65 0 1.66 1.34 3 3 3 .22 0 .44-.03.65-.08l1.55 1.55c-.67.33-1.41.53-2.2.53-2.76 0-5-2.24-5-5 0-.79.2-1.53.53-2.2zm4.31-.78l3.15 3.15.02-.16c0-1.66-1.34-3-3-3l-.17.01z'/>";
  html += "</svg>";
  html += "</span>";
  html += "</div>";
  html += "</div>";
  
  html += "<h3>Network Configuration:</h3>";
  html += "<div class='form-group'>";
  html += "<label for='staticIP'>Static IP Address:</label>";
  html += "<input type='text' name='staticIP' value='" + ip + "' placeholder='192.168.1.100 (leave empty for DHCP)'>";
  html += "</div>";
  
  // Enhanced Modbus Register Configuration
  html += "<h3>Modbus Register Configuration:</h3>";
  
  for (int i = 0; i < 5; i++) {
    String labels[] = {"Voltage", "Current", "Power Factor", "KWH", "KVA"};
    html += "<div class='register-config'>";
    html += "<div class='register-label'>" + labels[i] + " Register Configuration</div>";
    html += "<div class='register-row'>";
    
    // Register Address
    html += "<div>";
    html += "<label class='dropdown-label' for='addr" + String(i+1) + "'>Register Address:</label>";
    html += "<input type='text' name='addr" + String(i+1) + "' value='" + Add_flash[i] + "' placeholder='Register address (leave empty to existing address )'>";
    html += "</div>";
    
    // Data Type Dropdown
    html += "<div>";
    html += "<label class='dropdown-label' for='datatype" + String(i+1) + "'>Data Type:</label>";
    html += "<select name='datatype" + String(i+1) + "'>";
    html += "<option value='float'";
    html += (DataType_flash[i] == "float" ? " selected" : "");
    html += ">Float (IEEE 754)</option>";
    html += "<option value='int64'";
    html += (DataType_flash[i] == "int64" ? " selected" : "");
    html += ">Integer 64-bit</option>";
    html += "<option value='pf'";
    html += (DataType_flash[i] == "pf" ? " selected" : "");
    html += ">Power Factor</option>";
    html += "</select>";
    html += "</div>";
    
    // Byte Order Dropdown
    html += "<div>";
    html += "<label class='dropdown-label' for='byteorder" + String(i+1) + "'>Byte Order:</label>";
    html += "<select name='byteorder" + String(i+1) + "'>";
    html += "<option value='msb'";
    html += (ByteOrder_flash[i] == "msb" ? " selected" : "");
    html += ">MSB First (Big Endian)</option>";
    html += "<option value='lsb'";
    html += (ByteOrder_flash[i] == "lsb" ? " selected" : "");
    html += ">LSB First (Little Endian)</option>";
    html += "</select>";
    html += "</div>";
    
    html += "</div>";
    html += "</div>";
  }
  
  html += "<div style='background-color: #fff3cd; padding: 15px; border-radius: 5px; margin: 20px 0; border-left: 4px solid #ffc107;'>";
  html += "<strong>Configuration Notes:</strong><br>";
  html += " Each register can be configured independently<br>";
  html += " <strong>Float:</strong> IEEE 754 32-bit floating point format<br>";
  html += " <strong>Int64:</strong> 32-bit integer converted to 64-bit<br>";
  html += " <strong>Power Factor:</strong> Special power factor conversion<br>";
  html += " <strong>MSB First:</strong> Most Significant Byte first (Big Endian)<br>";
  html += " <strong>LSB First:</strong> Least Significant Byte first (Little Endian)<br>";
  html += " Empty address fields will reset to existing register addresses";
  html += "</div>";
  
  html += "<input type='submit' value='Save Configuration' class='btn'>";
  html += "<a href='/ota' class='btn btn-secondary'>Firmware Update</a>";
  html += "<button type='button' onclick='fetch(\"/readdata\").then(() => location.reload())' class='btn btn-secondary'>Refresh Data</button>";
  html += "</form>";
  
  html += "</div>";
  
  // JavaScript for password toggle
  html += "<script>";
  html += "function togglePassword() {";
  html += "  var passwordInput = document.getElementById('password');";
  html += "  var eyeOpen = document.getElementById('eye-open');";
  html += "  var eyeClosed = document.getElementById('eye-closed');";
  html += "  if (passwordInput.type === 'password') {";
  html += "    passwordInput.type = 'text';";
  html += "    eyeOpen.style.display = 'none';";
  html += "    eyeClosed.style.display = 'block';";
  html += "  } else {";
  html += "    passwordInput.type = 'password';";
  html += "    eyeOpen.style.display = 'block';";
  html += "    eyeClosed.style.display = 'none';";
  html += "  }";
  html += "}";
  html += "</script>";
  
  html += "</body></html>";
  
  webServer.send(200, "text/html", html);
}

/// Enhanced Handle configuration save with data type and byte order support
void handleSave() {
  // Check if any form parameter was submitted
  if (webServer.args() > 0) {
    
    bool wifi_changes_made = false;
    bool register_changes_made = false;
    
    // Handle SSID independently
    if (webServer.hasArg("ssid")) {
      String new_ssid = urlDecode(webServer.arg("ssid"));
      if (new_ssid != "" && new_ssid != ssid) {
        ssid = new_ssid;
        wifiPrefs.putString("ssid", ssid);
        wifi_changes_made = true;
        Serial.println("SSID updated: " + ssid);
      } else if (new_ssid == "" && ssid != "") {
        // Clear SSID if field is empty
        ssid = "";
        wifiPrefs.remove("ssid");
        wifi_changes_made = true;
        Serial.println("SSID cleared");
      }
    }
    
    // Handle Password independently
    if (webServer.hasArg("password")) {
      String new_password = urlDecode(webServer.arg("password"));
      if (new_password != "" && new_password != password) {
        password = new_password;
        wifiPrefs.putString("password", password);
        wifi_changes_made = true;
        Serial.println("Password updated");
      } else if (new_password == "" && password != "") {
        // Clear password if field is empty
        password = "";
        wifiPrefs.remove("password");
        wifi_changes_made = true;
        Serial.println("Password cleared");
      }
    }
    
    // Handle Static IP independently
    if (webServer.hasArg("staticIP")) {
      String new_ip = urlDecode(webServer.arg("staticIP"));
      if (new_ip != "" && new_ip != ip) {
        ip = new_ip;
        wifiPrefs.putString("staticIP", ip);
        wifi_changes_made = true;
        Serial.println("Static IP updated: " + ip);
      } else if (new_ip == "" && ip != "") {
        // Clear static IP if field is empty
        ip = "";
        wifiPrefs.remove("staticIP");
        wifi_changes_made = true;
        Serial.println("Static IP cleared");
      }
    }
    
    // Handle each Modbus address, data type, and byte order independently
    for (int i = 0; i < 5; i++) {
      // Handle register address
      String argName = "addr" + String(i+1);
      if (webServer.hasArg(argName)) {
        String new_address = urlDecode(webServer.arg(argName));
        String key = "Address_" + String(i+1);
        
        if (new_address != "" && new_address != Add_flash[i]) {
          // Update with new non-empty value
          Add_flash[i] = new_address;
          registerPrefs.putString(key.c_str(), Add_flash[i]);
          register_changes_made = true;
          Serial.println("Address " + String(i+1) + " updated: " + Add_flash[i]);
          MHeaders[i] = Add_flash[i].toInt();
        } 
      }
      
      // Handle data type
      String datatypeArg = "datatype" + String(i+1);
      if (webServer.hasArg(datatypeArg)) {
        String new_datatype = webServer.arg(datatypeArg);
        String datatypeKey = "DataType_" + String(i+1);
        
        if (new_datatype != DataType_flash[i]) {
          DataType_flash[i] = new_datatype;
          registerPrefs.putString(datatypeKey.c_str(), DataType_flash[i]);
          register_changes_made = true;
          Serial.println("Data type " + String(i+1) + " updated: " + DataType_flash[i]);
        }
      }
      
      // Handle byte order
      String byteorderArg = "byteorder" + String(i+1);
      if (webServer.hasArg(byteorderArg)) {
        String new_byteorder = webServer.arg(byteorderArg);
        String byteorderKey = "ByteOrder_" + String(i+1);
        
        if (new_byteorder != ByteOrder_flash[i]) {
          ByteOrder_flash[i] = new_byteorder;
          registerPrefs.putString(byteorderKey.c_str(), ByteOrder_flash[i]);
          register_changes_made = true;
          Serial.println("Byte order " + String(i+1) + " updated: " + ByteOrder_flash[i]);
        }
      }
    }
    
    // Set flash flag to indicate data is saved (only if any changes made)
      if (wifi_changes_made) {
         wifiPrefs.putString("wifi_saved", "saved");
      }
      if (register_changes_made) {
        registerPrefs.putString("register_saved", "saved");
      }
      Serial.println("Configuration changes saved to flash");
    
    // Response HTML
    String html = "<!DOCTYPE html><html><head>";
    html += "<title>Configuration Update</title>";
    html += "<meta http-equiv='refresh' content='3;url=/'>";
    html += "<style>";
    html += "body { font-family: Arial, sans-serif; text-align: center; padding: 50px; background-color: #f0f0f0; }";
    html += ".container { max-width: 400px; margin: 0 auto; background: white; padding: 30px; border-radius: 10px; box-shadow: 0 0 10px rgba(0,0,0,0.1); }";
    html += "h1 { color: #4CAF50; }";
    html += ".no-change { color: #ffc107; }";
    html += "</style></head><body>";
    html += "<div class='container'>";
    
    if (wifi_changes_made || register_changes_made) {
      html += "<h1>Configuration Updated!</h1>";
      html += "<p>The device will restart and apply the new settings.</p>";
      html += "<p>Redirecting in 3 seconds...</p>";
    } else {
      html += "<h1 class='no-change'>No Changes Detected</h1>";
      html += "<p>No configuration changes were made.</p>";
      html += "<p>Redirecting in 3 seconds...</p>";
    }
    
    html += "</div></body></html>";
    
    webServer.send(200, "text/html", html);
    
    // Only restart if changes were made
    if (wifi_changes_made || register_changes_made) {
      delay(2000);
      ESP.restart();
    }
    
  } else {
    webServer.send(400, "text/plain", "No form data received");
  }
}

// JSON API endpoint for current readings
void handleAPI() {
  String json = "{";
  json += "\"voltage\":" + String(dataStruct[0].data[0], 2) + ",";
  json += "\"current\":" + String(dataStruct[0].data[1], 2) + ",";
  json += "\"power_factor\":" + String(dataStruct[0].data[2], 2) + ",";
  json += "\"kwh\":" + String(dataStruct[0].data[3], 2) + ",";
  json += "\"kva\":" + String(dataStruct[0].data[4], 2) + ",";
  json += "\"timestamp\":" + String(millis()) + ",";
  json += "\"device_id\":\"" + WiFi.macAddress() + "\"";
  json += "}";
  
  webServer.sendHeader("Access-Control-Allow-Origin", "*");
  webServer.send(200, "application/json", json);
}

//////////////////////////  setup function
void setup() 
{
  pinMode(DE_RE_pin, OUTPUT);
  pinMode(ledpin, OUTPUT);
  pinMode(resetPin, INPUT_PULLUP);
  
  digitalWrite(resetPin, HIGH);
  digitalWrite(ledpin, LOW);

  Serial.begin(115200);
  Serial2.begin(tx_buadrate, SERIAL_8E1, RXD2, TXD2);
  Serial.printf("tx_buadrate, SERIAL_8E1, RXD2, TXD2");
  
  wifiPrefs.begin("wifi_config", false);
  registerPrefs.begin("register_config", false);

  String wifi_flag = wifiPrefs.getString("wifi_saved", "");
  String register_flag = registerPrefs.getString("register_saved", "");

  // Load register configuration INDEPENDENTLY
  if (register_flag != "") 
  {
    // Load register addresses
    Add_flash[0] = registerPrefs.getString("Address_1", "500");
    Add_flash[1] = registerPrefs.getString("Address_2", "502");
    Add_flash[2] = registerPrefs.getString("Address_3", "504");
    Add_flash[3] = registerPrefs.getString("Address_4", "506");
    Add_flash[4] = registerPrefs.getString("Address_5", "508");

    // Load data types (default to float)
    DataType_flash[0] = registerPrefs.getString("DataType_1", "float");
    DataType_flash[1] = registerPrefs.getString("DataType_2", "float");
    DataType_flash[2] = registerPrefs.getString("DataType_3", "float");
    DataType_flash[3] = registerPrefs.getString("DataType_4", "float");
    DataType_flash[4] = registerPrefs.getString("DataType_5", "float");

     // Load byte orders (default to MSB)
    ByteOrder_flash[0] = registerPrefs.getString("ByteOrder_1", "msb");
    ByteOrder_flash[1] = registerPrefs.getString("ByteOrder_2", "msb");
    ByteOrder_flash[2] = registerPrefs.getString("ByteOrder_3", "msb");
    ByteOrder_flash[3] = registerPrefs.getString("ByteOrder_4", "msb");
    ByteOrder_flash[4] = registerPrefs.getString("ByteOrder_5", "msb");
  } else {
    // Set defaults for register configuration
    Add_flash[0] = "500"; Add_flash[1] = "502"; Add_flash[2] = "504"; Add_flash[3] = "506"; Add_flash[4] = "508";
    DataType_flash[0] = "float"; DataType_flash[1] = "float"; DataType_flash[2] = "float"; DataType_flash[3] = "float"; DataType_flash[4] = "float";
    ByteOrder_flash[0] = "msb"; ByteOrder_flash[1] = "msb"; ByteOrder_flash[2] = "msb"; ByteOrder_flash[3] = "msb"; ByteOrder_flash[4] = "msb";
  }

  // Initialize MHeaders array for all cases
  for (int d = 0; d < 5; d++) {
    MHeaders[d] = Add_flash[d].toInt();
  }

  // Load WiFi configuration
  if (wifi_flag != "") {
    ssid = wifiPrefs.getString("ssid", "");
    password = wifiPrefs.getString("password", "");
    ip = wifiPrefs.getString("staticIP", "");
  }

  Serial.println("Configuration loaded from flash:");
  Serial.println("SSID: " + ssid);
  Serial.println("Static IP: " + ip);
  Serial.println("Register configurations:");
  for (int d = 0; d < 5; d++) {
    Serial.println("Register " + String(d+1) + ": Address=" + Add_flash[d] + 
                   ", DataType=" + DataType_flash[d] + 
                   ", ByteOrder=" + ByteOrder_flash[d]);
  }
  
  if(ip != "") 
  {
    Serial.println("Configuring static IP: " + ip);
    convert(ip);
    IPAddress local_IP(a, b, c, d);
    Serial.println("IP configuration: " + local_IP.toString());
    
    if (!WiFi.config(local_IP, gateway, subnet)) {
      Serial.println("Static IP configuration failed");
    }
  }
  
  if (ssid != "" && password != "") 
  {
    Serial.println("Connecting to WiFi: " + ssid);

    WiFi.begin(ssid.c_str(), password.c_str());
    while (WiFi.status() != WL_CONNECTED) {
      Serial.println("Connecting...");
      digitalWrite(ledpin, !digitalRead(ledpin)); // Blink LED while connecting
      delay(1000);
      retry_count++;
      
      if (retry_count > 20) {
        Serial.println("Connection timeout, restarting...");
        ESP.restart();
      }
      
      // Check for reset button during connection
      unsigned long currentMillis = millis();
      buttonState = digitalRead(resetPin);
      if (buttonState == LOW && programState == 0) {
        buttonMillis = currentMillis;
        programState = 1;
      } else if (programState == 1 && buttonState == HIGH) {
        programState = 0;
      }
      if (((currentMillis - buttonMillis) > 3000) && programState == 1) {
        programState = 0;
        Serial.println("WiFi reset during connection - clearing WiFi credentials");
        wifiPrefs.clear();
        ESP.restart();
      }
    }
    
    Serial.println("WiFi connected successfully");
    Serial.println("IP address: " + WiFi.localIP().toString());
    digitalWrite(ledpin, HIGH); // Solid LED when connected
    
    MODE = 1;
    Serial.println("MAC Address: " + WiFi.macAddress());
  }
  else { 
    MODE = 0;
    Serial.println("No WiFi credentials found, starting AP mode");
    WiFi.softAP(ssid_h, password_h);
    IPAddress IP = WiFi.softAPIP();
    Serial.println("AP IP address: " + IP.toString());
    digitalWrite(ledpin, LOW); // LED off in AP mode
  }

  // Setup web server routes
  webServer.on("/", handleLogin);
  webServer.on("/login", handleLogin);
  webServer.on("/readdata", handle_readdata);
  webServer.on("/getdata", handle_getdata);
  webServer.on("/save", HTTP_POST, handleSave);
  webServer.on("/api/readings", handleAPI);
  
  // Setup OTA
  setupOTA();
  
  webServer.begin();
  Serial.println("Web server started successfully");
  
  // Initialize first transmission
  Transmitidx = 0;
  Maddr = MHeaders[Transmitidx];
  //Transmit(Maddr);  // Uncomment to start automatic reading
}

//////////////////////////  loop function
void loop() {
  webServer.handleClient();
  
  // Handle reset button
  unsigned long currentMillis = millis();
  buttonState = digitalRead(resetPin);
  
  if (buttonState == LOW && programState == 0) {
    buttonMillis = currentMillis;
    programState = 1;
  } else if (programState == 1 && buttonState == HIGH) {
    programState = 0;
  }
  
  if (((currentMillis - buttonMillis) > 3000) && programState == 1) {
    programState = 0;
    Serial.println("WiFi reset during connection - clearing WiFi credentials");
    wifiPrefs.clear();
    wifiPrefs.end();
    ESP.restart();
  }

  // WiFi reconnection logic (only for STA mode)
  if (MODE == 1) {
    // Check WiFi connection periodically
    if (currentMillis - lastWiFiCheck > wifiCheckInterval) {
      lastWiFiCheck = currentMillis;
      
      if (WiFi.status() != WL_CONNECTED) {
        if (!isReconnecting) {
          Serial.println("WiFi connection lost! Attempting to reconnect...");
          isReconnecting = true;
          reconnectAttempts = 0;
          digitalWrite(ledpin, LOW); // Turn off LED when disconnected
        }
        
        // Try to reconnect
        if (reconnectAttempts < 20) {
          reconnectAttempts++;
          Serial.print("Reconnection attempt ");
          Serial.print(reconnectAttempts);
          Serial.println(" of 20");
          
          WiFi.disconnect();
          delay(100);
          WiFi.begin(ssid.c_str(), password.c_str());
          
          // Wait up to 10 seconds for connection
          int waitCount = 0;
          while (WiFi.status() != WL_CONNECTED && waitCount < 10) {
            delay(1000);
            waitCount++;
            digitalWrite(ledpin, !digitalRead(ledpin)); // Blink LED while reconnecting
            
            // Check for reset button during reconnection
            buttonState = digitalRead(resetPin);
            if (buttonState == LOW && programState == 0) {
              buttonMillis = millis();
              programState = 1;
            } else if (programState == 1 && buttonState == HIGH) {
              programState = 0;
            }
            if (((millis() - buttonMillis) > 3000) && programState == 1) {
              programState = 0;
              Serial.println("WiFi reset during connection - clearing WiFi credentials");
              wifiPrefs.clear();
              wifiPrefs.end();
              ESP.restart();
            }
          }
          
          if (WiFi.status() == WL_CONNECTED) {
            Serial.println("Reconnected to WiFi!");
            Serial.print("IP address: ");
            Serial.println(WiFi.localIP());
            isReconnecting = false;
            reconnectAttempts = 0;
            digitalWrite(ledpin, HIGH); // Solid LED when connected
          }
        } else {
          // After 20 failed attempts, restart ESP32
          Serial.println("Failed to reconnect after 20 attempts. Restarting ESP32...");
          ESP.restart();
        }
      } else {
        // WiFi is connected
        if (isReconnecting) {
          isReconnecting = false;
          reconnectAttempts = 0;
        }
        digitalWrite(ledpin, HIGH); // Solid LED when connected
      }
    }
  }
  
  // Handle Modbus communication
  if (Serial2.available()) {
    Serial.println("Modbus data available");
    
    int byteCount = 0;
    while (Serial2.available() && byteCount < 15) {
      RxData[byteCount] = Serial2.read();
      byteCount++;
      delay(2);
    }
    
    Serial.println("Received " + String(byteCount) + " bytes:");
    for (int i = 0; i < byteCount; i++) {
      Serial.print("0x");
      if (RxData[i] < 16) Serial.print("0");
      Serial.print(RxData[i], HEX);
      Serial.print(" ");
    }
    Serial.println();
    
    if (byteCount >= 7) {
      readRegs(RxData, 0, numRegs);
    }
  }
  
  // Periodic data refresh (uncomment if you want automatic reading every 5 seconds)
  // static unsigned long lastRefresh = 0;
  // if (currentMillis - lastRefresh > 5000) {
  //   lastRefresh = currentMillis;
  //   if (Transmitidx == 0) {
  //     Maddr = MHeaders[Transmitidx];
  //     Transmit(Maddr);
  //   }
  // }
  
  // Status LED management
  if (MODE == 1) {
    // Connected to WiFi - solid LED (unless reconnecting)
    if (!isReconnecting) {
      digitalWrite(ledpin, HIGH);
    }
  } else {
    // AP mode - slow blink
    if ((currentMillis / 1000) % 2 == 0) {
      digitalWrite(ledpin, HIGH);
    } else {
      digitalWrite(ledpin, LOW);
    }
  }
  
  delay(10);
}