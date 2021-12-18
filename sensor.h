#ifndef sensor_H
#define sensor_H
#include "component.h" 

class sensor : public component
{
  
  protected:
    static const int numReadings = 100;// value to determine the size of the readings array.
    int readings[numReadings];       // the readings from the analog input
    int readIndex = 0;               // the index of the current reading
    int total = 0;                   // the running total
    int averageResulte;             // the average resulte
    bool state=true;                // the average resulte
    
    
  public:
    void readingSetup();// initialize all the readings to 0:
    int readingResultsPercent(); // reading results and sending the average result in percent
    int readingResults(); // reading results and sending the average result in int 
    int readingOneResult(); // reading one result and returning it
    int readingResultsParNumberTest(int numberTests); // reading one result and returning it
    void writingState(bool stateChange );// depanding on the sensor getting "true" or "false"
    bool showState();// show the state
} ;
 #endif
