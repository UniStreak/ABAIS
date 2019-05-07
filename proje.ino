// Top servo pin	0
// Bottom servo pin	1
// LCD SDA			2
// LCD SCL			3
// Soil moisture   	4 (A6)
// Relay pin       	5
// Water level pin 	6 (A7)
// RealTimeClock  	7 8 9
// Sim Card (GSM)  	10 11 12
// LDRs + pot		A0-A4
// Water flow		A5

#include <Servo.h>
#include <LiquidCrystal_I2C_AvrI2C.h>
#include <Wire.h>
#include <Sim800l.h>
#include <SoftwareSerial.h>
#include <virtuabotixRTC.h>

//Definitions for sim card usage
Sim800l Sim800l;
char* textMessage;
char* number = "+905544544972"; //User's phone number
String incomingText, numberSms;
uint8_t index;
bool errorSend;
bool errorDel;
bool postpone = false;

//Definitions for servo motors
Servo bottomServo;
Servo topServo;
int bottomServoLocation = 92;
int topServoLocation = 30;
const int bottomServoPin = 1;
const int topServoPin = 0;

//Definitions for LDRs
const int ldrL = A0;
const int ldrR = A1;
const int ldrB = A2;
const int ldrT = A3;
const int potPin = A4;
int ldrLValue = 0;
int ldrRValue = 0;
int ldrBValue = 0;
int ldrTValue = 0;
int potValue = 0;

//Definitions for water pump and sensors
const int relayPin = 5;
const int soilPin = A6;
const int waterFlowPin = A5;
const int waterLevelPin = A7;
int waterLevelValue;
int soilMoistureValue;
int waterFlowValue;

//Definitions for Real Time Clock
const int rtcCLK = 7;
const int rtcDAT = 8;
const int rtcRST = 9;
int day, month, year, minute, second, hour;
int lastHour = 0;
virtuabotixRTC myRTC(rtcCLK, rtcDAT, rtcRST);
unsigned long lastTime = 0;
unsigned long newTime = 0;

//Definition for LCD
LiquidCrystal_I2C_AvrI2C lcd(0x27, 16, 2);


void setup() //This function is called when a sketch starts.
{
  Serial.begin(9600); //Starts serial communication.

  //Starts the LCD screen on ABAIS.
  lcd.begin();
  lcd.backlight();
  abaisOnTheLCD();
  lcd.print("Please wait...");
  //Attach the Servo variable to a pin.
  bottomServo.attach(bottomServoPin);
  topServo.attach(topServoPin);
  bottomServo.write(bottomServoLocation);
  topServo.write(topServoLocation);
  delay(50); //Pauses the program (in milliseconds) for stable operation.

  //Sim800l.begin();
  //Sim800l.reset();
  //delay(5000);
  //textMessage = "Welcome to ABAIS. To learn the available SMS commands, type 'help' and reply to this message.";
  //SendSMS();
  //errorDel=Sim800l.delAllSms(); //clean memory of sms
  delay(2000);
}

void loop() //This function loops consecutively, allowing your program to change and respond.
{
  solarTrackingLoop(15); //Follow the sun for 15 seconds to determine the position of the sun.
  timeOnTheLCD();
  
  //Read sensors values.
  readWaterLevel();
  readSoilMoisture();
  readWaterFlow();

  if (waterLevelValue < 20) {
    Serial.println("Water level is low.");
    abaisOnTheLCD();
    lcd.print("Check water tank");
    //textMessage = "The amount of water in the tank is low, please add water.";
    //SendSMS();
    delay(2000);
  }

  //irrigation(3);
  if (!postpone) {
    if ((myRTC.hours >= 18) || (myRTC.hours < 6)) {
      if (myRTC.hours != lastHour) {
        if (soilMoistureValue < 40) {
          irrigation(5);
          lastHour = myRTC.hours;
        }
      }
    }
  }

  //readSMS();

  Serial.println("----END LOOP----");
  delay(5000);
}

void readSMS()
{
  incomingText = Sim800l.readSms(1); //read the first sms
  if (incomingText.indexOf("OK") != -1) {
    if (incomingText.length() > 7) {
      numberSms = Sim800l.getNumberSms(1);
      Serial.println(numberSms);
      incomingText.toUpperCase();
      if (incomingText.indexOf("STOP") != -1) {
        postpone = true;
        Serial.println("postpone = true");
      }
      else if (incomingText.indexOf("START") != -1) {
        postpone = false;
        Serial.println("postpone = false");
      }
      else if (incomingText.indexOf("SYSTEMTIME") != -1) {
        textMessage = myRTC.dayofmonth;
        textMessage += '/';
        textMessage += myRTC.month;
        textMessage += '/';
        textMessage += myRTC.year;
        textMessage += ' ';
        textMessage += myRTC.hours;
        textMessage += ':';
        textMessage += myRTC.minutes;
        SendSMS();
      }
      else if (incomingText.indexOf("HELP") != -1) {
        textMessage = "1/4 To postpone irrigation jobs indefinitely, write 'stop'.";
        SendSMS();
        delay(3000);
        textMessage = "2/4 To start indefinitely postponed irrigation jobs, write 'start'.";
        SendSMS();
        delay(3000);
        textMessage = "3/4 To learn the system time, write 'systemtime'.";
        SendSMS();
        delay(3000);
        textMessage = "4/4 To learn the available SMS commands,  write 'help'.";
        SendSMS();
        delay(3000);
      }
      else {
        Serial.println("incomingText not compatible!");
      }
      Sim800l.delAllSms(); //when receive a new sms always will be in first position
    }
  }
}

