  #include "motor.h"
  #include "component.h"


  
  void motor::motor_current_Sub(int Sub) {
    this->motorCurrentSub = Sub;
    }   
  int motor::show_motor_Current_Sub (){
    return motorCurrentSub;
    }
  void motor::readingsAffterInsert(int reading){
    this->readingsAffter = reading;
    }   
  int motor::showReadingsAffter(){
    return readingsAffter;
    }
    
  void motor::motorModeChange(bool Status){
        if(Status == true)
          on_comp();
        else
          off_comp();
        this->motorMode=Status;
      }
    bool motor::showMotorModeChange(){
    return motorMode;
    }
