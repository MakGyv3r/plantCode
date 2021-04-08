#include <esp_now.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include "eeprom_functions.h"
#include "auto_watering_plan.h"
#include "EOTAUpdate.h"
#include "motor.h"
#include "sensor.h"


#define EEPROM_SIZE 300
#define EEPROM_plantIdNumber 200
#define EEPROM_routerMacAddress 50

//sleeping defind
#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  4.9        //Time ESP32 will go to sleep (in seconds)

//sleep memory 
RTC_DATA_ATTR uint8_t macRouter[6];
RTC_DATA_ATTR String plantIdNumber;
RTC_DATA_ATTR bool autoIrrigateState;

//need to go RTCmemory
//const char * const   VERSION_STRING = "0.1";
  const unsigned short VERSION_NUMBER = 1;
  const char * const   UPDATE_URL     = "http://morning-falls-78321.herokuapp.com/update.txt";
//  const char * const   UPDATE_URL     = "http://0db0f6a29cd7.ngrok.io/update.txt";
  EOTAUpdate updater(UPDATE_URL, VERSION_NUMBER);
  
int timeAWake=100; 
char macStr[18];
uint8_t macGet[6];

//config the autoWatering
  auto_watering_plan  autoWatering;
  int resultTestLoop;
  
//sensor calling
  sensor moistureSensor;//moisture sensor calling
  sensor lightSensor;//moisture sensor calling
  sensor waterSensor;//moisture sensor calling

//motor calling
  motor waterMotor_AIN1 ;

//irrigatePlant difrent plan options  veribale
 int irrigatePlantOptionTime=0;
 int irrigatePlantOptionsTimeCheck;

//struct of veribales that are received from router
typedef struct receiveDataStruct{ 
  int task;//the tesk to do
  String plantIdNumber;
  int motorCurrentSub;
  bool motorState;
  bool autoIrrigateState;
  int irrigatePlantOption;
  unsigned short versuionNumber;
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
  bool massgeSuccess=false;
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
void eight_checkUpdateProgrem();//update the progrem

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status);//sending data function
void sendtask();
void EspNowRegusterPeer();
void onReceiveData(const uint8_t * mac, const uint8_t *dataIncom, int len);//received data function
void swithTask( int task);// swith case of esp32 taskes 
void swithIrrigatePlantOptionTask( int task);// swith case how mcuh to irrigate the plant 
void sendMotorStartStopWorking ();//a function to send informasion that the motor strop working and the resuon

void setup() {
    //Initialize Serial Monitor
  Serial.begin(115200);
  delay(50);
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);
  if((!*macRouter)||(!plantIdNumber==0)){
    EEPROM.begin(EEPROM_SIZE);
    if(!plantIdNumber==0){
      plantIdNumber=readStringEEPROM(EEPROM_plantIdNumber);
  }}
  Serial.println("it is starter, i am grut");//delete before prduction

  
//motor setup
  waterMotor_AIN1.set_comp( 27, "waterMotor" , 27);
  waterMotor_AIN1.init_output();

//sensor setup
  moistureSensor.set_comp( 34 ,"moisture", 34);
  moistureSensor.init_input();
  lightSensor.set_comp( 33 ,"light", 33);
  lightSensor.init_input(); 
  waterSensor.set_comp( 35 ,"waterlevel", 35);
  waterSensor.init_input(); 
  autoWatering.testSetup(34) ;
  
  waterSensor.readingSetup();

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");// need to delet when it is started
     ESP.restart();
    return;
  }
  
  esp_now_register_recv_cb(onReceiveData);
  esp_now_register_send_cb(OnDataSent);
}

void loop() {
  testingMotorWaterState (); 
  if(autoWatering.autoWateringEnable == true)
  {
    resultTestLoop=autoWatering.testLoop();
    if(resultTestLoop == 1)
      waterMotor_AIN1.motorModeChange(true);
    if(resultTestLoop == 0) 
      waterMotor_AIN1.motorModeChange(false);
//      sendMotorStartStopWorking ();  
  }
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
      sendMotorStartStopWorking ();
    }
    if(receiveData.motorState == true){
      waterMotor_AIN1.motor_current_Sub(receiveData.motorCurrentSub);//need to save it to the rtc memory
      waterMotor_AIN1.readingsBefore= waterSensor.readingResults();
      delay(50);
      waterMotor_AIN1.motorModeChange(true);
      delay(100);
      testingMotorWaterState ();  
      sendMotorStartStopWorking ();
     } 
}

