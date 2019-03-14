/*  Author: Nate (Nathan) Diven
     Date: March 3, 2019

     Description: This program is made to turn the ESP32 into an HID Joystick/Gamepad.
     This one has 9

     Credits: Code was inspired from ESP32 HID examples
     from https://bit.ly/2IPKHK8 (ESP32_BLE_Arduino on github)
     Thank you to all of the people who made the libraries for me to use. Would've taken
     years for me to do it alone. This code is based off of HID_keyboard example.

     This code is for anyone to use or modify royalty free, but it is not gaurenteed to work.
*/

#include <BLEDevice.h>  //Libraries necessary for program
#include <BLEUtils.h>
#include <BLEServer.h>
#include "BLE2902.h"
#include "BLEHIDDevice.h"
#include "HIDTypes.h"
#include <driver/adc.h>

BLEHIDDevice* hid; //declare hid device
BLECharacteristic* input; //Characteristic that inputs button values to devices
BLECharacteristic* output; //Characteristic that takes input from client

//Stores if the board is connected or not
bool connected = false;

//input pins (16 toatal button avaliable, 1 analog input 0-255 but inputs can be changed)
#define BUTTON1 5
#define BUTTON2 18
#define BUTTON3 19  //You can use any pins on the board
#define BUTTON4 21  //but I don't recommend the ones that are used
#define ANALOG1 32 //for flashing or are input only.

//pin that goes high while there's a device connected
#define CONNECTED_LED_INDICATOR_PIN 2

//inputValues[0] is the first 8 buttons, [1] is the next 8, [2] is the analog input
//Each one of the bits represnets a button. 1 == pressed 0 == not pressed
uint8_t inputValues[3] = {0b00000000, 0b00000000, 0x0};

class MyCallbacks : public BLEServerCallbacks { //Class that does stuff when device disconects or connects
    void onConnect(BLEServer* pServer) {
      connected = true;
      Serial.println("Connected");
      BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(true);

      digitalWrite(CONNECTED_LED_INDICATOR_PIN, HIGH);
    }

    void onDisconnect(BLEServer* pServer) {
      connected = false;
      Serial.println("Disconnected");
      BLE2902* desc = (BLE2902*)input->getDescriptorByUUID(BLEUUID((uint16_t)0x2902));
      desc->setNotifications(false);
      
      digitalWrite(CONNECTED_LED_INDICATOR_PIN, LOW);
    }
};

class MyOutputCallbacks : public BLECharacteristicCallbacks {
    void onWrite(BLECharacteristic* me) {
      uint8_t* value = (uint8_t*)(me->getValue().c_str());
      //ESP_LOGI(LOG_TAG, "special keys: %d", *value);
    }
};

void taskServer(void*) {


  BLEDevice::init("GAME-CONTROLLER");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyCallbacks());

  hid = new BLEHIDDevice(pServer);
  input = hid->inputReport(1); // <-- input REPORTID from report map
  output = hid->outputReport(1); // <-- output REPORTID from report map

  output->setCallbacks(new MyOutputCallbacks());

  std::string name = "NANETIN";
  hid->manufacturer()->setValue(name);

  hid->pnp(0x02, 0xe502, 0xa111, 0x0210);
  hid->hidInfo(0x00, 0x02);

  BLESecurity *pSecurity = new BLESecurity();
  //  pSecurity->setKeySize();
  pSecurity->setAuthenticationMode(ESP_LE_AUTH_BOND);

  const uint8_t report[] = { //This is where the amount, type, and value range of the inputs are declared
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x09, 0x05, // USAGE (Gamepad)
    0xa1, 0x01, // COLLECTION (Application)
    0x85, 0x01, //   REPORT_ID (1)
    0x15, 0x00, // LOGICAL_MINIMUM (0)
    0x25, 0x01, // LOGICAL_MAXIMUM (1)
    0x35, 0x00, // PHYSICAL_MINIMUM (0)
    0x45, 0x01, // PHYSICAL_MAXIMUM (1)
    0x75, 0x01, // REPORT_SIZE (1)
    0x95, 0x10, // REPORT_COUNT (16)
    0x05, 0x09, // USAGE_PAGE (Button)
    0x19, 0x01, // USAGE_MINIMUM (Button 1)
    0x29, 0x10, // USAGE_MAXIMUM (Button 16)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0x05, 0x01, // USAGE_PAGE (Generic Desktop)
    0x26, 0xff, 0x00, // LOGICAL_MAXIMUM (255)
    0x46, 0xff, 0x00, // PHYSICAL_MAXIMUM (255)
    0x09, 0x31, // USAGE (Y)
    0x75, 0x08, // REPORT_SIZE (8)
    0x95, 0x01, // REPORT_COUNT (1)
    0x81, 0x02, // INPUT (Data,Var,Abs)
    0xc0 // END_COLLECTION
  };


hid->reportMap((uint8_t*)report, sizeof(report));
hid->startServices();

BLEAdvertising *pAdvertising = pServer->getAdvertising();
pAdvertising->setAppearance(HID_GAMEPAD);
pAdvertising->addServiceUUID(hid->hidService()->getUUID());
pAdvertising->start();
hid->setBatteryLevel(7);

ESP_LOGD(LOG_TAG, "Advertising started!");
delay(portMAX_DELAY);

};

void buttonOne(){
  if (digitalRead(BUTTON1) == 0){
    inputValues[0] |= 00000001;
  }
  else{
    inputValues[0] &= 11111110;
  }
}

void buttonTwo(){
  if (digitalRead(BUTTON2) == 0){
    inputValues[0] |= 00000010;
  }
  else{
    inputValues[0] &= 11111101;
  }
}

void buttonThree(){
  if (digitalRead(BUTTON3) == 0){
    inputValues[0] |= 00000100;
  }
  else{
    inputValues[0] &= 11111011;
  }
}

void buttonFour(){
  if (digitalRead(BUTTON4) == 0){
    inputValues[0] |= 00001000;
  }
  else{
    inputValues[0] &= 11110111;
  }
}

void setup() {
  Serial.begin(115200);

  pinMode(BUTTON1, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON1), buttonOne, CHANGE);

  pinMode(BUTTON2, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON1), buttonTwo, CHANGE);

  pinMode(BUTTON3, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON3), buttonThree, CHANGE);

  pinMode(BUTTON4, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(BUTTON4), buttonFour, CHANGE);

  xTaskCreate(taskServer, "server", 20000, NULL, 5, NULL);
}

void loop() {
  inputValues[2] = analogRead(ANALOG1);
  if (connected){
    input->setValue(inputValues, sizeof(inputValues));
    input->notify();
    delay(20);
  }

}
