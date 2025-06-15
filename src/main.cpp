
#include <Arduino.h>
#include <WiFi.h>
#include "time.h"


#define GROW_LIGHT_PIN 27 // Replace with your pin connected to the relay

#define FERTILIZER_PUMP_PIN 12
#define MAIN_PUMP_PIN 2

#define WATERING_SOLENOID_PIN 25
#define SUMP_DRAIN_FILTER_SOLENOID 4

#define WATER_LEVEL_SENSOR 32

const long gmtOffset_sec = -28800;      // Adjust for your timezone (e.g., -28800 for PST)
const int daylightOffset_sec = 3600/2; // Adjust for daylight saving time if applicable
const char* ntpServer = "pool.ntp.org";

const int lightOnHours = 9; // Hours the grow light should be ON per day
const int sleepStartHour = 22; // 10 PM
const int sleepEndHour = 10;  // 10 AM

const int reconnectTime = 60*60*3; // If many wifi connection issues occur, wait 3 hours before trying to reconnect

const int msToM = 1000*60; //Milliseconds to minutes
unsigned long fillTimerStart = 0;
unsigned long elapsedTime = 0;

//These all need to be calibrated experimentally
const int fertilizeTimer = 0.5*msToM;
const int fillTime = 2*msToM;
const int drainTime = 10*msToM;

void setup() {
  pinMode(GROW_LIGHT_PIN, OUTPUT);
  digitalWrite(GROW_LIGHT_PIN, LOW); 


  pinMode(FERTILIZER_PUMP_PIN, OUTPUT);
  digitalWrite(FERTILIZER_PUMP_PIN, LOW); 

  pinMode(MAIN_PUMP_PIN, OUTPUT);
  digitalWrite(MAIN_PUMP_PIN, LOW); 


  pinMode(WATERING_SOLENOID_PIN, OUTPUT);
  digitalWrite(WATERING_SOLENOID_PIN, LOW); 

  pinMode(SUMP_DRAIN_FILTER_SOLENOID, OUTPUT);
  digitalWrite(SUMP_DRAIN_FILTER_SOLENOID, LOW); 

  pinMode(WATER_LEVEL_SENSOR, INPUT);

  // Connect to WiFi 
  WiFi.begin("Vilo_7945", "KTJrnFky"); //Admittedly it's kind of whack for this to be wifi-connected, but idc anymore
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

void waterPlants(){

  digitalWrite(MAIN_PUMP_PIN, HIGH);

  fillTimerStart = millis();
  while (WATER_LEVEL_SENSOR != HIGH){
    elapsedTime = millis() - fillTimerStart;

    if (elapsedTime > fillTime) { // This is a safety mechanism to prevent overfilling. If the water level sensor doesn't tick, the pump also has a time cutoff
      break;
    }
  }
  digitalWrite(MAIN_PUMP_PIN, LOW); //sensor-triggered shutoff. Nominal.
  
  digitalWrite(WATERING_SOLENOID_PIN, HIGH);
  delay(drainTime);
  digitalWrite(WATERING_SOLENOID_PIN, LOW);
}

void fertilize(){
  digitalWrite(FERTILIZER_PUMP_PIN, HIGH);
  delay(fertilizeTimer); // determined experimentally
  digitalWrite(FERTILIZER_PUMP_PIN, LOW);
}



void loop() {
  
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
