#include <esp_now.h>
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClient.h>
//#include <WiFiAP.h>
#include <esp_wifi.h>
#include "eeprom_functions.h"
#include "auto_watering_plan.h"
#include "EOTAUpdate.h"
#include "motor.h"
#include "sensor.h"
#include "config_wifi.h"


#define EEPROM_SIZE 300
#define EEPROM_plantIdNumber 200
#define EEPROM_hubMacAddress 150
#define EEPROM_SSID 0
#define EEPROM_PASS 50
#define EEPROM_version 250
#define EEPROM_wifiWorked 275
#define EEPROM_updateWorked 285

//sleeping defind
#define uS_TO_S_FACTOR 1000000  //Conversion factor for micro seconds to seconds
#define TIME_TO_SLEEP  4.9        //Time ESP32 will go to sleep (in seconds)
#define TIME_A_WAKE 100
#define TIME_WHEN_START 300
RTC_DATA_ATTR int stopSleep=0;

// master number  FC:F5:C4:31:A4:7C

//sleep memory 
RTC_DATA_ATTR uint8_t macRouter[6];
RTC_DATA_ATTR String plantIdNumber;
RTC_DATA_ATTR int timeAWakeStart;

//sleep memory auto Irrigate
RTC_DATA_ATTR bool autoIrrigateState=false;
RTC_DATA_ATTR bool timeDelayWaterPump=3600000;
RTC_DATA_ATTR bool timedelay_hum=5000;
RTC_DATA_ATTR bool waterPumpOnTime=0;
RTC_DATA_ATTR bool timeLength=0;
RTC_DATA_ATTR bool timeLength2=0;

//autoIrrigate
static const int numReadings = 10;
int humvalue[numReadings];     // the readings from the analog hum input
int humIndex = 0;              // the index of the current reading
float total = 0;               // the running total
int humAverage = 0;            // the average
int pin;                       // reading pin


//need to go RTCmemory
//const char * const   VERSION_STRING = "0.1";
const unsigned short VERSION_NUMBER = 1;
RTC_DATA_ATTR bool updateWorked=false;//need to change before sending update
const char * const   UPDATE_URL     = "http://morning-falls-78321.herokuapp.com/update.txt";
//  const char * const   UPDATE_URL     = "http://0db0f6a29cd7.ngrok.io/update.txt";
EOTAUpdate updater(UPDATE_URL, VERSION_NUMBER); 
RTC_DATA_ATTR bool tryUpdate=false;

//mac address
char macStr[18];
uint8_t macGet[6];
  
//sensor calling
sensor moistureSensor;//moisture sensor calling
sensor lightSensor;//moisture sensor calling
sensor waterSensor;//moisture sensor calling

//motor calling
motor waterMotor_AIN1 ;
//sleep memory motor
RTC_DATA_ATTR int motorCurrentSub;
RTC_DATA_ATTR int motorReadingsBefore;

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
  unsigned short VERSION_NUMBER;
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
  unsigned short VERSION_NUMBER;
  bool massgeSuccess=true;
  bool wifiWorked=true;
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
  //Set device as a Wi-Fi Station
  WiFi.mode(WIFI_STA);//WIFI_MODE_STA
    //WiFi.mode(WIFI_MODE_STA);
      //WiFi.mode(WIFI_AP_STA);
  // build an if that will ceack and if the will cheack the version in the data base if it change 

  if((!*macRouter)||(!plantIdNumber==0)){
    EEPROM.begin(EEPROM_SIZE);
    if(!plantIdNumber==0){
      plantIdNumber=readStringEEPROM(EEPROM_plantIdNumber);
  }}
  timeAWakeStart=millis();
 // Serial.println(plantIdNumber);//delete before prduction
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
  waterSensor.set_comp( 35 ,"waterlevel", 35);
  waterSensor.init_input(); 
 // waterSensor.readingSetup();

  //Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");// need to delet when it is started
     ESP.restart();
    return;
  }

  esp_now_register_recv_cb(onReceiveData);
  esp_now_register_send_cb(OnDataSent);
