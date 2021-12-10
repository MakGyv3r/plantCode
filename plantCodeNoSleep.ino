#include <esp_now.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
//#include <WiFiAP.h>
#include <esp_wifi.h>
#include <ESP32httpUpdate.h>
#include "eeprom_functions.h"
#include "auto_watering_plan.h"
//#include "EOTAUpdate.h"
#include "motor.h"
#include "sensor.h"
#include "config_wifi.h"
#include <EEPROM.h>
#include <Adafruit_INA219.h>


#define EEPROM_SIZE 400 
#define EEPROM_ID 200
#define EEPROM_hubMacAddress 150
#define EEPROM_SSID 0
#define EEPROM_PASS 50
#define EEPROM_version 250
#define EEPROM_wifiWorked 275
#define EEPROM_updateWorked 285
#define EEPROM_updateURL 300
 
//sleeping defind
//#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
//#define TIME_TO_SLEEP  4.9        //Time ESP32 will go to sleep (in seconds)
//#define TIME_A_WAKE 100
//#define TIME_WHEN_START 300

RTC_DATA_ATTR int stopSleep=0;
int lastTaskRecived=0;
int lastRecivedTaskTime=0;
int lastRecivedTaskTimeApproved=1000;


//sleep memory 
RTC_DATA_ATTR uint8_t macRouter[6];
RTC_DATA_ATTR String plantIdNumber;
RTC_DATA_ATTR int timeAWakeStart;

//sleep memory auto Irrigate
RTC_DATA_ATTR bool autoIrrigateState=false;
int timeDelayWaterPump=3600000;
int timedelay_hum=5000;
int waterPumpOnTime=0;
int timeLength=0;
int timeLength2=0;
int timePass=0;
int timePass2=0; 

//autoIrrigate
static const int numReadings = 10;
int humvalue[numReadings];     // the readings from the analog hum input
int humIndex = 0;              // the index of the current reading
float total = 0;               // the running total
int humAverage = 0;            // the average
int pin;                       // reading pin


//need to go RTCmemory
//const char * const   VERSION_STRING = "0.1.1";
const unsigned short versuionNumber = 2;
RTC_DATA_ATTR bool updateWorked=false;//need to change before sending update
const char * const   UPDATE_URL     = "http://aqueous-river-62632.herokuapp.com/plantproduct.bin";
//  const char * const   UPDATE_URL     = "http://0db0f6a29cd7.ngrok.io/update.txt";
//EOTAUpdate updater(UPDATE_URL, versuionNumber); 
RTC_DATA_ATTR bool tryUpdate=false;

//mac address
char macStr[18];
uint8_t macGet[6];
  
//sensor calling
sensor moistureSensor;//moisture sensor calling
sensor lightSensor;//moisture sensor calling
sensor waterSensor;//moisture sensor calling
Adafruit_INA219 ina219;

//motor calling
motor waterMotor_AIN1 ;
//sleep memory motor
RTC_DATA_ATTR int motorCurrentSub;
RTC_DATA_ATTR int motorReadingsBefore;
RTC_DATA_ATTR int sendMotorState=false;

//irrigatePlant difrent plan options  veribale
int irrigatePlantOptionTime;
int irrigatePlantOptionsTimeCheck;

//WIFI init
Config_wifi wifi;

//struct of veribales that are received from router
typedef struct receiveDataStruct{ 
  int task;//the tesk to do
  String plantIdNumber;
  int motorCurrentSub;
  bool motorState;
  bool autoIrrigateState;
  int irrigatePlantOption;
  String UPDATE_URL;
  int versuionNumber;
  String ssid;
  String pass;
 } receiveDataStruct;
receiveDataStruct receiveData;

//struct of veribales that are sent to the router
typedef struct sentDataStruct{ 
  int task;
  String plantIdNumber;
  int batteryStatus;
  int moistureStatus;
  int lightStatus;
  bool motorState=false;
  bool waterState=true;
  bool autoIrrigateState=false;
  bool irrigatePlantWorking=false;
  unsigned short versuionNumber;
  bool massgeSuccess=true;
  bool wifiWorked=true;
  int motorStateSubtraction=0;
} sentDataStruct;
sentDataStruct sentData;


