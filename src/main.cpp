
#include <Arduino.h>
#include <WiFi.h>
#include "time.h"


#define LIGHT_PIN 27 // Replace with your pin connected to the relay
#define FLAG_LIGHT  35  // Error light (proc: wifi error)
#define ON  33

const long gmtOffset_sec = -28800;      // Adjust for your timezone (e.g., -28800 for PST)
const int daylightOffset_sec = 3600/2; // Adjust for daylight saving time if applicable
const char* ntpServer = "pool.ntp.org";

const int lightOnHours = 8; // Hours the grow light should be ON per day
const int sleepStartHour = 22; // 10 PM
const int sleepEndHour = 10;  // 10 AM

const int reconnectTime = 60*60*3; // If many wifi connection issues occur, wait 3 hours before trying to reconnect

void setup() {
  pinMode(LIGHT_PIN, OUTPUT);
  digitalWrite(LIGHT_PIN, LOW); 

  pinMode(FLAG_LIGHT, OUTPUT);
  digitalWrite(FLAG_LIGHT, LOW);

  pinMode(ON, OUTPUT);
  digitalWrite(ON, HIGH);

  // Connect to WiFi 
  WiFi.begin("Vilo_7945", "KTJrnFky");
  int count = 1;
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    count++;
    if (count > 15){
      digitalWrite(FLAG_LIGHT, HIGH);
      digitalWrite(LIGHT_PIN, LOW);
      delay(reconnectTime);
    } 
  }
  if (WiFi.status() == WL_CONNECTED){
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  }
  
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

  digitalWrite(LIGHT_PIN, lightOn ? HIGH : LOW);
  delay(60000);
}
