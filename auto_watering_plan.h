#ifndef auto_watering_plan_h
#define auto_watering_plan_h

#include <Arduino.h>


class auto_watering_plan 
{
  public:
  int timeDelayWaterPump=3600000; // one hour= 3600000 one minut = 60000
  int timedelay_hum=5000;
  int waterPumpOnTime=0;
  int timeLength=0,timeLength2=0;
  //hum sensor
  static const int numReadings = 10;
  int humvalue[numReadings];      // the readings from the analog hum input
  int humIndex = 0;              // the index of the current reading
  float total = 0;                  // the running total
  int humAverage = 0;                // the average
  int pin;// reading pin
  bool autoWateringEnable1;  
    
  public:
    void testSetup(int pin) ;
    int testLoop();  
    void readingResults();
    void soulMoistureDegree ();
//    void autoWateringState (bool autoWateringEnable );
} ;
 #endif
