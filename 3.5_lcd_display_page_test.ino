
#include <Adafruit_GFX.h>
#include <MCUFRIEND_kbv.h>
#include <TouchScreen.h>
#include <HardwareSerial.h>
#include <avr/wdt.h>

#define MINPRESSURE 200
#define MAXPRESSURE 1000

MCUFRIEND_kbv tft;
const int XP = 8, XM = A2, YP = A3, YM = 9;
const int TS_LEFT = 937, TS_RT = 165, TS_TOP = 976, TS_BOT = 159;
TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

#define BLACK   0x0000
#define WHITE   0xFFFF
#define PURPLE  0x780F
#define RED     0xF800
#define GREEN   0x07E0
#define BLUE    0x001F

Adafruit_GFX_Button reset_btn, wifi_btn, back_btn, help_btn;
bool page = 0; //0 = 1st page
int pixel_x, pixel_y;
unsigned long lastPressedTime = 0;
const unsigned long debounceDelay = 200;

String receivedData= "";
uint8_t r1, r2;
uint32_t count = 0, prev_count = 0, temp = 0;
   int circleX = 250;  // X position
    int circleY = 45;   // Y position
    int radius = 30;    // Circle radius

bool Touch_getXY(void) {
    TSPoint p = ts.getPoint();
    pinMode(YP, OUTPUT);
    pinMode(XM, OUTPUT);
    digitalWrite(YP, HIGH);
    digitalWrite(XM, HIGH);
    bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    
    if (pressed) {
        pixel_x = map(p.y, TS_LEFT, TS_RT, 0, tft.width());
        //pixel_y = map(p.x, TS_TOP, TS_BOT, 0, tft.height() - 30);
        pixel_y = map(p.x, TS_TOP, TS_BOT, 0, tft.height());
    }
    return pressed;
}

void drawCount() {
    tft.setTextSize(10);
    tft.setTextColor(RED, WHITE);

    // Calculate text width dynamically
    int numDigits = count == 0 ? 1 : log10(count) + 1;
    int textWidth = numDigits * 36;  // Adjust character width as needed

    // Clear only the required area
    tft.fillRect(320 - textWidth, 150, textWidth, 80, WHITE);
    
    // Print count at the rightmost position
    tft.setCursor(320 - textWidth, 150);
    tft.print(count);
}

void updateCircle() {
    uint16_t fillColor = (r1 == 10) ? GREEN : RED;
    String statusText = (r1 == 10) ? "ON" : "OFF";
    uint16_t textColor = (r1 == 10) ? BLACK : WHITE;

    // Redraw circle
    tft.fillCircle(circleX, circleY, radius, fillColor);
    tft.drawCircle(circleX, circleY, radius, BLUE);

    // Print ON/OFF inside the circle
    tft.setTextSize(2);
    tft.setTextColor(textColor);
    tft.setCursor(circleX - 12, circleY - 6);
    tft.print(statusText);
}


void drawFirstPage() 
{
    tft.fillScreen(WHITE);

    // Draw R1 square  
    // tft.fillRect(60, 50, 60, 60, RED);   // Filled red square  
    // tft.drawRect(60, 50, 60, 60, BLUE);  // Blue border  
    tft.setTextColor(BLACK);  
    tft.setTextSize(3);  
    tft.setCursor(80, 40);  // Centered text
    tft.print("STATUS");  

    // // Draw a Green Circle instead of R1
    // tft.fillCircle(circleX, circleY, radius, GREEN); // Filled green circle
    // tft.drawCircle(circleX, circleY, radius, BLUE);  // Blue border

    // // Display "ON" inside the circle
    // tft.setTextSize(2);  // Adjust text size to fit inside the circle
    // tft.setTextColor(WHITE);  // White text for contrast
    // tft.setCursor(circleX - 12, circleY - 6);  // Center text inside circle
    // tft.print("OFF");
        updateCircle(); // Call function to update circle based on `r1`

    tft.setTextSize(6);
    tft.setTextColor(BLACK);
    tft.setCursor(220, 95);
    tft.print("COUNT");

    drawCount();

    reset_btn.initButton(&tft, 160, 290, 80, 40, PURPLE, BLUE, WHITE, "Reset", 2);
    help_btn.initButton(&tft, 320, 290, 80, 40, RED, BLUE, WHITE, "Help", 2);

    reset_btn.drawButton(false);
    help_btn.drawButton(false);
}