//functions 
void one_plantInitialization();//Stores the mac address
void two_irrigatePlantOptio();//A program that irrigate the plant accurding to spcifice times
void three_sendsSensors();//Checks the status of the sensors and gives update
void four_autoIrrigateState ();//Unabling auto Irrigate 
//void batteryStatus ();//Checks the battery state
void six_motorStopStart();//start or stop the motor
void testingMotorWaterState ();//cheacks if there is water
void eight_UpdateProgrem();//update the progrem
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);//sending data function
void sendtask();
void EspNowRegusterPeer();
void onReceiveData(const uint8_t * mac, const uint8_t *dataIncom, int len);//received data function
void swithTask( int task);// swith case of esp32 taskes 
void swithIrrigatePlantOptionTask( int task);// swith case how mcuh to irrigate the plant 
void sendMotorStartStopWorking ();//a function to send informasion that the motor strop working and the resuon
void print_wakeup_reason();
void autoIrrigateStateTestLoop();


void setup() {
    //Initialize Serial Monitor
  Serial.begin(115200);
  delay(50);
  Serial.println(WiFi.macAddress());

  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);//WIFI_MODE_STA
    //WiFi.mode(WIFI_MODE_STA);
      //WiFi.mode(WIFI_AP_STA);
  // build an if that will ceack and if the will cheack the version in the data base if it change 

  if((!*macRouter)||(!plantIdNumber==0)){
    EEPROM.begin(EEPROM_SIZE);
    if(!plantIdNumber==0){
      plantIdNumber=readStringEEPROM(EEPROM_ID);
  }}
  timeAWakeStart=millis();
  Serial.println(plantIdNumber);//delete before prduction
  Serial.println("it is starter, i am grut");//delete before prduction
  print_wakeup_reason();
  
//motor setup
  waterMotor_AIN1.set_comp( 27, "waterMotor" , 27);
  waterMotor_AIN1.init_output();

//sensor setup
  moistureSensor.set_comp( 34 ,"moisture", 34);
  moistureSensor.init_input();
  lightSensor.set_comp( 33 ,"light", 33);
  lightSensor.init_input(); 
  if (! ina219.begin()) {
    Serial.println("Failed to find INA219 chip");
    while (1) { delay(10); }
  }

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");// need to delet when it is started
     ESP.restart();
    return;
  }

  esp_now_register_recv_cb(onReceiveData);
  esp_now_register_send_cb(OnDataSent);
  
  if(versuionNumber<readStringEEPROM(EEPROM_version).toInt()){
      if (esp_now_deinit() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
      }
      WiFi.mode(WIFI_STA);//WIFI_MODE_STA
      tryUpdate=false;
      String ssidD=wifi.urldecode(readStringEEPROM(EEPROM_SSID));
      String passD=wifi.urldecode(readStringEEPROM(EEPROM_PASS));
      //String updateUrl=wifi.urldecode(readStringEEPROM(EEPROM_updateURL));
      char ssidA[100];
      char passA[100];
      ssidD.toCharArray(ssidA, 99);
      passD.toCharArray(passA, 99);
      Serial.println("Trying to connect to " + ssidD + " with passwd:" + passD);  
      WiFi.begin(ssidA, passA); 
      delay(1000);   
      int i = 100;
      while (i-- > 0 && WiFi.status() != WL_CONNECTED)
      {
         // WiFi.begin(ssidA, passA);
          delay(50);
          Serial.print(".");//delete before prduction
      }
      if (WiFi.status() != WL_CONNECTED)
      {      
        Serial.println("Connection failed");//delete before prduction 
        sentData.task=8;
        sentData.massgeSuccess=true;
        sentData.versuionNumber=versuionNumber;
        sentData.wifiWorked=false;
        updateWorked=false;
        sendtask();
      }
      else if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println("WiFi connected");//delete before prduction
        Serial.println("Checking for updates. Remember to set the URL!");//delete before prduction
        //updateWorked=true;
      //  Serial.println(updateUrl);  
        t_httpUpdate_return ret = ESPhttpUpdate.update(UPDATE_URL); 
        delay(30);
        } 
          if (esp_now_init() != ESP_OK) {
        Serial.println("Error initializing ESP-NOW");
        return;
      }
  }

}

