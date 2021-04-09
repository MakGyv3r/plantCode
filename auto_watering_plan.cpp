 #include "auto_watering_plan.h"

void auto_watering_plan::testSetup(int pin) {
  // initialize all the readings to 0 for average:
  this->pin=pin;
  for (int thisReading = 0; thisReading < numReadings; thisReading++) 
    {
       this-> humvalue[numReadings] = 0;
    }
}

//void auto_watering_plan::autoWateringState ( ){
//         Serial.println(autoWateringEnable);//delete before prduction
//  //this-> autoWateringEnable=autoWateringEnable;
//}

int auto_watering_plan::testLoop() {
  
    int timePass=millis();
    int timePass2=millis();
    
       Serial.println("working2");//delete before prduction

   if(timePass-timeLength > timeDelayWaterPump)
    { 
      return  1; // turn the motor on (HIGH is the voltage level)
      Serial.println("watering");//delete before prduction
      if (timePass-timeLength> timeDelayWaterPump+2000)
        {
        return 0; // turn the motor off by making the voltage LOW 
        this-> timeLength = timePass;
        }       
    }

  if(timePass2-timeLength2 > timedelay_hum)
    { 
      readingResults ();
      soulMoistureDegree();//func that chacke the state of the soil  
     this-> timeLength2=timePass2;
    }
   return 0; 
 }


void auto_watering_plan::readingResults () {
   // subtract the last reading:
  this->total = total - humvalue[humIndex];
  // read from the sensor:
  this->humvalue[humIndex] = analogRead(pin);
  Serial.println(humvalue[humIndex]);//delete before prduction
  delay(20);
  // add the reading to the total:
 this-> total = total + humvalue[humIndex];
  // advance to the next position in the array:
  this->humIndex = humIndex + 1;
  
  // if we're at the end of the array...
  if (humIndex >= numReadings) {
    // ...wrap around to the beginning:
    this-> humIndex = 0;
  }
   Serial.println("working");//delete before prduction
       delay(50);
  // calculate the average:
  this-> humAverage = (total / numReadings); 
}



void auto_watering_plan::soulMoistureDegree ()//func that chacke the state of the soil
{
    Serial.println(humAverage);
    if (humAverage>2900)
      {
       Serial.println(" :  soil is too dry needs special treatment"); //delete before prduction
       this->timeDelayWaterPump=60000;// one minut = 60000
       this->waterPumpOnTime=2000;
      }
    else if (humAverage<2900 && humAverage>2760)
      {
       Serial.println(" :  soil is dry"); //delete before prduction
       this->timeDelayWaterPump=3600000; // one hour
       this->waterPumpOnTime=5000;
      }
    else if(humAverage<2760)
      {
       Serial.println(" :  soil is moist no watering needed");  //delete before prduction
       this->timeDelayWaterPump=86400000;//24hr water delay
       this->waterPumpOnTime=5000;
      }
 }
          
