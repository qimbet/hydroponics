
/*********************************************************************************
About this code; 

Build by Jacob Mattie, May-July 2025
Hydroponics system; controlled by esp32

Schematics available on github: 
https://github.com/qimbet/hydroponics

This should include CAD files for all 3d printed parts (e.g. pvc mounts, strut attachments)
A full BOM is not yet developed, but should also be included lol

*********************************************************************************/

#include <Arduino.h>
#include <WiFi.h>
#include "time.h"
#include "pw.h"

#define GROW_LIGHT_PIN 27 // Replace with your pin connected to the relay

#define FERTILIZER_PUMP_PIN 25
#define MAIN_PUMP_PIN 12
#define SUMP_DRAIN_FILTER_SOLENOID 4
#define WATER_LEVEL_SENSOR 32
#define TESTPINHIGH 26

#define minorErrorLight 1
#define majorErrorLight 2


const long gmtOffset_sec = -28800;      // Adjust for your timezone (e.g., -28800 for PST)
const int daylightOffset_sec = 3600/2; // Adjust for daylight saving time if applicable
const char* ntpServer = "pool.ntp.org";


/*********************************************************************************
 
                              System Scheduling

*********************************************************************************/

const int lightOnHours = 9; // Hours the grow light should be ON per day
const int sleepStartHour = 22; // 10 PM
const int sleepEndHour = 10;  // 10 AM



const int reconnectTime = 60*60*3; // If many wifi connection issues occur, wait 3 hours before trying to reconnect

/*********************************************************************************
 
                              Constants

*********************************************************************************/


const int minutesInMilliseconds = 1000*60; 
const int secondsInMilliseconds = 1000; 
unsigned long fillTimerStart = 0;
unsigned long elapsedTime = 0;

//These all need to be calibrated experimentally
const int fertilizeTimer = 1.5*secondsInMilliseconds;
const int fillTimeMax = 1.5*minutesInMilliseconds; //It shouldn't take longer than 1.5 minutes to fill the reservoir

const int drainTime = 10*minutesInMilliseconds;

bool minorError = false;
bool majorError = false;

/*********************************************************************************
 
                              Setup

*********************************************************************************/

void setup() {
  pinMode(GROW_LIGHT_PIN,                   OUTPUT);
  pinMode(FERTILIZER_PUMP_PIN,              OUTPUT);
  pinMode(MAIN_PUMP_PIN,                    OUTPUT);
  pinMode(SUMP_DRAIN_FILTER_SOLENOID,       OUTPUT);
  pinMode(TESTPINHIGH,                      OUTPUT);
  pinMode(WATER_LEVEL_SENSOR,               INPUT);

  digitalWrite(GROW_LIGHT_PIN,              LOW); 
  digitalWrite(FERTILIZER_PUMP_PIN,         LOW); 
  digitalWrite(MAIN_PUMP_PIN,               LOW); 
  digitalWrite(SUMP_DRAIN_FILTER_SOLENOID,  LOW); 
  digitalWrite(TESTPINHIGH,                 HIGH);


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

void waterPlants(){
  digitalWrite(MAIN_PUMP_PIN, HIGH);

  fillTimerStart = millis();
  while (digitalRead(WATER_LEVEL_SENSOR) != HIGH){
    elapsedTime = millis() - fillTimerStart;
    if (elapsedTime > fillTimeMax) { // This is a safety mechanism to prevent overfilling. If the water level sensor doesn't tick, the pump also has a time cutoff
      majorError = true;
      break;
    }
  }
  digitalWrite(MAIN_PUMP_PIN, LOW); //sensor-triggered shutoff. Should be the standard trigger for shutoff
}

void fertilize(){
  digitalWrite(FERTILIZER_PUMP_PIN, HIGH);
  delay(fertilizeTimer); // determined experimentally
  digitalWrite(FERTILIZER_PUMP_PIN, LOW);
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
    digitalWrite(SUMP_DRAIN_FILTER_SOLENOID,  LOW); 
    digitalWrite(TESTPINHIGH,                 HIGH);

    while (true){ //Endless loop, stops all hardware from running
      digitalWrite(majorErrorLight, HIGH);
      delay(500);
      digitalWrite(majorErrorLight, LOW);
      delay(500)
    }
  }
  
  struct tm timeinfo;
  if (!getLocalTime(&timeinfo)) {
    delay(1000);
    return; 
  }

  int currentHour = timeinfo.tm_hour;



  bool lightOn = false;
  if (currentHour >= sleepEndHour && currentHour < sleepStartHour) {

    int lightOnStart = sleepEndHour; // 10 AM
    int lightOnEnd = lightOnStart + lightOnHours; // Calculate end time

    if (currentHour >= lightOnStart && currentHour < lightOnEnd) {
      lightOn = true;
    }
  }

  digitalWrite(GROW_LIGHT_PIN, lightOn ? HIGH : LOW);
  delay(60000);



}