void loop() {
   if (Serial.available() > 0) {//task that are recived from user
    stopSleep = Serial.readString().toInt() ;}
  if(waterMotor_AIN1.motorMode== true)
      testingMotorWaterState (); 
  if(autoIrrigateState == true){
      autoIrrigateStateTestLoop();}
  if(irrigatePlantOptionTime != 0 ){
    if(millis()-irrigatePlantOptionsTimeCheck>=irrigatePlantOptionTime){
      irrigatePlantOptionTime=0;
      sentData.irrigatePlantWorking=false;
      waterMotor_AIN1.motorModeChange(false);
      sendMotorStartStopWorking ();
    }
  }    
}
void six_motorStopStart() {
    if(receiveData.motorState== false){
      waterMotor_AIN1.motorModeChange(false);
        Serial.print("the motor status is:");//delete before prduction
        Serial.println(waterMotor_AIN1.showMotorModeChange());//delete before prduction
        delay(50);//delete before prduction
      sendMotorState=false;  
      sendMotorStartStopWorking ();      
    }
    if((receiveData.motorState == true)&&(sendMotorState== false)){
      motorCurrentSub=receiveData.motorCurrentSub;//need to save it to the rtc memory
      waterMotor_AIN1.motorModeChange(true);
      delay(100);
      Serial.print("motor mode:");//delete before prduction
      Serial.println(waterMotor_AIN1.motorMode);//delete before prduction
        delay(50);
      waterSensor.writingState(true);   
      sendMotorState=true;  
      sendMotorStartStopWorking ();
     } 
}
void updateSendMACAddress(String MacAddressSring){
  Serial.println("this is my mac address"+MacAddressSring);
   char MacAddressChar[18];
   MacAddressSring.toCharArray(MacAddressChar, 18);
     char* ptr;
     macRouter[0] = strtol( strtok(MacAddressChar,":"), &ptr, HEX );
  for( uint8_t i = 1; i < 6; i++ )
  {
    macRouter[i] = strtol( strtok( NULL,":"), &ptr, HEX );
  }
}
void onReceiveData(const uint8_t * mac, const uint8_t *dataIncom, int len) {
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(macStr);
  memcpy(&receiveData, dataIncom, sizeof(receiveData));
  Serial.print("Packet received size: ");
  Serial.println(sizeof(receiveData));
  Serial.print ("print receive Data task " );
  Serial.println( receiveData.task);
  Serial.println( receiveData.plantIdNumber);
  sentData.plantIdNumber=plantIdNumber; 
  if(receiveData.plantIdNumber== plantIdNumber ){
    if(!*macRouter){
          for (int i = 0; i < 6; i++) 
          macRouter[i]=(uint8_t)mac[i];   
    }
    
   //need to creat a function that send if recived to stop the medness 
//  if (lastRecivedTaskTime==0){
//    lastRecivedTaskTime=millis();
//  }
  Serial.print ("print receive Data task " );
  Serial.println( lastTaskRecived);
//    ten_sendReciveMassage();
//    if((lastTaskRecived != 3)&(lastRecivedTaskTime)){//if(receiveData.task != lastTaskRecived){ 
//        Serial.print ("i am doing  swithTask" );
       swithTask(receiveData.task);
//     }
//     lastTaskRecived=receiveData.task;
   }    
}
void swithTask( int task){  
  switch(task) {
    case 1:
      one_plantInitialization();
    break;
    case 2:
        two_irrigatePlantOption();
      break;
    case 3:
        three_sendsSensors();
      break;
    case 4:
        four_autoIrrigateState();
      break;
//  case 5:
//      batteryStatus();
//    break;
    case 6:
      six_motorStopStart();
    break;
    case 8:
      eight_UpdateProgrem();
    break;
}
}
void one_plantInitialization(){
   motorCurrentSub=receiveData.motorCurrentSub;
   sentData.task=1;
  // EspNowRegusterPeer();
   sendtask();
}
void two_irrigatePlantOption(){
  swithIrrigatePlantOptionTask(receiveData.irrigatePlantOption);
  sentData.irrigatePlantWorking=true;
  irrigatePlantOptionsTimeCheck=millis();
  motorCurrentSub=receiveData.motorCurrentSub;
  waterMotor_AIN1.motorModeChange(true);
  sendMotorStartStopWorking ();   
  testingMotorWaterState (); 
  if(waterMotor_AIN1.motorMode== false)
  sendMotorStartStopWorking ();      
}
void swithIrrigatePlantOptionTask( int irrigatePlantOption){  
       irrigatePlantOptionTime=irrigatePlantOption*1000;
}
}

