
/*********************************************************************************
About this code; 

Build by Jacob Mattie, May-July 2025
Hydroponics system; controlled by esp32

Schematics available on github: 
https://github.com/qimbet/hydroponics

This should include CAD files for all 3d printed parts (e.g. pvc mounts, strut attachments)
A full BOM is not yet developed, but should also be included lol

Small text appendment to test github password reset
*********************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "pw.h"

#define GROW_LIGHT_PIN 27 // Replace with your pin connected to the relay

#define FERTILIZER_PUMP_PIN 25
#define MAIN_PUMP_PIN 12
#define WATER_LEVEL_SENSOR 32
#define drainSolenoid 100
#define outputHigh 101

#define minorErrorLight 34
#define majorErrorLight 35


const long gmtOffset_sec = -28800;      // Adjust for your timezone (e.g., -28800 for PST)
const int daylightOffset_sec = 3600/2; // Adjust for daylight saving time if applicable
const char* ntpServer = "pool.ntp.org";

/*********************************************************************************
 
                              Constants

*********************************************************************************/

//System variables
bool minorError = false;  //error code 1
bool majorError = false;  // error code 2
unsigned long fillTimerStart = 0;
unsigned long elapsedTime = 0;

int wateringStatus = 0; 
int fertilizerStatus = 0;
int lightStatus = 0;


//Universal constants
const int minutesInMilliseconds = 1000*60; 
const int secondsInMilliseconds = 1000; 

/*********************************************************************************
 
                              System Scheduling

*********************************************************************************/

/*********************************************
Grow Light
**********************************************/
const int lightOnHours = 9; // Hours the grow light should be ON per day
const int growLightStart = 10;  // 10 AM
const int growLightShutoff = 22; // 10 PM

/*********************************************
Watering Pump
**********************************************/
const int wateringTime = 15; //3 pm

const unsigned long reservoirMaxFillTime = 1.5*minutesInMilliseconds; 
const unsigned long pipeFlushTime = 0.5*minutesInMilliseconds;

/*********************************************
Fertilizer
**********************************************/
const int fertilizeTime = 15; //3pm, ensure that fertilizer is pumped prior to water (ensures mixing)
const int fertilizeDay = 0; //Sunday

const unsigned long fertilizeTimer = 2*secondsInMilliseconds;

/*********************************************
Wifi
**********************************************/
const int reconnectTime = 60*30; // If many wifi connection issues occur, wait 30 minutes before trying to reconnect

/*********************************************************************************
 
                              Setup

*********************************************************************************/

void setup() {
  pinMode(GROW_LIGHT_PIN,                   OUTPUT);
  pinMode(FERTILIZER_PUMP_PIN,              OUTPUT);
  pinMode(MAIN_PUMP_PIN,                    OUTPUT);
  pinMode(drainSolenoid,                    OUTPUT);
  pinMode(outputHigh,                       OUTPUT);
  pinMode(WATER_LEVEL_SENSOR,               INPUT);

  digitalWrite(GROW_LIGHT_PIN,              LOW); 
  digitalWrite(FERTILIZER_PUMP_PIN,         LOW); 
  digitalWrite(MAIN_PUMP_PIN,               LOW); 
  digitalWrite(drainSolenoid,               LOW);
  digitalWrite(outputHigh,                  HIGH);

  // Connect to WiFi 
  WiFi.begin(WIFI_SSID, WIFI_PW); //Admittedly it's kind of whack for this to be wifi-connected, but idc anymore
  int count = 1;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    count++;
    if (count > 15){
      digitalWrite(GROW_LIGHT_PIN, LOW);
      delay(reconnectTime);
    } 
  }
  if (WiFi.status() == WL_CONNECTED){
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }
  
}

