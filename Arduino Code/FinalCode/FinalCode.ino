#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SH1106.h>
#include "MAX30105.h"
#include <SoftwareSerial.h>

#include "heartRate.h"
//////////////////////////////////////////////////////////////////////
MAX30105 particleSensor;

const byte RATE_SIZE = 4; //Increase this for more averaging. 4 is good.
byte rates[RATE_SIZE]; //Array of heart rates
byte rateSpot = 0;
long lastBeat = 0; //Time at which the last beat occurred

float beatsPerMinute;
int beatAvg;
//////////////////////////////////////////////////////////////////////
Adafruit_SH1106 display(-1);
//////////////////////////////////////////////////////////////////////
SoftwareSerial mySerial(3, 2); 
String Data_SMS;

void setup()   
{   

  Serial.begin(115200);
  mySerial.begin(9600);
  
  //////////////////////////////////////////////////////////////////////
  // initialize with the I2C addr 0x3C
  display.begin(SH1106_SWITCHCAPVCC, 0x3C);  // initialize with the I2C addr 0x3D (for the 128x64)
  // init done 

  // Clear the buffer.
  display.clearDisplay();

  display.setTextColor(WHITE);
  display.setCursor(0,24);
  display.setTextSize(2);

  //////////////////////////////////////////////////////////////////////

  if (!particleSensor.begin(Wire, I2C_SPEED_FAST)) //Use default I2C port, 400kHz speed
  {
    //Serial.println("MAX30105 was not found. Please check wiring/power. ");
    display.println("Heart Preasure device not found!");
    display.display();
    delay(2000);
    while (1);
  }  

  particleSensor.setup(); //Configure sensor with default settings
  particleSensor.setPulseAmplitudeRed(0x0A); //Turn Red LED to low to indicate sensor is running
  particleSensor.setPulseAmplitudeGreen(0); //Turn off Green LED
  particleSensor.enableDIETEMPRDY(); //Enable the temp ready interrupt. This is required.
  
  display.clearDisplay();
  
  display.setTextColor(WHITE);
  display.setTextSize(2);

  //////////////////////////////////////////////////////////////////////
  
  pinMode(10, INPUT); // Setup for leads off detection LO +
  pinMode(11, INPUT); // Setup for leads off detection LO -
  pinMode(7, OUTPUT);
  delay(1000);
}
void loop() {
  //////////////////////////////////////////////////////////////////////
  long irValue = particleSensor.getIR();

  if (checkForBeat(irValue) == true)
  {
    //We sensed a beat!
    long delta = millis() - lastBeat;
    lastBeat = millis();

    beatsPerMinute = 60 / (delta / 1000.0);

    if (beatsPerMinute < 255 && beatsPerMinute > 20)
    {
      rates[rateSpot++] = (byte)beatsPerMinute; //Store this reading in the array
      rateSpot %= RATE_SIZE; //Wrap variable

      //Take average of readings
      beatAvg = 0;
      for (byte x = 0 ; x < RATE_SIZE ; x++)
        beatAvg += rates[x];
      beatAvg /= RATE_SIZE;
    }
  }
  //////////////////////////////////////////////////////////////////////
  float temperature = particleSensor.readTemperature();
  
  //////////////////////////////////////////////////////////////////////
  //display.setCursor(2,24);

  Serial.print(analogRead(A0));
  Serial.print(","); 
  
  if (irValue < 50000){
    //display.println("No finger?"); // Display Comment
    Serial.print(0);
  }else{
    //display.print(beatAvg); // Display Comment
    //display.print(" BPM"); // Display Comment
    Serial.print(beatAvg);
  }

  //display.println(); // Display Comment
  //display.display(); // Display Comment
  //display.clearDisplay(); // Display Comment
  
  Serial.print(","); 
  Serial.print(temperature, 4);  
  Serial.println();

  //delay(10);

  if ( Serial.available() > 5 ){


    digitalWrite(7, HIGH);   // turn the LED on (HIGH is the voltage level)
    //delay(1000);                       // wait for a second
    //digitalWrite(7, LOW);    // turn the LED off by making the voltage LOW
    //delay(1000);

  Serial.println("Sending Data...");     //Displays on the serial monitor
  mySerial.print("AT+CMGF=1\r");          // Set the shield to SMS mode
  delay(100);
  mySerial.print("AT+CMGS=\"+94777066094\"\r");  //Your phone number don't forget to include your country code example +212xxxxxxxxx"
  delay(100);
  Data_SMS = "Message: " + Serial.readString();   //A string to regroup the whole message as it's composed of Strings and Float --> to a single string,
                                                                                      //Example: Temperature 23.1 C
                                                                                      //         Humidity 40 %
  mySerial.print(Data_SMS);  //This string is sent as SMS
  delay(100);
  mySerial.print((char)26);//(required according to the datasheet)
  //delay(100);
  mySerial.println();
  Serial.println("Data Sent.");
  delay(500);
     
 }else{
    digitalWrite(7, LOW);
 }
  
}



void updateSerial()
{
  delay(500);
  while (Serial.available()) 
  {
    mySerial.write(Serial.read());//Forward what Serial received to Software Serial Port
  }
  while(mySerial.available()) 
  {
    Serial.write(mySerial.read());//Forward what Software Serial received to Serial Port
  }
}