void three_sendsSensors(){
    sentData.task=3;
    moistureSensor.readingSetup();
    lightSensor.readingSetup();
//    sentData.moistureStatus =50; //moistureSensor.readingResultsPercent();
    Serial.print("moistureStatus:");//delete before prduction
    Serial.println(moistureSensor.readingOneResult());//delete before prduction
    Serial.print("lightSensor:");//delete before prduction
    Serial.println(lightSensor.readingOneResult());//delete before prduction
    sentData.moistureStatus= moistureSensor.readingResultsPercent();   
    sentData.lightStatus= 100-lightSensor.readingResultsPercent();   
//    sentData.batteryStatus = batterySensor.readingResultsPercent();// need to creat 

    sendtask();
}
void four_autoIrrigateState () {
  if(receiveData.autoIrrigateState == false){
    autoIrrigateState=false;
    Serial.println("autoWatering disebel");//delete before prduction
  }
  if(receiveData.autoIrrigateState == true){
    autoIrrigateState=true;//need to save it to the rtc memory
    waterSensor.writingState(true); 
    Serial.println("autoWatering enabel");//delete before prduction
  }
  
  timePass=millis();
  timePass2=millis(); 
  timeLength2=timePass2;
  timeLength = timePass;

  sentData.autoIrrigateState= autoIrrigateState;
  sendMotorStartStopWorking ();
}//for tesk1
void autoIrrigateStateTestLoop() {      
  //Serial.println("working2");//delete before prduction
  timePass2=millis();  
  timePass=millis();
    
  if(timePass2-timeLength2 > timedelay_hum)
    { 
      waterMotor_AIN1.motorModeChange(false); 
      delay(50);
      soilMoistureDegree(moistureSensor.readingResultsParNumberTest(numReadings));//func that chacke the state of the soil  
      timeLength2=timePass2;
    }
    

  if(timePass-timeLength > timeDelayWaterPump)
    { 
      waterMotor_AIN1.motorModeChange(true);
      delay(50);
      Serial.println("watering ");//delete before prduction
      if (timePass-timeLength> timeDelayWaterPump+waterPumpOnTime)
        {
        waterMotor_AIN1.motorModeChange(false);
        delay(50); 
        timeLength = timePass;
        }       
    }
    
 }
void soilMoistureDegree (int humAverage){//func that chacke the state of the soil
    Serial.println(humAverage);//delete before prduction
    if (humAverage>2900)
      {
       Serial.println(" :  soil is too dry needs special treatment"); //delete before prduction
       delay(30);
       timeDelayWaterPump=60000;// one minut = 60000
       waterPumpOnTime=2000;
       
      }
    else if (humAverage<2900 && humAverage>2760)
      {
       Serial.println(" :  soil is dry"); //delete before prduction
       delay(30);
       timeDelayWaterPump=3600000; // one hour
       waterPumpOnTime=5000;
      }
    else if(humAverage<2760)
      {
       Serial.println(" :  soil is moist no watering needed");  //delete before prduction
       delay(30);
       timeDelayWaterPump=86400000;//24hr water delay
       waterPumpOnTime=5000;
      }
 } 
