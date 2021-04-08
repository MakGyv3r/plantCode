#ifndef eeprom_functions
#define eeprom_functions
#include <EEPROM.h>
#include <mac_eeprom.h>
#define MAX_EEPROM_LEN 50 
#define EEPROM_SIZE 300


//EEPROM functions
void writeStringEEPROM(char add, String data){
    int _size = data.length();
    if (_size > MAX_EEPROM_LEN)
        return;
    int i;
    for (i = 0; i < _size; i++)
    {
        EEPROM.write(add + i, data[i]);
    }
    EEPROM.write(add + _size, '\0'); //Add termination null character for String Data
    EEPROM.commit();
}

String readStringEEPROM(char add){
    int i;
    char data[MAX_EEPROM_LEN + 1];
    int len = 0;
    unsigned char k;
    k = EEPROM.read(add);
    while (k != '\0' && len < MAX_EEPROM_LEN) //Read until null character
    {
        k = EEPROM.read(add + len);
        data[len] = k;
        len++;
    }
    data[len] = '\0';
    return String(data);
}

const uint8_t *getMACFromEEPROM(char add)
{
  uint8_t mac[6];
  for (int i = 0; i < 6; i++) {   
    mac[i] = EEPROM.read(add + i);
    Serial.println(mac[i],HEX);
  }
  static const uint8_t buffer[] = {mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]};
  return buffer;
}

void setMACToEEPROM(char add,const uint8_t *mac)
{
  for (int i = 0; i < 6; i++) {
    EEPROM.write(add + i, mac[i]);
//    Serial.println(mac[i],HEX);
  }
  EEPROM.commit();
}
 #endif
