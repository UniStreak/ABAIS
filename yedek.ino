// analog deger pinleri: A6-A11 4 6 8 9 10 12

// üst servo pin  0
// alt servo pin 	1
// LCD SDA        2
// LCD SCL        3
// toprak         4 (A6)
// Role pin       5
// su seviye      6 (A7)
// RTC            7 8 9
// sim TX pin     10
// sim RX pin     11
// sim RST pin    12
// LDR+pot 			A0-A4
// akış				A5

#include <Servo.h>
#include <LiquidCrystal_I2C_AvrI2C.h>
#include <Wire.h>
#include <Sim800l.h>
#include <SoftwareSerial.h>
#include <virtuabotixRTC.h>   

//Definitions to sim card usage
Sim800l Sim800l;
char* text; 
char* number; 
char* incomingText;    
uint8_t index;   
bool error; 

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
int soilMoistureValue; 
const int waterFlowPin=A5;
const int waterLevelPin = A7;

//Definitions for Real Time Clock
const int rtcCLK = 7;
const int rtcDAT = 8;
const int rtcRST = 9;
int day,month,year,minute,second,hour;
virtuabotixRTC myRTC(rtcCLK, rtcDAT, rtcRST);

//Definition for LCD
LiquidCrystal_I2C_AvrI2C lcd(0x27,16,2);
unsigned long lastTime = 0;
unsigned long newTime = 0;

void setup() //sistem başlangıcı
{
  Serial.begin(9600);
  lcd.begin(); 
  lcd.backlight(); 
  lcd.setCursor(0,0); 
  lcd.print("     ~ABAIS");
  delay(500); 
  
  bottomServo.attach(bottomServoPin);
  topServo.attach(topServoPin);
  bottomServo.write(bottomServoLocation);
  topServo.write(topServoLocation);

  //Sim800l.begin(); 
  //delay(10000);
  //text = "denemeq";
  //SendSMS();
  delay(2000);
  instantTime(); 
}

void loop()
{
  second = myRTC.seconds;
  minute = myRTC.minutes;  
  while(1){
    solarTracking();
    newTime=millis();
    if (newTime - lastTime > 1000){
      myRTC.updateTime();      
      lastTime=newTime;
      Serial.println("UPDATE");
    }        
    if ((myRTC.seconds - second > 15)||(second - myRTC.seconds > 15)){
      Serial.println("BREAK");
      break;
    }
  }  
  Serial.println("donguden ciktik");
  delay(5000);
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
 delay(60);
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

int readSoilMoisture()
{
  int x;
	x=analogRead(soilPin);    //deger 700 ün üstündeyse sula 300 ün altındaysa sulama (1023 kuru, 0 ıslak)
	x=map(x,0,1023,0,100);
	x=100-x;
	return x;
}

void SendSMS()
{
	number="+905544544972"; 
	error=Sim800l.sendSms(number,text);
	//Sim800l.sendSms("+540111111111","the text go here")
}