void eight_checkUpdateProgrem(){
//  if(receiveData.versuionNumber>VERSION_NUMBER){
//    WiFi.begin(receiveData.ssid, receiveData.pass);
//    int i = 5;
//    while (i-- > 0 && WiFi.status() != WL_CONNECTED)
//    {
//        delay(50);
//        Serial.print(".");//delete before prduction
//    }
//    if (WiFi.status() != WL_CONNECTED)
//    {
//        Serial.println("Connection failed");//delete before prduction
//    }
//    else if (WiFi.status() == WL_CONNECTED)
//    {
//      Serial.println("WiFi connected");//delete before prduction
//      WiFi.mode(WIFI_STA);
//      Serial.println("Checking for updates. Remember to set the URL!");//delete before prduction
//      updater.CheckAndUpdate();
//      delay(30);
//      }
//  }
}
void sendMotorStartStopWorking (){
  sentData.task=4;
  if ((waterMotor_AIN1.motorMode== false)&&(waterSensor.showState()==false)){        
         sentData.motorState=waterMotor_AIN1.motorMode;
        sentData.irrigatePlantWorking=false;
        sentData.autoIrrigateState=false;
        sentData.waterState=waterSensor.showState();
        Serial.println("motor mode");//delete before prduction
        Serial.println(waterMotor_AIN1.motorMode);//delete before prduction
  }
  else  sentData.motorState=waterMotor_AIN1.motorMode;     
  sendtask();
}

