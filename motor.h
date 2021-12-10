#ifndef motor_H
#define motor_H
#include "component.h"

  

class motor : public component
{
  public:
   int  startTestTime;
   int  countingTestTime;
   int countCheackTimeLower=0;
   float readingCurrent=0;
   const int cheackTime=100;
   const int numberCheackTimeLower=3;
   bool motorMode = false;
   
    
  protected:
    int  motorCurrentSub;
    
  public:
    void motor_current_Sub(int Sub);
    int show_motor_Current_Sub();
    void readingCurrentInsert(float reading);
    float showReadingCurrent ();
    void motorModeChange(bool Status);
    bool showMotorModeChange();
}; 
#endif