//Set timer to 5 seconds
  esp_sleep_enable_timer_wakeup(TIME_TO_SLEEP * uS_TO_S_FACTOR);   

  if (updateWorked==true){
    Serial.println("update worked mother fucker");
    delay(1000);
    updateWorked=false;
    sentData.task=8;
    sentData.massgeSuccess=true;
    sentData.plantIdNumber=plantIdNumber; 
    sentData.VERSION_NUMBER=VERSION_NUMBER;
    sendtask();
  }
  
  if(tryUpdate==true){
      WiFi.mode(WIFI_STA);//WIFI_MODE_STA
      tryUpdate=false;
//      String ssid="Y&t";
//      String pass="0526855952";
//       String ssidD=wifi.urldecode(ssid);
//      String passD=wifi.urldecode(pass);
      String ssidD=wifi.urldecode(readStringEEPROM(EEPROM_SSID));
      String passD=wifi.urldecode(readStringEEPROM(EEPROM_PASS));
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
        sentData.VERSION_NUMBER=VERSION_NUMBER;
        sentData.wifiWorked=false;
        updateWorked=false;
        sendtask();
      }
      else if (WiFi.status() == WL_CONNECTED)
      {
        Serial.println("WiFi connected");//delete before prduction
        Serial.println("Checking for updates. Remember to set the URL!");//delete before prduction
        updateWorked=true; 
  //      updater.CheckAndUpdate();
        delay(30);
        } 
  }

}


void loop() {
   if (Serial.available() > 0) {//task that are recived from user
    stopSleep = Serial.readString().toInt() ;}
  if(waterMotor_AIN1.motorMode== true)
      testingMotorWaterState (); 
  if(autoIrrigateState == true){
    Serial.println("is enabel");
     autoIrrigateStateTestLoop();}
  if(irrigatePlantOptionTime != 0 ){
    if(millis()-irrigatePlantOptionsTimeCheck>=irrigatePlantOptionTime){
      irrigatePlantOptionTime=0;
      sentData.irrigatePlantWorking=false;
      waterMotor_AIN1.motorModeChange(false);
      sendMotorStartStopWorking ();
    }
  }  
      if((waterMotor_AIN1.motorMode== false)&&(millis()-timeAWakeStart>TIME_A_WAKE)&&(stopSleep==0)) {
      Serial.println(millis()-timeAWakeStart);// need to delet when it is started
      esp_deep_sleep_start();
      }
}

