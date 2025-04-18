
#include<HardwareSerial.h>
#include <Wire.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#include <Adafruit_MLX90614.h>
Adafruit_MLX90614 mlx = Adafruit_MLX90614();

#define I2C_SDA 33
#define I2C_SCL 32
#define SDA_1 27
#define SCL_1 26

#define SEALEVELPRESSURE_HPA (1013.25)
TwoWire I2CBME = TwoWire(0);
TwoWire I2Ctwo = TwoWire(1);
Adafruit_BME280 bme;

const int DOUT_PIN = 12;
const int SCK_PIN = 14;
const int calib = 34;  // Set your reset button pin

unsigned long delayTime;
int offset = -56693 , kg_weight = 39270, calib_weight = 1000;
int gain = 128, GAIN = 1;
int CMode = 0;
int programState = 0,buttonState, c = 0;
long buttonMillis = 0;
float grams_fun;

// Define structures for reading data into.
signed long num = 0;

uint8_t data[3] = { 0 };
uint8_t filler = 0x00;
uint8_t tx_data[9];
float in_kg;

union FloatToBytes {
  float value;
  uint8_t bytes[4];
};
FloatToBytes converter;

///////////      shifting function
uint8_t shiftInSlow(uint8_t bitOrder)
{
    uint8_t value = 0;
    uint8_t i;

    for(i = 0; i < 8; ++i)
    {
       digitalWrite(SCK_PIN, HIGH);  //sck high
       delayMicroseconds(1);
        if(bitOrder == LSBFIRST)
        {
            value |= digitalRead(DOUT_PIN) << i; //dout reading
            // Serial.println("lsb bits");
            // Serial.println(value);
        }
        else
        {
            value |= digitalRead(DOUT_PIN) << (7 - i);     //dout reading
            // Serial.println("msb bits");
            // Serial.println(value);
        }
        digitalWrite(SCK_PIN, LOW); //sck low
        delayMicroseconds(1);
    }
    return value;
}

///////////      read function
signed long read()
{
//	 Define structures for reading data into.

		// Pulse the clock pin 24 times to read the data.
			data[2] = shiftInSlow( MSBFIRST);
			data[1] = shiftInSlow( MSBFIRST);
			data[0] = shiftInSlow( MSBFIRST);

			// Set the channel and the gain factor for the next reading using the clock pin.
			for (unsigned int i = 0; i < GAIN; i++)
			{
				digitalWrite(SCK_PIN, HIGH);
				delayMicroseconds(1);
				digitalWrite(SCK_PIN, LOW);
				delayMicroseconds(1);
			}

			// Replicate the most significant bit to pad out a 32-bit signed integer
				if (data[2] & 0x80) 
        {
					filler = 0xFF;
				} 
        else 
        {
					filler =0; // = 0x00;
				}

				// Construct a 32-bit signed integer
			//	num = ((unsigned long)filler << 24) | ((unsigned long)data[2] << 16) | ((unsigned long)data[1] << 8) | (unsigned long)data[0];
         num = ((signed long)filler << 24) | ((signed long)data[2] << 16) | ((signed long)data[1] << 8) | (signed long)data[0];
				return (num);
}

///////////      average function
long read_average(uint8_t times)
{
	long sum = 0;
	for (uint8_t i = 0; i < times; i++) 
  {
		sum += read();
		// Probably will do no harm on AVR but will feed the Watchdog Timer (WDT) on ESP.
		// https://github.com/bogde/HX711/issues/73
	  //delay(4);
	}
	return sum / times;
}
///////////////  weight function
float weight_fun(int update, int off, int kg_wei)
{
  
  float res = (-off + update)*calib_weight/kg_wei;
  return res;
}



void printValues() 
{
  Serial.print("Temperature = ");
  Serial.print(bme.readTemperature());
  Serial.println(" *C");
  
  // Convert temperature to Fahrenheit
  /*Serial.print("Temperature = ");
  Serial.print(1.8 * bme.readTemperature() + 32);
  Serial.println(" *F");*/
  
  Serial.print("Pressure = ");
  Serial.print(bme.readPressure() / 100.0F);
  Serial.println(" hPa");

  Serial.print("Approx. Altitude = ");
  Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA));
  Serial.println(" m");

  Serial.print("Humidity = ");
  Serial.print(bme.readHumidity());
  Serial.println(" %");

  Serial.println();
}


void mlx_data()
{
  Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempC()); 
  Serial.print("*C\tObject = "); Serial.print(mlx.readObjectTempC()); Serial.println("*C");
  Serial.print("Ambient = "); Serial.print(mlx.readAmbientTempF()); 
  Serial.print("*F\tObject = "); Serial.print(mlx.readObjectTempF()); Serial.println("*F");
  Serial.println();
  delay(1000);
}
void setup() 
{
  // put your setup code here, to run once:
  Serial.begin(115200);

  pinMode(DOUT_PIN, INPUT);
  pinMode(SCK_PIN, OUTPUT);

  pinMode(calib, INPUT_PULLUP);  // Set the reset pin as input with internal pull-up resistor(default state is high)
  //digitalWrite(calib, HIGH);
  I2CBME.begin(I2C_SDA, I2C_SCL, 100000);
  I2Ctwo.begin(SDA_1, SCL_1, 100000);
  Serial.println("Adafruit MLX90614 test");  
   mlx.begin(0x5A, &I2Ctwo); 

  Serial.println(F("BME280 test"));
  I2CBME.begin(I2C_SDA, I2C_SCL, 100000);

  bool status;

  // default settings
  // (you can also pass in a Wire library object like &Wire2)
  status = bme.begin(0x76, &I2CBME);  
  if (!status) {
    Serial.println("Could not find a valid BME280 sensor, check wiring!");
   // while (1);
  }

  Serial.println("-- Default Test --");
  delayTime = 1000;

  Serial.println();
}

void loop() 
{  
  // put your main code here, to run repeatedly:
  printValues();
  delay(delayTime);

  mlx_data();
 /////////////  calibration function
  while(CMode)
  {

    Serial.println("Entering calibration mode, remove known weight");
    delay(500);
    //should wait on while
    if(digitalRead(DOUT_PIN) == 0)   // if dout low
	  {
		    offset = read_average(10);
        Serial.println("offset");
        Serial.println(offset);
    }   
    Serial. println("put known weight and enter value");
    while(Serial.available() < 1) 
    {}
    if(Serial.available())
    {
      calib_weight = Serial.parseInt();

        Serial.println("calib weight");
        Serial.println(calib_weight);
      if(digitalRead(DOUT_PIN) == 0)   // if dout low
	    {
		    kg_weight = read_average(10)-offset;
        Serial.println("weight");
        Serial.println(kg_weight);
        CMode = 0;
      }   
    }
    
  }

  /////////////////     weight knowing 
  if(digitalRead(DOUT_PIN) == 0)   // if dout low
  {
     long result = read_average(10);
      in_kg = weight_fun(result, offset, kg_weight); 
      converter.value = in_kg;
      Serial.print("weight: ");
      Serial.println((String)in_kg + " grams");
      
  }

  /////////////////calib button 
  unsigned long currentMillis = millis();
  buttonState = digitalRead(calib);
  if (buttonState == LOW && programState == 0) 
  {
    buttonMillis = currentMillis;
    programState = 1;
  } 
  else if (programState == 1 && buttonState == HIGH) 
  {
    programState = 0;  //reset
  }
  if (((currentMillis - buttonMillis) > 3000) && programState == 1) 
  {
    programState = 0;
    //ledMillis = currentMillis;
    Serial.println(" entering into calibration mode");
    CMode = 1;
  }
}