void drawWiFiPage() 
{
    tft.fillScreen(WHITE);
    tft.setTextColor(BLACK, WHITE);
    tft.setTextSize(4);
    
    tft.setCursor(50, 70);
    tft.print(" Connect to WiFi"); 
    tft.setCursor(70, 120);
    tft.print(" LCD_COUNTER");
    tft.setCursor(90, 180);
    tft.setTextSize(3);
    tft.print("192.168.4.1/login");
    
    back_btn.initButton(&tft, 120, 270, 100, 50, GREEN, BLUE, WHITE, "Back", 2);
    back_btn.drawButton(false);
    wifi_btn.initButton(&tft, 340, 270, 100, 50, GREEN, BLUE, WHITE, "WiFi", 2);
    wifi_btn.drawButton(false);
}


void parseData(String data) {
    int pIndex = data.indexOf("P:");
    int r1Index = data.indexOf("R1:");
    int r2Index = data.indexOf("R2:");

    if (pIndex != -1 && r1Index != -1 && r2Index != -1) {
        count = data.substring(pIndex + 2, r1Index).toInt();
        r1 = data.substring(r1Index + 3, r2Index).toInt();
        r2 = data.substring(r2Index + 3).toInt();
    }
}

void setup() {
    Serial.begin(9600);
    //espSerial.begin(9600);
    uint16_t ID = tft.readID();
    if (ID == 0xD3D3) ID = 0x9486;
    tft.begin(ID);
    tft.setRotation(1);
    drawFirstPage();
}

void loop() 
{
    while (Serial.available())
    {
       char c = Serial.read();
       receivedData += c;
       if (c == '\n') { // End of transmission 
          parseData(receivedData);
          receivedData = ""; // Clear buffer
       }
    }
    bool down = Touch_getXY();
    unsigned long currentMillis = millis();
    
    if (currentMillis - lastPressedTime >= debounceDelay) {
        lastPressedTime = currentMillis;
        
        if (page == 0) 
        {
            reset_btn.press(down && reset_btn.contains(pixel_x, pixel_y));

            help_btn.press(down && help_btn.contains(pixel_x, pixel_y));

            if (reset_btn.justPressed()) {
                reset_btn.drawButton(true);
                Serial.println("reset");
                Serial.flush();
                 delay(10);
                  wdt_enable(WDTO_15MS); // Enable watchdog with 15ms timeout
                  while(1);  // Enter infinite loop to trigger reset
                  count = 0; 
                  prev_count = 0;
                  tft.fillScreen(WHITE);
                  drawFirstPage();  
            }
            if (reset_btn.justReleased()) reset_btn.drawButton(false);

        
            
            if (help_btn.justPressed()) {
                help_btn.drawButton(true);
                  page = 1;
                 tft.fillScreen(WHITE);  // Clear screen before switching pages
                drawWiFiPage(); 
                return; // Prevent further execution
            }
            if (help_btn.justReleased()) help_btn.drawButton(false);

             if (prev_count != count) {
                    drawCount();
                    prev_count = count;
             }
             static uint8_t prev_r1 = 255; // Store previous r1 value
if (r1 != prev_r1) {
    updateCircle(); // Update the circle only if `r1` changes
    prev_r1 = r1;
}
    //          if(r1 == 10)
    //          {
    // // Draw a Green Circle instead of R2
    // tft.fillCircle(circleX, circleY, radius, GREEN); // Filled green circle
    // tft.drawCircle(circleX, circleY, radius, BLUE);  // Blue border 
    //     // Display "ON" inside the circle
    // tft.setTextSize(2);  // Adjust text size to fit inside the circle
    // tft.setTextColor(BLACK);  // White text for contrast
    // tft.setCursor(circleX - 12, circleY - 6);  // Center text inside circle
    // tft.print("ON");
    //          }
    //          else
    //          {
    // // Draw a Green Circle instead of R2
 
    // tft.fillCircle(circleX, circleY, radius, RED); // Filled green circle
    // tft.drawCircle(circleX, circleY, radius, BLUE);  // Blue border
    //     // Display "ON" inside the circle
    // tft.setTextSize(2);  // Adjust text size to fit inside the circle
    // tft.setTextColor(WHITE);  // White text for contrast
    // tft.setCursor(circleX - 12, circleY - 6);  // Center text inside circle
    // tft.print("OFF");
    //          }

        }
        else // WIFI PAGE
        {  
            back_btn.press(down && back_btn.contains(pixel_x, pixel_y));
            if (back_btn.justPressed()) {
                back_btn.drawButton(true);
                page = 0;  // Go back to the main page
                drawFirstPage();
            }
            if (back_btn.justReleased()) back_btn.drawButton(false);

            wifi_btn.press(down && wifi_btn.contains(pixel_x, pixel_y));
            if (wifi_btn.justPressed()) {
                wifi_btn.drawButton(true);
                Serial.println("wifi");
                Serial.flush();
            }
            if (wifi_btn.justReleased()) wifi_btn.drawButton(false);

        }
    }
}


