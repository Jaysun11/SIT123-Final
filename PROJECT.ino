


//Libraries
#include "SD.h" //Writing to SD card
#include <Wire.h>
#include "RTClib.h" //used for data capture
#include <SoftwareSerial.h> // used for creating a second serial in/out
#include <DHT.h>//used for temp sensor

SoftwareSerial mySerial(2, 3); // defines bluetooth serian TX and RX to pins 2 and 3

const int MQ7Out = A0;//the AOUT pin of the MQ-7 Sensor
const int MQ7In = 8;//the Din pin of the MQ-7 Sensor
const int LEDPIN = 11;

const int BuzzerPin = 6; //pin forthe onboard buzzer
int safeThreshold = 150;//Safe threshold for Mq-7

#define tempSensorPin 9     // Temperature sensorpin
#define sensortype DHT22   // set sensortype to DHT 22  (AM2302)
DHT dht(tempSensorPin, sensortype); // Initialize DHT sensor for normal 16mhz Arduino


RTC_DS1307 RTC; // define the Real Time Clock object
const int sdCardPin = 10; //SD card pin

File outputFile;

float temperature; //Stores temperature value
float humidity; //Stores temperature value
int CarbonMonoxide; //Stores CO value

void setup() {
  mySerial.begin(9600); // Init baud of bluetooth serial
  Serial.begin(9600); //Init baud of arduino serial

  dht.begin();

  pinMode(BuzzerPin, OUTPUT);      // sets buzzer pin to an output
  pinMode(LEDPIN, OUTPUT);      // sets buzzer pin to an output
  pinMode(MQ7Out, INPUT);   //Sets the output of the MQ-7 pin to an input


   // initialize the SD card
  initSDcard();

  // create a new file
  createFile();

  initRTC();

  outputFile.println("millis,stamp,temperature, humidity, Carbon Monoxide");
}

void loop() {


  checkTempHum();

  checkCO();

  checkBT();

  broadcastValues();


}

void outputToSerial(){
    Serial.println("Checking Temperature");
    
    //Print temp and humidity values to serial monitor
    Serial.println(temperature);

    Serial.println("Checking Humidity");

    Serial.println(humidity);

    Serial.println("Checking CO");
  
    Serial.println(CarbonMonoxide);

  
}

void recordData() {
  DateTime currentTime;
  uint32_t timeSinceStart = millis();
  currentTime = RTC.now();
  outputFile.print(currentTime.unixtime()); // seconds since 2000
  outputFile.print(", ");
  outputFile.print(currentTime.year(), DEC);
  outputFile.print("/");
  outputFile.print(currentTime.month(), DEC);
  outputFile.print("/");
  outputFile.print(currentTime.day(), DEC);
  outputFile.print(" ");
  outputFile.print(currentTime.hour(), DEC);
  outputFile.print(":");
  outputFile.print(currentTime.minute(), DEC);
  outputFile.print(":");
  outputFile.print(currentTime.second(), DEC);
  outputFile.print(", ");
  outputFile.print(temperature);
  outputFile.print(", ");
  outputFile.print(humidity);
  outputFile.print(", ");
  outputFile.println(CarbonMonoxide);

  outputFile.flush();
  
}

void broadcastValues(){

  mySerial.write('T');
  mySerial.println(temperature);
  mySerial.write('H');
  mySerial.println(humidity);
  mySerial.write('C');
  mySerial.println(CarbonMonoxide);
  delay(2000);
  //wait two seconds second

  outputToSerial(); //use this to debug sensors

  recordData(); //records to SD
  //takes a second


}

void checkBT() {

//bluetooth transmission and reciept
  
  char transmission;
  char characterSent;

  
  if (mySerial.available()) { //if connected to bluetooth module
    transmission = mySerial.read();// Read from bluetooth serial and store in transmission
    delay(100);
    Serial.write(transmission); // Write to console recieved message

  }

  if (Serial.available()) {
    characterSent = Serial.read(); // Read character sent from serial (used to test sending messages through serial out
 
    // HM-10 Modules can't recieve N/L characters, so don't send them
    if (characterSent!=10 & characterSent!=13 ) 
      {  
        mySerial.write(characterSent); // Sent message to bluetooth
      }
    Serial.write(characterSent); //display sent message to console
  }

}

void checkTempHum(){

    temperature= dht.readTemperature();
    humidity= dht.readHumidity();

}

void checkCO(){

  CarbonMonoxide = analogRead(MQ7Out);

  if(CarbonMonoxide > safeThreshold) {
    alarm();
  } else {
    alarmOff();
  }
  
}

void alarm() {

  tone(BuzzerPin, 1000); // Send 1KHz sound signal...
  digitalWrite(LEDPIN, HIGH); //Flash LED
  delay(500);        // ...for 0.5 sec
  tone(BuzzerPin, 750); // Send 0.5KHz sound signal...
   digitalWrite(LEDPIN, LOW); //Flash LED
  delay(500);        // ...for 0.5 sec

}

void alarmOff() {

  noTone(BuzzerPin);
   digitalWrite(LEDPIN, LOW); //Flash LED

}

void initSDcard()
{
   //Code provided by SIT123 Unit Chair,  Original Author: Niroshinie Fernando, Modified by Jason Tubman
  Serial.print("Initializing SD card...");
  // make sure that the default chip select pin is set to
  // output, even if you don't use it:
  pinMode(10, OUTPUT);

  // see if the card is present and can be initialized:
  if (!SD.begin(sdCardPin)) {
    Serial.println("Card failed, or not present");
    // don't do anything more:
    return;
  }
  Serial.println("card initialized.");

}

void createFile()
{
  //Code provided by SIT123 Unit Chair,  Original Author: Niroshinie Fernando, Modified by Jason Tubman
  char filename[] = "COMO00.CSV";
  for (uint8_t i = 0; i < 100; i++) {
    filename[4] = i / 10 + '0';
    filename[5] = i % 10 + '0';
    if (! SD.exists(filename)) {
      // only open a new file if it doesn't exist
      outputFile = SD.open(filename, FILE_WRITE);
      break;  // leave the loop!
    }
  }

  if (! outputFile) {
    Serial.println("error: Could not create file");
    while (1);
  }

  Serial.print("Logging to: ");
  Serial.println(filename);
}

void initRTC()
{
  
  Wire.begin();
  if (!RTC.begin()) {
    outputFile.println("RTC failed");
  }
}