void eight_UpdateProgrem(){ 
    sentData.massgeSuccess=true;
    sentData.task=10;
    sendtask(); 
  if(receiveData.VERSION_NUMBER>=VERSION_NUMBER){
    tryUpdate=true;
    String macStrS=wifi.urldecode((String)macStr);
    writeStringEEPROM(EEPROM_hubMacAddress,macStrS);
    writeStringEEPROM(EEPROM_SSID,wifi.urldecode(receiveData.ssid));
    writeStringEEPROM(EEPROM_PASS,wifi.urldecode(receiveData.pass));
  }
//  if(receiveData.VERSION_NUMBER==VERSION_NUMBER){
//    tryUpdate=false;
//    sentData.task=8;
//    sentData.massgeSuccess=true;
//    sentData.VERSION_NUMBER=VERSION_NUMBER;
//    sentData.IsUpdating=false;
//    sendtask();
//  }
  
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
  stopSleep=1;
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
  motorReadingsBefore= waterSensor.readingResults();
  waterMotor_AIN1.motorModeChange(true);
  sendMotorStartStopWorking ();   
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
    Serial.print("hall test resulte after motore start: ");//delete before prduction
    waterMotor_AIN1.readingsAffterInsert(waterSensor.readingOneResult());
    delay(50);
    Serial.println(waterSensor.readingOneResult());//delete before prduction
    delay(50);//delete before prduction
    Serial.println(motorReadingsBefore-waterMotor_AIN1.showReadingsAffter());//delete before prduction
    if ((motorReadingsBefore-waterMotor_AIN1.showReadingsAffter()<motorCurrentSub)&&(motorReadingsBefore-waterMotor_AIN1.showReadingsAffter()>50))
    {
      waterMotor_AIN1.countCheackTimeLower++;
      if (waterMotor_AIN1.countCheackTimeLower>=waterMotor_AIN1.numberCheackTimeLower){
        waterMotor_AIN1.motorModeChange(false);
        Serial.println("motor Status 'off'");//delete before prduction
//        Serial.println(waterMotor_AIN1.showReadingsAffter());//delete before prduction
        delay(50);//delete before prduction
        waterMotor_AIN1.countCheackTimeLower=0;
        autoIrrigateState=false;
        waterSensor.writingState(false);
        irrigatePlantOptionTime=0;
        sendMotorStartStopWorking ();
      }       
    }
    //else if (waterMotor_AIN1.readingsBefore-waterMotor_AIN1.showReadingsAffter ()>(motorCurrentSub)){waterMotor_AIN1.countCheackTimeLower=0; }
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
   // moistureSensor.readingSetup();
  //  lightSensor.readingSetup();
    sentData.moistureStatus = moistureSensor.readingResultsPercent();
    sentData.lightStatus= lightSensor.readingResultsPercent();   
//    sentData.batteryStatus = batterySensor.readingResultsPercent();// need to creat 
    sendtask();
}
void four_autoIrrigateState () {
  if(receiveData.autoIrrigateState == false){
    autoIrrigateState=false;
    Serial.println("autoWatering disebel");//delete before prduction
  }
  if(receiveData.autoIrrigateState == true){
    motorCurrentSub=receiveData.motorCurrentSub;
    motorReadingsBefore= waterSensor.readingResults();//need to save it to the rtc memory
    autoIrrigateState=true;//need to save it to the rtc memory
    Serial.println("autoWatering enabel");//delete before prduction
  }
  sentData.autoIrrigateState= autoIrrigateState;
  sendMotorStartStopWorking ();
}//for tesk1
void autoIrrigateStateTestLoop() {  
    int timePass=millis();
    int timePass2=millis();
    int numberTests=10;
    
    Serial.println("working2");//delete before prduction
   if(timePass-timeLength > timeDelayWaterPump)
    { 
      waterMotor_AIN1.motorModeChange(true);
      Serial.println("watering");//delete before prduction
      if (timePass-timeLength> timeDelayWaterPump+2000)
        {
        waterMotor_AIN1.motorModeChange(false); 
        timeLength = timePass;
        }       
    }

  if(timePass2-timeLength2 > timedelay_hum)
    { 
      waterMotor_AIN1.motorModeChange(false); 
      soilMoistureDegree(moistureSensor.readingResultsParNumberTest(numberTests));//func that chacke the state of the soil  
      timeLength2=timePass2;
    }
 }
void soilMoistureDegree (int humAverage){//func that chacke the state of the soil
    Serial.println(humAverage);//delete before prduction
    if (humAverage>2900)
      {
       Serial.println(" :  soil is too dry needs special treatment"); //delete before prduction
       timeDelayWaterPump=60000;// one minut = 60000
       waterPumpOnTime=2000;
      }
    else if (humAverage<2900 && humAverage>2760)
      {
       Serial.println(" :  soil is dry"); //delete before prduction
       timeDelayWaterPump=3600000; // one hour
       waterPumpOnTime=5000;
      }
    else if(humAverage<2760)
      {
       Serial.println(" :  soil is moist no watering needed");  //delete before prduction
       timeDelayWaterPump=86400000;//24hr water delay
       waterPumpOnTime=5000;
      }
 } 
void six_motorStopStart() {

    if(receiveData.motorState== false){
      waterMotor_AIN1.motorModeChange(false);
      sendMotorStartStopWorking ();
    }
    if(receiveData.motorState == true){
      motorCurrentSub=receiveData.motorCurrentSub;//need to save it to the rtc memory
      motorReadingsBefore= waterSensor.readingResults();
      delay(50);
      waterMotor_AIN1.motorModeChange(true);
      delay(100);
      testingMotorWaterState ();  
      sendMotorStartStopWorking ();
     } 
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
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status){
  Serial.print("\r\nLast Packet Send Status:\t");
  Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
    switch (status){
  case ESP_NOW_SEND_SUCCESS:
     Serial.println("Success");
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