// #include <Adafruit_GFX.h>
// #include <MCUFRIEND_kbv.h>
// #include <TouchScreen.h>
// #include <SoftwareSerial.h>
// #include <avr/wdt.h>

// #define MINPRESSURE 200
// #define MAXPRESSURE 1000

// MCUFRIEND_kbv tft;
// const int XP = 8, XM = A2, YP = A3, YM = 9;
// const int TS_LEFT = 937, TS_RT = 165, TS_TOP = 976, TS_BOT = 159;
// TouchScreen ts = TouchScreen(XP, YP, XM, YM, 300);

// #define BLACK   0x0000
// #define WHITE   0xFFFF
// #define PURPLE  0x780F
// #define RED     0xF800
// #define GREEN   0x07E0
// #define BLUE    0x001F

// Adafruit_GFX_Button reset_btn, wifi_btn;
// bool page = 0;
// int pixel_x, pixel_y;
// unsigned long lastPressedTime = 0;
// const unsigned long debounceDelay = 200;

// SoftwareSerial espSerial(10, 11);//rx tx
// String receivedData= "";
// uint8_t r1, r2;
// uint8_t rxdata[4]= {0}, txdata[4] = {0};
// uint32_t count = 0, prev_count = 0, temp = 0;
// bool wifiConnected = false;

// bool Touch_getXY(void) {
//     TSPoint p = ts.getPoint();
//     pinMode(YP, OUTPUT);
//     pinMode(XM, OUTPUT);
//     digitalWrite(YP, HIGH);
//     digitalWrite(XM, HIGH);
//     bool pressed = (p.z > MINPRESSURE && p.z < MAXPRESSURE);
    
//     if (pressed) {
//         pixel_x = map(p.y, TS_LEFT, TS_RT, 0, tft.width());
//         pixel_y = map(p.x, TS_TOP, TS_BOT, 0, tft.height() - 30);
//     }
//     return pressed;
// }

// void drawCount() {
//     tft.setTextSize(10);
//     tft.setTextColor(RED, WHITE);

//     // Calculate text width dynamically
//     int numDigits = count == 0 ? 1 : log10(count) + 1;
//     int textWidth = numDigits * 36;  // Adjust character width as needed

//     // Clear only the required area
//     tft.fillRect(320 - textWidth, 150, textWidth, 80, WHITE);
    
//     // Print count at the rightmost position
//     tft.setCursor(320 - textWidth, 150);
//     tft.print(count);
// }

// void drawFirstPage() 
// {
//     tft.fillScreen(WHITE);
//   // Draw R1 rectangle  
//     tft.fillRect(10, 70, 80, 40, RED);   // Filled blue rectangle  
//     tft.drawRect(10, 70, 80, 40, BLUE);    // Red border  
//     tft.setTextColor(WHITE);  
//     tft.setTextSize(2);  
//     tft.setCursor(35, 85);  
//     tft.print("R1");  

//     // Draw R2 rectangle  
//     tft.fillRect(100, 70, 80, 40, RED);  // Filled blue rectangle  
//     tft.drawRect(100, 70, 80, 40, BLUE);   // Red border  
//     tft.setCursor(125, 85);  
//     tft.print("R2"); 