void timeOnTheLCD()
{
  delay(1000);
  instantTime();
  abaisOnTheLCD();
  lcd.print(myRTC.dayofmonth);
  lcd.print("/");
  lcd.print(myRTC.month);
  lcd.print("/");
  lcd.print(myRTC.year);
  lcd.print(" ");
  lcd.print(myRTC.hours);
  lcd.print(":");
  lcd.print(myRTC.minutes);
}

void solarTrackingLoop(int loopSecond)
{
  abaisOnTheLCD();
  lcd.print("Solar tracking..");
  second = myRTC.seconds;
  minute = myRTC.minutes;
  while (1) {
    solarTracking();
    newTime = millis();
    if (newTime - lastTime > 3000) {
      instantTime();
      Serial.println("UPDATED");
      lastTime = newTime;
    }
    if ((myRTC.seconds - second > loopSecond) || (second - myRTC.seconds > loopSecond)) {
      Serial.println("BREAK");
      break;
    }
  }
}

void solarTracking()
{
  ldrLValue = analogRead(ldrL);
  ldrRValue = analogRead(ldrR);
  ldrTValue = analogRead(ldrT);
  ldrBValue = analogRead(ldrB);
  potValue = analogRead(potPin);
  potValue = map(potValue, 0, 1023, 0, 50);

  Serial.print("potValue = ");
  Serial.print(potValue);
  Serial.print(" ldrRValue = ");
  Serial.print(ldrRValue);
  Serial.print(" ldrLValue = ");
  Serial.print(ldrLValue);
  Serial.print(" ldrTValue = ");
  Serial.print(ldrTValue);
  Serial.print(" ldrBValue = ");
  Serial.println(ldrBValue);

  if (ldrLValue > ( ldrRValue + potValue ))
  {
    if (bottomServoLocation > 0)
      bottomServoLocation -= 1;
    bottomServo.write(bottomServoLocation);
  }

  if (ldrRValue > ( ldrLValue + potValue ))
  {
    if ( bottomServoLocation < 180 )
      bottomServoLocation++;
    bottomServo.write(bottomServoLocation);
  }

  if (ldrTValue > ( ldrBValue + potValue ))
  {
    if ( topServoLocation > 0 )
      topServoLocation -= 1;
    topServo.write(topServoLocation);
  }

  if (ldrBValue > ( ldrTValue + potValue ))
  {
    if (topServoLocation < 180)
      topServoLocation++;
    topServo.write(topServoLocation);
  }
  delay(50);
}

void instantTime()
{
  myRTC.updateTime();
  Serial.print("Date / Time : ");
  Serial.print(myRTC.dayofmonth);
  Serial.print("/");
  Serial.print(myRTC.month);
  Serial.print("/");
  Serial.print(myRTC.year);
  Serial.print(" ");
  Serial.print(myRTC.hours);
  Serial.print(":");
  Serial.print(myRTC.minutes);
  Serial.print(":");
  Serial.println(myRTC.seconds);
}

void readSoilMoisture()
{
  int x;
  x = analogRead(soilPin);
  x = map(x, 0, 1023, 0, 100);
  x = 100 - x;
  soilMoistureValue = x;
  Serial.print("soilMoistureValue: ");
  Serial.println(soilMoistureValue);
}

void readWaterLevel()
{
  int x;
  x = analogRead(waterLevelPin);
  x = map(x, 0, 1023, 0, 100);
  waterLevelValue = x;
  Serial.print("waterLevelValue: ");
  Serial.println(waterLevelValue);
}

void readWaterFlow()
{
  int x;
  x = analogRead(waterFlowPin);
  x = map(x, 0, 1023, 0, 100);
  waterFlowValue = x;
  Serial.print("waterFlowValue: ");
  Serial.println(waterFlowValue);
}

void irrigation(int iSecond)
{
  if (waterLevelValue > 20) {
    pinMode(relayPin, OUTPUT);
    digitalWrite(relayPin, LOW);
    Serial.println("Irrigation is started.");
    iSecond = 1000 * iSecond;
    readWaterFlow();
    delay(iSecond);
    if (waterFlowValue < 50) {
      Serial.println("Irrigation is NOT completed. Water flow error.");
      abaisOnTheLCD();
      lcd.print("Irrigation error");
      //textMessage = "Irrigation failure. Please check the water pipe connections.";
      //SendSMS();
    }
    else {
      Serial.println("Irrigation is completed.");
      //textMessage = "Irrigation is completed successfully.";
      //SendSMS();
    }
    digitalWrite(relayPin, HIGH);
    delay(50);
    pinMode(relayPin, INPUT);
  }
  else {
    Serial.println("Irrigation is NOT completed. Water level is too low.");
  }
}

void SendSMS()
{
  errorSend = Sim800l.sendSms(number, textMessage);
  if (errorSend != 0) {
    abaisOnTheLCD();
    lcd.print("SMS error!!");
    Serial.println("SMS error!!");
  }
}

void abaisOnTheLCD()
{
  lcd.clear();
  lcd.print("    ~ABAIS");
  lcd.setCursor(0, 1);
}