void onReceiveData(const uint8_t * mac, const uint8_t *dataIncom, int len) {
  Serial.print("Packet received from: ");
  snprintf(macStr, sizeof(macStr), "%02x:%02x:%02x:%02x:%02x:%02x",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
  Serial.println(macStr);
  memcpy(&receiveData, dataIncom, sizeof(receiveData));
  Serial.print("Packet received size: ");
  Serial.println(sizeof(receiveData));
  Serial.println("print receive Data task" );
  Serial.println( receiveData.task);
  Serial.println( receiveData.plantIdNumber);
  sentData.plantIdNumber=plantIdNumber; 
  if(receiveData.plantIdNumber== plantIdNumber ){
    receiveData.plantIdNumber="";
    if(!*macRouter){
          for (int i = 0; i < 6; i++) 
          macRouter[i]=(uint8_t)mac[i];
    }
   swithTask(receiveData.task);
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
//      eight_checkUpdateProgrem();
    break;
}
}
void one_plantInitialization(){
   waterMotor_AIN1.motor_current_Sub(receiveData.motorCurrentSub);//need to save it to the rtc memory
   sentData.task=1;
   sentData.massgeSuccess=true;
   EspNowRegusterPeer();
   sendtask();
}
void two_irrigatePlantOption(){
  swithIrrigatePlantOptionTask(receiveData.irrigatePlantOption);
  sentData.irrigatePlantWorking=true;
  irrigatePlantOptionsTimeCheck=millis();
  waterMotor_AIN1.motor_current_Sub(receiveData.motorCurrentSub);//need to save it to the rtc memory
  waterMotor_AIN1.readingsBefore= waterSensor.readingResults();
  waterMotor_AIN1.motorModeChange(true);
  testingMotorWaterState (); 
  sendMotorStartStopWorking ();      
}
void swithIrrigatePlantOptionTask( int irrigatePlantOption){  
  switch(irrigatePlantOption) {
    case 1:
       irrigatePlantOptionTime=3000;
      break;
    case 2:
       irrigatePlantOptionTime=4000;
      break;
    case 3:
       irrigatePlantOptionTime=10000;
      break;
}
}
void testingMotorWaterState (){
   delay(50);
   waterMotor_AIN1.countingTestTime=millis();   
   if ((waterMotor_AIN1.countingTestTime-waterMotor_AIN1.startTestTime>waterMotor_AIN1.cheackTime)&&(digitalRead(waterMotor_AIN1.show_pin())== 1)){
    Serial.print("hall test resulte after motore start");//delete before prduction
    waterMotor_AIN1.readingsAffterInsert(waterSensor.readingOneResult());
    delay(50);
    Serial.println(waterSensor.readingOneResult());//delete before prduction
    delay(50);//delete before prduction
    Serial.println(waterMotor_AIN1.readingsBefore-waterMotor_AIN1.showReadingsAffter());//delete before prduction
    if ((waterMotor_AIN1.readingsBefore-waterMotor_AIN1.showReadingsAffter()<waterMotor_AIN1.show_motor_Current_Sub())&&(waterMotor_AIN1.readingsBefore-waterMotor_AIN1.showReadingsAffter()>50))
    {
      waterMotor_AIN1.countCheackTimeLower++;
      if (waterMotor_AIN1.countCheackTimeLower>=waterMotor_AIN1.numberCheackTimeLower){
        waterMotor_AIN1.motorModeChange(false);
        Serial.println("motor Status 'off'");//delete before prduction
//        Serial.println(waterMotor_AIN1.showReadingsAffter());//delete before prduction
        delay(50);//delete before prduction
        waterMotor_AIN1.countCheackTimeLower=0;
        autoWatering.autoWateringState (false); 
        waterSensor.writingState(false);
        irrigatePlantOptionTime=0;
        sendMotorStartStopWorking ();
      }       
    }
    //else if (waterMotor_AIN1.readingsBefore-waterMotor_AIN1.showReadingsAffter ()>(waterMotor_AIN1.show_motor_Current_Sub())){waterMotor_AIN1.countCheackTimeLower=0; }
    else {
      waterMotor_AIN1.countCheackTimeLower=0;
      waterSensor.writingState(true); 
//      Serial.println("i am in else");//delete before prduction
//      delay(100);
    }
    waterMotor_AIN1.startTestTime=millis();
   }
}
void three_sendsSensors(){
    sentData.task=3;
    moistureSensor.readingSetup();
    lightSensor.readingSetup();
    sentData.moistureStatus = moistureSensor.readingResultsPercent();
    sentData.lightStatus= lightSensor.readingResultsPercent();   
//    sentData.batteryStatus = batterySensor.readingResultsPercent();// need to creat 
    sendtask();
}
void four_autoIrrigateState () {
  if(receiveData.autoIrrigateState == false){
    autoWatering.autoWateringState(false);
    Serial.println("autoWatering disebel");//delete before prduction
  }
  if(receiveData.autoIrrigateState == true){
    waterMotor_AIN1.motor_current_Sub(receiveData.motorCurrentSub);//need to save it to the rtc memory
    waterMotor_AIN1.readingsBefore= waterSensor.readingResults();
    autoWatering.autoWateringState(true);
    Serial.println("autoWatering enabel");//delete before prduction
  }
  sentData.autoIrrigateState= autoWatering.autoWateringEnable;
  sendMotorStartStopWorking ();
}//for tesk1

void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
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
  }
  else
  {
    Serial.println("Error sending the data");//delete before prduction
  } 
}

 /*   byte macTry[6];
  uint8_t * macGet;
  setMACToEEPROM(EEPROM_routerMacAddress,(uint8_t *)macRouter);
    uint8_t macGet[6];
          for (int i = 0; i < 6; i++) {   
            macGet[i] = EEPROM.read(EEPROM_routerMacAddress + i);
          //  Serial.println(macGet[i],HEX);
          }
       char macTry[18];
      snprintf(macTry, sizeof(macTry), "%02x:%02x:%02x:%02x:%02x:%02x",
        macGet[0], macGet[1], macGet[2], macGet[3], macGet[4], macGet[5]);
      Serial.println(macTry);

  
    Serial.println("test eeprom: ");
  setMACToEEPROM(EEPROM_routerMacAddress,mac);
    for (int i = 0; i < 6; i++) {
    macTry[i] = EEPROM.read(EEPROM_routerMacAddress + i);}
   macGet=(uint8_t *)macTry;*/
