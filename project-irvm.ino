// Board:             DOIT ESP32 (Node ESP?)
// Documentation URL: asdasd
// WIFI
#include <WiFi.h>
#include <HTTPClient.h>
const char* ssid = "IRVMWIFI";
const char* password = "12345678";
String serverName = "http://martorenzo.click/project/smartbin/server/";  // include "/"
String deviceName = "1";
int wifiConnected = 0;

// LCD
#include <LiquidCrystal_I2C.h>
LiquidCrystal_I2C lcd(0x27, 16, 2);

// Servo
#include <ESP32Servo.h>
Servo myservo1;
Servo myservo2;
Servo myservo3;

// Load Cell
#include "HX711.h"
HX711 scale1;                         // Metal
HX711 scale2;                         // Plastic
float loadcellCalibration1 = 975;     // Metal
float loadcellCalibration2 = 975;     // Plastic
float loadcellReading1 = 0;           // Metal
float loadcellReading2 = 0;           // Plastic

// Pin Input - Loadcell
int loadcellDout1 = 18;               // 
int loadcellSck1 = 5;                 // 

// Pin Input - Sensor
int metalsensor = 23;
int objectsensor = 35;

// Pin Input - Button
int btnCandy = 32;
int btnOpen = 33;
int btnCode = 19;

// Pin Output - Servo
int servoPin1 = 4;                    // Metal
int servoPin2 = 2;                    // Plastic
int servoPin3 = 15;                   // Candy

// Others
int Vmetalsensor = 0;
int Vobjectsensor = 0;
int VCandy = 0;
int VOpen = 0;                        // basurahan 
int VCode = 0;

int accumulatedPoints = 0;
int displayMode = 0;                  // 0 - idle
                                      // 1 - metal
                                      // 2 - plastic
                                      // 3 - candy
                                      // 8 - code

String dCode = "";                    // server - code
String dReward = "0";                 // server - reward


// =====================================
// START
// =====================================
void setup()
{
  // Serial
  Serial.begin(9600);

  // init lcd
  lcd.init();
  lcd.backlight();
  lcd.clear();

  // Pin Input
  pinMode(metalsensor , INPUT_PULLUP);
  pinMode(objectsensor , INPUT_PULLUP);
  pinMode(btnCandy , INPUT_PULLUP);
  pinMode(btnOpen , INPUT_PULLUP);
  pinMode(btnCode , INPUT_PULLUP);

  // Scale
  scale1.begin(loadcellDout1, loadcellSck1);
  scale1.set_scale(loadcellCalibration1);
  scale1.tare();
  scale2.begin(loadcellDout2, loadcellSck2);
  scale2.set_scale(loadcellCalibration2);
  scale2.tare();

  // Pin Output
  myservo1.setPeriodHertz(50);
  myservo1.attach(servoPin1);
  myservo2.setPeriodHertz(50);
  myservo2.attach(servoPin2);
  myservo3.setPeriodHertz(50);
  myservo3.attach(servoPin3);

  //
  lcd.setCursor(0, 0);
  lcd.print("     IVENDO     ");

  //
  myservo1.write(0);
  delay(1000);
  myservo2.write(0);
  delay(1000);
  myservo3.write(0);
  delay(1000);
}


