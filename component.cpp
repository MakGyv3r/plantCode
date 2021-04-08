#include "component.h"


  void component::set_comp(int id, String  name1,int pin)
  {
    this-> pin = pin;
    this-> id=id;
    this-> name1=name1;
  }
  
  void component::init_output(){
    pinMode(pin,OUTPUT);
    off_comp();
  }
  void component::on_comp(){
    digitalWrite(pin, HIGH);
  }
  
  void component::off_comp(){
    digitalWrite(pin,LOW);
  } 
  void component::init_input(){
    pinMode(pin,INPUT);
  }
    int component::show_pin(){
    return pin;
  }
  
  String component::show_id(){
    return String(id);
  }

 