//     tft.setTextSize(6);
//     tft.setTextColor(BLACK);
//     tft.setCursor(220, 70);
//     tft.print("COUNT");

//     drawCount();

//     reset_btn.initButton(&tft, 100, 290, 80, 40, PURPLE, BLUE, WHITE, "Reset", 2);
//     wifi_btn.initButton(&tft, 340, 290, 80, 40, wifiConnected ? GREEN : RED, BLUE, WHITE, "WiFi", 2);

//     reset_btn.drawButton(false);
//     wifi_btn.drawButton(false);
// }


// void parseData(String data) {
//     int pIndex = data.indexOf("P:");
//     int r1Index = data.indexOf("R1:");
//     int r2Index = data.indexOf("R2:");

//     if (pIndex != -1 && r1Index != -1 && r2Index != -1) {
//         count = data.substring(pIndex + 2, r1Index).toInt();
//         r1 = data.substring(r1Index + 3, r2Index).toInt();
//         r2 = data.substring(r2Index + 3).toInt();

//         Serial.print("Pulse Count: ");
//         Serial.println(count);
//         Serial.print("R1: ");
//         Serial.println(r1);
//         Serial.print("R2: ");
//         Serial.println(r2);
//     }
// }

// void setup() {
//     Serial.begin(9600);
//     espSerial.begin(9600);
//     uint16_t ID = tft.readID();
//     if (ID == 0xD3D3) ID = 0x9486;
//     tft.begin(ID);
//     tft.setRotation(1);
//     drawFirstPage();
// }

// void loop() 
// {
//     bool down = Touch_getXY();
//     unsigned long currentMillis = millis();
    
//     if (currentMillis - lastPressedTime >= debounceDelay) {
//         lastPressedTime = currentMillis;
        
//         if (page == 0) {
//             reset_btn.press(down && reset_btn.contains(pixel_x, pixel_y));
//             wifi_btn.press(down && wifi_btn.contains(pixel_x, pixel_y));

//             if (reset_btn.justPressed()) {
//                 reset_btn.drawButton(true);
//                 espSerial.println("reset");
//                 espSerial.flush();
//                  delay(10);
//                   wdt_enable(WDTO_15MS); // Enable watchdog with 15ms timeout
//                   while(1);  // Enter infinite loop to trigger reset
//                   count = 0; 
//                   prev_count = 0;
//                   tft.fillScreen(WHITE);
//                   drawFirstPage();  
//                 Serial.println("Count Reset");
//             }
//             if (reset_btn.justReleased()) reset_btn.drawButton(false);

//             if (wifi_btn.justPressed()) {
//                 wifi_btn.drawButton(true);
//                 espSerial.println("wifi");
//                 espSerial.flush();
//                 Serial.println("WiFi message sent to ESP32");
//             }
//             if (wifi_btn.justReleased()) wifi_btn.drawButton(false);


//             while (espSerial.available())
//            {
//              char c = espSerial.read();
//              receivedData += c;
//              if (c == '\n') { // End of transmission
//                    parseData(receivedData);
//                    receivedData = ""; // Clear buffer
//              }
//                 if (prev_count != count) {
//                     drawCount();
//                     prev_count = count;
//                 }
//            }
//            if(r1 == 10)
//            {
//                 r1=0;
//                 tft.fillRect(10, 70, 80, 40, GREEN);   // Filled blue rectangle  
//                 tft.drawRect(10, 70, 80, 40, BLUE);    // Red border  
//                 tft.setTextColor(WHITE);  
//                 tft.setTextSize(2);  
//                 tft.setCursor(35, 85);  
//                 tft.print("R1");  
//            }
//            if(r2 == 10)
//            {
//             r2 =0;
//                 // Draw R2 rectangle  
//                  tft.fillRect(100, 70, 80, 40, GREEN);  // Filled blue rectangle  
//                  tft.drawRect(100, 70, 80, 40, BLUE);   // Red border  
//                  tft.setCursor(125, 85);  
//                  tft.print("R2"); 
//            }
//          }
//     }
// }