// =====================================
// LOOP
// =====================================
void loop()
{
  // Input
  Vmetalsensor = digitalRead(metalsensor);
  Vobjectsensor = digitalRead(objectsensor);
  VCandy = digitalRead(btnCandy);
  VOpen = digitalRead(btnOpen);
  VCode = digitalRead(btnCode);

  // Loadcell
  Serial.println("Loadcell 1 (Metal): " + String(scale1.get_units()) + " grams");
  Serial.println("Loadcell 2 (Plastic): " + String(scale2.get_units()) + " grams");

  // Wifi
  if (wifiConnected == 0)
  {
    ConnectWifi();
  }

  // Display
  {
    // 0 - idle
    // 1 - metal
    // 2 - plastic
    // 3 - candy
    // 8 - code
    
    // Idle
    if (displayMode == 0)
    {
      //
      lcd.setCursor(0, 0);
      lcd.print("   No  Object   ");
      lcd.setCursor(0, 1);
      lcd.print("Points:    " + ConvertNumberSpace(String(accumulatedPoints)));
    }

    // Metal
    if (displayMode == 1)
    {
      //
      lcd.setCursor(0, 0);
      lcd.print(" Metal Detected ");
      lcd.setCursor(0, 1);
      lcd.print("Points:    " + ConvertNumberSpace(String(accumulatedPoints)));

      //
      RequestSetPointsOn();
      scale1.tare();
      scale2.tare();

      //
      myservo1.write(180);
      delay(5000);
      myservo1.write(0);
      delay(1000);

      //
      loadcellReading1 = scale1.get_units();
      float result = loadcellReading1;
      int resultRound = result;
      accumulatedPoints = accumulatedPoints + resultRound;

      //
      RequestSetPointsOff();
      displayMode = 0;
    }

    // Plastic
    if (displayMode == 2)
    {
      //
      lcd.setCursor(0, 0);
      lcd.print("Plastic Detected");
      lcd.setCursor(0, 1);
      lcd.print("Points:    " + ConvertNumberSpace(String(accumulatedPoints)));

      //
      RequestSetPointsOn();
      scale1.tare();
      scale2.tare();

      //
      myservo2.write(180);
      delay(5000);
      myservo2.write(0);
      delay(1000);

      //
      loadcellReading2 = scale2.get_units();
      float result = loadcellReading2;
      int resultRound = result;
      accumulatedPoints = accumulatedPoints + resultRound;

      //
      RequestSetPointsOff();
      displayMode = 0;
    }

    // Candy
    if (displayMode == 3)
    {
      //
      while(dReward.toInt() > 0)
      {
        if (dReward.toInt() < 200)
        {
          break;  
        }
        
        //
        RequestSetRewardOn();
        
        //
        lcd.setCursor(0, 0);
        lcd.print("  Reward Claim  ");
        lcd.setCursor(0, 1);
        lcd.print("Points:    " + ConvertNumberSpace(dReward));

        // 
        int x = dReward.toInt() - 200;
        dReward = String(x);

        //
        myservo3.write(180);
        delay(1000);
        myservo3.write(0);
        delay(1000);
      }

      //
      RequestSetRewardOff();
      displayMode = 0;
    }

    // Code
    if (displayMode == 8)
    {
      //
      lcd.setCursor(0, 0);
      lcd.print("  Reward  Code  ");
      lcd.setCursor(0, 1);
      lcd.print("    " + dCode + "    ");

      //
      delay(10000);
      RequestSetPointsOff();
      displayMode = 0;
    }
  }

  // Button
  {
    // Claim
    if (!VCandy && displayMode == 0)
    {
      RequestGetRewardStatus();
      int x = accumulatedPoints + dReward.toInt();
      dReward = String(x);
      accumulatedPoints = 0;
      displayMode = 3;
    }

    // Open Basurahan
    if (!VOpen)
    {
      
    }

    // Code
    if (!VCode && displayMode == 0)
    {
      RequestSetCode();
      RequestSetPointsOn();
      RequestGetCodeStatus();
      accumulatedPoints = 0;

      if (dCode != "" && dCode.length() == 8)
      {
        displayMode = 8;
      }
    }
  }
  
  // Sensor
  {
    // Metal
    if (!Vmetalsensor && !Vobjectsensor && !VOpen)
    {
      displayMode = 1;
    }

    // Plastic
    if (Vmetalsensor && !Vobjectsensor && !VOpen)
    {
      displayMode = 2;
    }
  }
}


// =====================================
// FUNCTION
// =====================================
// Connect
void ConnectWifi()
{
  lcd.setCursor(0, 0);
  lcd.print("   CONNECTING   ");
  lcd.setCursor(0, 1);
  lcd.print("      WIFI      ");

  Serial.println("init wifi");
  // init wifi
  WiFi.begin(ssid, password);
  Serial.println("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to WiFi network with IP Address: ");
  Serial.println(WiFi.localIP());

  wifiConnected = 1;
}

// Get Code Status
void RequestGetCodeStatus()
{
  if (wifiConnected == 0)
  {
    return;
  }
  
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=devcodestatus";
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
    dCode = payload;
  }

  http.end();

  delay(500);
}

// Get Reward Status
void RequestGetRewardStatus()
{
  if (wifiConnected == 0)
  {
    return;
  }
  
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=devrewardstatus";
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
    dReward = payload;
  }

  http.end();

  delay(500);
}

// Get Reward Status
void RequestSetCode()
{
  if (wifiConnected == 0)
  {
    return;
  }
  
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=devcodeset&cpoints=" + String(accumulatedPoints);
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();

  delay(500);
}

// Set In-use by Reward
void RequestSetRewardOn() 
{
  if (wifiConnected == 0)
  {
    return;
  }
  
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=devinuserewardset";
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();

  delay(500);
}

// Set In-use by Points
void RequestSetPointsOn() 
{
  if (wifiConnected == 0)
  {
    return;
  }
  
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=devinusepointsset";
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();

  delay(500);
}

// Set In-use by Reward Off
void RequestSetRewardOff() 
{
  if (wifiConnected == 0)
  {
    return;
  }
  
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=devinuserewardoffset";
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();

  delay(500);
}

// Set In-use by Points Off
void RequestSetPointsOff() 
{
  if (wifiConnected == 0)
  {
    return;
  }
  
  HTTPClient http;
  String serverPath = serverName + "api.php?mode=devinusepointsoffset";
  http.begin(serverPath.c_str());
  int httpResponseCode = http.GET();

  if (httpResponseCode > 0) {
    String payload = http.getString();
    Serial.println(payload);
  }

  http.end();

  delay(500);
}

String ConvertNumberSpace(String x) 
{ // result will be "      "
  if (x.length() == 4)
  {
    x = " " + x;
  }

  if (x.length() == 3)
  {
    x = "  " + x;
  }

  if (x.length() == 2)
  {
    x = "  " + x;
  }

  if (x.length() == 1)
  {
    x = "   " + x;
  }

  if (x.length() <= 0)
  {
    x = "     ";
  }

  return x;
}