void testingMotorWaterState (){
//   delay(50);
   waterMotor_AIN1.countingTestTime=millis(); 
   if ((waterMotor_AIN1.countingTestTime-waterMotor_AIN1.startTestTime>waterMotor_AIN1.cheackTime)&&(digitalRead(waterMotor_AIN1.show_pin())== 1)){
//    sentData.task=7;
    waterMotor_AIN1.readingCurrentInsert(ina219.getCurrent_mA());
//    sentData.motorStateSubtraction=(int)waterMotor_AIN1.showReadingCurrent();
    Serial.println(waterMotor_AIN1.showReadingCurrent());//delete before prduction
//    delay(100);
    //sendtask();
    if ((waterMotor_AIN1.showReadingCurrent()<motorCurrentSub)&&(waterMotor_AIN1.showReadingCurrent()>0))
    {
      waterMotor_AIN1.countCheackTimeLower=waterMotor_AIN1.countCheackTimeLower+1;
      delay(50);//delete before prduction
      Serial.print("count ");//delete before prduction
      Serial.println(waterMotor_AIN1.countCheackTimeLower);//delete before prduction
      delay(50);//delete before prduction
      if (waterMotor_AIN1.countCheackTimeLower>=waterMotor_AIN1.numberCheackTimeLower){
        waterMotor_AIN1.motorModeChange(false);
        Serial.println("motor Status 'off'");//delete before prduction
        delay(50);//delete before prduction
        Serial.print("the motor status is:");//delete before prduction
        delay(50);//delete before prduction
        Serial.println(waterMotor_AIN1.showMotorModeChange());//delete before prduction
        delay(50);//delete before prduction
        waterMotor_AIN1.countCheackTimeLower=0;
        autoIrrigateState=false;
        waterSensor.writingState(false);
        irrigatePlantOptionTime=0;
        sentData.autoIrrigateState= autoIrrigateState;
        sendMotorState=false; 
        delay(1000);
        sendMotorStartStopWorking();
      }       
    }
 
    else if ((waterMotor_AIN1.showReadingCurrent()<=0))
    {
        waterMotor_AIN1.motorModeChange(false);
//        Serial.println("motor Status 'off'");//delete before prduction
//        Serial.println(waterMotor_AIN1.showReadingsAffter());//delete before prduction
//        delay(50);//delete before prduction
//        Serial.print("the motor status is:");//delete before prduction
//        delay(50);//delete before prduction
//        Serial.println(waterMotor_AIN1.showMotorModeChange());//delete before prduction
//        delay(50);//delete before prduction
        waterMotor_AIN1.countCheackTimeLower=0;
        autoIrrigateState=false;
        waterSensor.writingState(false);
        irrigatePlantOptionTime=0;
        sendMotorState=false; 
        delay(1000);
        sendMotorStartStopWorking();     
    }        
    else if (waterMotor_AIN1.showReadingCurrent()>motorCurrentSub) {
      waterMotor_AIN1.countCheackTimeLower=0;
      waterSensor.writingState(true); 
      Serial.println("i am in else");//delete before prduction
      delay(50);
    }
    waterMotor_AIN1.startTestTime=millis();
   }
}
void sendMotorStartStopWorking (){
  sentData.task=4;
  if ((waterMotor_AIN1.motorMode== false)&&(waterSensor.showState()==false)){        
        sentData.motorState=waterMotor_AIN1.motorMode;
        sentData.irrigatePlantWorking=false;
        sentData.autoIrrigateState=false;
        sentData.waterState=waterSensor.showState();
        Serial.print("motor mode:");//delete before prduction
        delay(50);
        Serial.println(waterMotor_AIN1.motorMode);//delete before prduction
        delay(50);
  }
  else {
  sentData.waterState=waterSensor.showState();
  sentData.motorState=waterMotor_AIN1.motorMode;
//          Serial.print("motor pin:");//delete before prduction
//        delay(50);
//        Serial.println(digitalRead(27));//delete before prduction
//        delay(50);    
  } 
  sendtask();
}
void eight_UpdateProgrem(){ 
    sentData.massgeSuccess=true;
    sentData.task=10;
    sendtask(); 
    Serial.println(receiveData.versuionNumber);
    Serial.println(versuionNumber);
  if(receiveData.versuionNumber>=versuionNumber){
    tryUpdate=true;
    String macStrS=wifi.urldecode((String)macStr);
    writeStringEEPROM(EEPROM_hubMacAddress,macStrS);
    writeStringEEPROM(EEPROM_SSID,wifi.urldecode(receiveData.ssid));
    writeStringEEPROM(EEPROM_PASS,wifi.urldecode(receiveData.pass));
    writeStringEEPROM(EEPROM_version,wifi.urldecode(String(receiveData.versuionNumber)));        
    delay(100); 
    ESP.restart();
  } 
//  if(receiveData.versuionNumber==versuionNumber){
//    tryUpdate=false;
//    sentData.task=8;
//    sentData.massgeSuccess=true;
//    sentData.versuionNumber=versuionNumber;
//    sentData.IsUpdating=false;
//    sendtask();
//  }
  
}
void ten_sendReciveMassage(){ 
  sentData.task=10;
  sendtask();
}
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
  delay(50);
    switch (status){
  case ESP_NOW_SEND_SUCCESS:
     Serial.println("Success");
     delay(50);
    stopSleep=0;
   break;
  case ESP_NOW_SEND_FAIL:
      Serial.println("fail");
   break;   
   default:
       Serial.println("fail");
      break;
  } 
}
void EspNowRegusterPeer(){
  esp_now_peer_info_t peerInfo= {};
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  // register first peer  
  memcpy(peerInfo.peer_addr, (uint8_t *)macRouter, 6);
  if (!esp_now_is_peer_exist((uint8_t *)macRouter))
  {
    esp_now_add_peer(&peerInfo);
  }
}
void sendtask(){
  EspNowRegusterPeer();
  esp_err_t result = esp_now_send((uint8_t *)macRouter, (uint8_t *)&sentData, sizeof(sentData));
  if (result == ESP_OK)
  {
    Serial.println("Sent with success");//delete before prduction
    delay(100);
  }
  else
  {
    Serial.println("Error sending the data");//delete before prduction
  } 
}
//Function that prints the reason by which ESP32 has been awaken from sleep
void print_wakeup_reason(){
  esp_sleep_wakeup_cause_t wakeup_reason;
  wakeup_reason = esp_sleep_get_wakeup_cause();
  switch(wakeup_reason)
  {
    case 1  : Serial.println("Wakeup caused by external signal using RTC_IO"); break;
    case 2  : Serial.println("Wakeup caused by external signal using RTC_CNTL"); break;
    case 3  : Serial.println("Wakeup caused by timer"); break;
    case 4  : Serial.println("Wakeup caused by touchpad"); break;
    case 5  : Serial.println("Wakeup caused by ULP program"); break;
    default : Serial.println("Wakeup was not caused by deep sleep"); break;
  }
}
 
