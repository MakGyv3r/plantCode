  #include "sensor.h"
  #include "component.h"


//  void sensor:: readingSetup(){
//    for (int thisReading = 0; thisReading < numReadings; thisReading++) {
//     this-> readings[thisReading] = 0;
//      }
//    }
    
  int sensor:: readingResultsPercent(){
    int readings[numReadings]={}; 
    readIndex = 0;  
    while(readIndex<numReadings){
      // subtract the last reading:
      total = total - readings[readIndex];
      // read from the sensor:
      readings[readIndex] = analogRead(pin);
     // Serial.println(readings[readIndex]);
      // add the reading to the total:
      total = total + readings[readIndex];
      // advance to the next position in the array:
      readIndex = readIndex + 1;
      // calculate the average:
      // send it to the computer as ASCII digits
    delay(30); 
    }
          averageResulte = (total / numReadings);
          this->total=0;
          Serial.println( numReadings);
          Serial.print( "averageResulte in presentage:");
          Serial.println( (100-averageResulte*100/4095));
          Serial.print( "averageResulte");
          Serial.println( (averageResulte));
          // delay in between reads for stability
//          readingSetup();
          delay(30);
         return(100 - averageResulte*100/4095);
  }

int sensor::readingResults(){
     int readings[numReadings]={}; 
     readIndex = 0;  
    while(readIndex<numReadings){
      // subtract the last reading:
      total = total - readings[readIndex];
      // read from the sensor:
      readings[readIndex] = analogRead(pin);
     // Serial.println(readings[readIndex]);
      // add the reading to the total:
      total = total + readings[readIndex];
      // advance to the next position in the array:
      readIndex = readIndex + 1;
      // calculate the average:
      // send it to the computer as ASCII digits
    delay(30); 
    }
          averageResulte = (total / numReadings);
          this->total=0;
          // delay in between reads for stability
//          readingSetup();
          delay(30);
         return(averageResulte);
    
  }
  
  int sensor::readingResultsParNumberTest(int numberTests){
    int specialreading[numberTests]={};
     readIndex = 0;  
    while(readIndex<numberTests){
      // subtract the last reading:
      total = total - specialreading[readIndex];
      // read from the sensor:
      specialreading[readIndex] = analogRead(pin);
     // Serial.println(readings[readIndex]);
      // add the reading to the total:
      total = total + specialreading[readIndex];
      // advance to the next position in the array:
      readIndex = readIndex + 1;
      // calculate the average:
      // send it to the computer as ASCII digits
    delay(30); 
    }
          averageResulte = (total / numReadings);
          this->total=0;
          // delay in between reads for stability
          delay(30);
         return(averageResulte);
  }
  
  int sensor::readingOneResult(){
    int readingsResult = analogRead(show_pin());
    delay(30);
    return(readingsResult); 
  }
  
  void sensor::writingState(bool stateChange) {
    this->state = stateChange;
    }

      bool sensor::showState() {
    return state;
    }
