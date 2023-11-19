#ifdef ARDUINO_ARCH_ESP32
#include <WiFi.h>
#else
#include <ESP8266WiFi.h>
#endif
#include <Espalexa.h>

#define AC_RELAY_PIN 15 //D8
#define RP_RELAY_PIN 5 //D1
#define LED_MOSFET_PIN 4 //D2


#define CLOSED_SENS_PIN 12 //D6 
#define OPEN_SENS_PIN 14 //D5 
#define BUTTON_PIN 0//D3 yellow

#define OPERATION_TIMEOUT 4000

#define LED_ON_DELAY 3500
#define LED_OFF_DELAY 2500
#define LED_BEFORE_ON_DELAY 500

enum st{CLSED, OPNING, OPN, CLSING, FLID};

enum st state;

void turnLEDon();
void turnLEDoff();

void(* resetFunc) (void) = 0;
boolean connectWifi();

// WiFi Credentials
const char* ssid = "DHome";
const char* password = "23332330";
String Device_1_Name = "Wardrobe";


void firstLightChanged(uint8_t brightness);
boolean connectWifi();

Espalexa espalexa;
//boolean wifiConnected = false;

void setup() {

  Serial.begin(9600);

  pinMode(AC_RELAY_PIN, OUTPUT);
  pinMode(RP_RELAY_PIN, OUTPUT);
  pinMode(LED_MOSFET_PIN, OUTPUT);

  digitalWrite(AC_RELAY_PIN, LOW);
  digitalWrite(RP_RELAY_PIN, LOW);
  digitalWrite(LED_MOSFET_PIN, LOW);

  pinMode(CLOSED_SENS_PIN, INPUT_PULLUP);
  pinMode(OPEN_SENS_PIN, INPUT_PULLUP);
  pinMode(BUTTON_PIN, INPUT_PULLUP);

  state = CLSED;

  pinMode(16, OUTPUT);




  if (connectWifi()){
    
    espalexa.addDevice(Device_1_Name, firstLightChanged); //simplest definition, default state off
    espalexa.begin();
  }
  else{
    
    Serial.println("Cannot connect to WiFi.");
    delay(1000);

    resetFunc();
  }

  Serial.println("READY");
}


unsigned long lastStateUpdateTime = 0;

void loop() {


  //WiFi Control
  espalexa.loop();
  delay(1);

  if(millis() - lastStateUpdateTime >= 100){
    
  
    if(digitalRead(OPEN_SENS_PIN) == LOW)
      state = OPN;
  
    if(digitalRead(CLOSED_SENS_PIN) == LOW)
      state = CLSED;

    lastStateUpdateTime = millis();
  }

  //Serial.print(digitalRead(OPEN_SENS_PIN));   Serial.print("     ");   Serial.println(digitalRead(CLOSED_SENS_PIN));

//  Serial.println(state);
  
  switch(state){

    case CLSED : {
      
      turnLEDoff();
      
      if(digitalRead(BUTTON_PIN) == LOW)
        state = OPNING;
        
      break;
    }


    case OPNING :{
      
      Serial.println("OPENING");

      digitalWrite(AC_RELAY_PIN, HIGH);

      unsigned long timer = millis();
      
      while(digitalRead(OPEN_SENS_PIN) == HIGH && millis() - timer <= OPERATION_TIMEOUT){

        turnLEDon();
        delay(5);
      }

      digitalWrite(AC_RELAY_PIN, LOW);

      if(millis() - timer >= OPERATION_TIMEOUT){
        
        state = FLID; 
        Serial.println("Failed opening");
        break;
      }

      Serial.println("OPENED");

      state = OPN;
      
      break;
    }

     case OPN :{
      
      turnLEDon();
      
      if(digitalRead(BUTTON_PIN) == LOW){

        state = CLSING;
      }

      
      break;
     }


     case CLSING : {
      
      Serial.println("CLOSING");

      digitalWrite(RP_RELAY_PIN, HIGH);
      digitalWrite(AC_RELAY_PIN, HIGH);

      unsigned long timer = millis();
      
      while(digitalRead(CLOSED_SENS_PIN) == HIGH && millis() - timer <= OPERATION_TIMEOUT){

        turnLEDoff();
        delay(10);
        //Serial.print(millis() - timer); Serial.print("   "); Serial.println(digitalRead(CLOSED_SENS_PIN));
      }

      digitalWrite(AC_RELAY_PIN, LOW);
      digitalWrite(RP_RELAY_PIN, LOW);


      if(millis() - timer >= OPERATION_TIMEOUT){
        
        state = FLID; 
        Serial.println("Failed closing");
        break;
      }


      Serial.println("CLOSED");

      state = CLSED;

      break;
     }


     case FLID : {

      delay(5000);

      resetFunc();
      
      break;
     }
  }


  if(state == CLOSED && millis() >= 4294967000) //reset
    resetFunc();

}


byte ledBrightness = 0;
bool changingState = 0;
unsigned long changeStTime = 0;

void turnLEDon(){

  if(ledBrightness == 255){

    changingState = false;
//    Serial.print(millis() - changeStTime);   Serial.print("  brightness exit  ");   Serial.println(ledBrightness);   

    return;
  }
  
  if(!changingState){

    changingState = true;
    changeStTime = millis();
  }


  if(millis() - changeStTime < LED_BEFORE_ON_DELAY)
    return;

  
  if(millis() - changeStTime >= LED_ON_DELAY + LED_BEFORE_ON_DELAY){

    changingState = false;
    
//    Serial.print(millis() - changeStTime);   Serial.print("  time exit  ");   Serial.println(ledBrightness);   

    ledBrightness = 255;
    analogWrite(LED_MOSFET_PIN, ledBrightness);
    
    return;    
  }


  ledBrightness = map(millis() - changeStTime - LED_BEFORE_ON_DELAY, 0, LED_ON_DELAY, 0, 255);

  analogWrite(LED_MOSFET_PIN, ledBrightness);
}


void turnLEDoff(){

  if(ledBrightness == 0){ //brightness reached exit

    changingState = false;
    
//    Serial.print(millis() - changeStTime);   Serial.print("  brightness exit  ");   Serial.println(ledBrightness);   

    return;
  }
  
  if(!changingState){ //initialize

    changingState = true;
    changeStTime = millis();
  }

  if(millis() - changeStTime >= LED_OFF_DELAY){ //time reached exit
    
    ledBrightness = 0;
    analogWrite(LED_MOSFET_PIN, ledBrightness);
 
//    Serial.print(millis() - changeStTime);   Serial.print("  time exit  ");   Serial.println(ledBrightness);   

    changingState = false;
    
    return;    
  }


  ledBrightness = map(millis() - changeStTime, 0, LED_OFF_DELAY, 255, 0);

//  Serial.print(millis() - changeStTime);   Serial.print("   ");   Serial.println(ledBrightness);   

  analogWrite(LED_MOSFET_PIN, ledBrightness);
}
//enum st{CLSED, OPNING, OPN, CLSING, FLID};


void firstLightChanged(uint8_t brightness)
{
  //Control the device
  if (brightness == 255)
    {
      if(state == CLSED)
        state = OPNING;
        
      //digitalWrite(RelayPin1, LOW);
      Serial.println("Device1 ON");
    }
  else
  {
    //digitalWrite(RelayPin1, HIGH);
    Serial.println("Device1 OFF");

    if(state == OPN)
      state = CLSING;
      
  }
}


boolean connectWifi()
{
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting...");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20) {
      state = false; break;
    }
    i++;
  }
  Serial.println("");
  if (state) {
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else {
    Serial.println("Connection failed.");
  }
  return state;
}