/*********************************************************************************
 
                              Function Definitions

*********************************************************************************/
bool timedSystem(int targetPin, int timeout, int errorLevel, int sensorShutoffPin=500, int sensorFlagForShutoff=HIGH){
  digitalWrite(targetPin, HIGH);
  fillTimerStart = millis();

  if (sensorShutoffPin != 500){

    while (digitalRead(sensorShutoffPin) != sensorFlagForShutoff){
        elapsedTime = millis() - fillTimerStart;

        if (elapsedTime > timeout) { // This is a safety mechanism to prevent overfilling. If the water level sensor doesn't tick, the pump also has a time cutoff
          if (errorLevel == 1){
            minorError = true;
          }
          else if (errorLevel == 2){
            majorError = true;
          }
          return false;

        }
      }
  }
  digitalWrite(targetPin, LOW); //sensor-triggered shutoff. Should be the standard trigger for shutoff
  return true;
}

bool runOnSchedule(int scheduledHourTime, bool daily, const struct tm& t, int scheduledDayOfTheWeek=0){
  bool runToday = true;
  if (daily == false){
    if (t.tm_mday != scheduledDayOfTheWeek){
      runToday = false;
    }
  }

  if (runToday == true){
    if (scheduledHourTime > t.tm_hour){
      return true;
    }
    else{
      return false;
    }
  }
  else{
    return false;
  }
}
/*********************************************************************************
 
                              Main Loop

*********************************************************************************/

void loop() {
  if (minorError == true){//Turns on flag light
    digitalWrite(minorErrorLight, HIGH); 
  }
  if (majorError == true){ //resets all pinouts to initVals, halts operation. Warning light blinks
    digitalWrite(GROW_LIGHT_PIN,              LOW); //reset all pins to default
    digitalWrite(FERTILIZER_PUMP_PIN,         LOW); 
    digitalWrite(MAIN_PUMP_PIN,               LOW); 

    while (true){ //Endless loop, stops all hardware from running
      digitalWrite(majorErrorLight, HIGH);
      delay(500);
      digitalWrite(majorErrorLight, LOW);
      delay(500);
    }
  }

  struct tm timeinfo; //pass this as an argument 'timeinfo'
  if (!getLocalTime(&timeinfo)) { //gets current time. Works, but should be revised
    delay(1000);
    return; 
  }
  
  int currentHour = timeinfo.tm_hour;   //hours since midnight
  int currentDay = timeinfo.tm_mday;    //day of the month
  int dayOfTheWeek = timeinfo.tm_wday;  //days since sunday; [0-6]


  delay(15*secondsInMilliseconds); //slows down loop, but locks out interaction. Problematics with errorFlags & reset button


  /*******************************************************************************************
  Controls Grow Light
  *******************************************************************************************/
  bool lightOn = false;
  if (currentHour >= growLightStart && currentHour < growLightShutoff) {
    int lightOnEnd = growLightStart + lightOnHours; // Calculate end time

    if (currentHour < lightOnEnd) {
      lightOn = true;
    }
 
  }
  digitalWrite(GROW_LIGHT_PIN, lightOn ? HIGH : LOW);

  /*******************************************************************************************
  Controls Watering Pump
  *******************************************************************************************/
  //wateringTime
  bool wateredToday = false;

  if (wateredToday == false){
    if(runOnSchedule(wateringTime, true, timeinfo)){  
      wateredToday = timedSystem(MAIN_PUMP_PIN, reservoirMaxFillTime, 2, WATER_LEVEL_SENSOR, HIGH);
      timedSystem(drainSolenoid, pipeFlushTime, 1)
    }
  }

  /*******************************************************************************************
  Controls Fertilizer
  *******************************************************************************************/
  //fertilizeTime
  //fertilizeDay
  bool fertilizedToday = false;

  if (fertilizedToday == false){
    if (runOnSchedule(fertilizeTime, false, timeinfo, fertilizeDay)){
      fertilizedToday = timedSystem(FERTILIZER_PUMP_PIN, fertilizeTime, 1);
    }
    
  }
}
