#ifndef motor_H
#define motor_H
#include "component.h"

  

class motor : public component
{
  public:
   int  startTestTime;
   int  countingTestTime;
   int countCheackTimeLower=0;
   int readingsAffter;//hall semsore reading affter motor start
   int readingsBefore;//hall semsore reading before motor start
   const int cheackTime=100;
   const int numberCheackTimeLower=4;
   bool motorMode = false;
   
    
  protected:
    int  motorCurrentSub;
    
  public:
    void motor_current_Sub(int Sub);
    int show_motor_Current_Sub();
    void readingsAffterInsert(int reading);
    int showReadingsAffter ();
    void motorModeChange(bool Status);
    bool showMotorModeChange();
}; 
#endif
