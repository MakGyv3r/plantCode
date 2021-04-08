#ifndef COMPONENT_H
#define COMPONENT_H

#include <Arduino.h>

class component
 {
  protected:
    int  id;// date base id number
    String  name1; // name of the sensor
    int pin; // lag that the sensor is connected to  
    
  public:
  void set_comp(int id, String  name1 ,int pin);
  void init_output();
  void on_comp();
  void off_comp();
  void init_input();
  int show_pin();
  String show_id();
 };
 
 #endif
