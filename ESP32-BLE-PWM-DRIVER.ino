/*
    Based on Neil Kolban example for IDF: https://github.com/nkolban/esp32-snippets/blob/master/cpp_utils/tests/BLE%20Tests/SampleServer.cpp
    Ported to Arduino ESP32 by Evandro Copercini
*/

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>

// See the following for generating UUIDs:
// https://www.uuidgenerator.net/

#define SERVICE_UUID        "a1d4c6e9-a076-45ae-a48e-b6b2ecf38f46"
#define CHARACTERISTIC_UUID_left "8b82764b-26d4-4adc-90f0-d28ff488ea62"
#define CHARACTERISTIC_UUID_right "81fa7d09-5cdc-4d2c-b3db-35592cd1e366"


//#define waitserial true


//setup hardware and hardware control
#define gpio_pwm_left 1
#define gpio_pwm_right 2
#define btn1 3
#define btn2 4
#define btn3 5
#define btn4 6
int pwm_left = 0;
int pwm_right = 0;

// setting PWM properties
const int freq = 5000;
const int ledChannel = 0;
const int ledChanne2 = 1;
const int resolution = 8;



//Setup callbacks onConnect and onDisconnect
bool Connected = false;

class MyServerCallbacks: public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    Serial.println("connected");
    Connected = true;
  };
  void onDisconnect(BLEServer* pServer) {
    Serial.println("disconnected");
    Connected = false;
  }
};

class MyCharacteristicCallBacks : public BLECharacteristicCallbacks
{
public:
  //This method not called
  void onWrite(BLECharacteristic *pCharacteristic) override
  {

    uint32_t int_val;
    memcpy(&int_val, pCharacteristic->getData(), sizeof(int_val));
    if(int_val > 254)
    {
      return;
    }
    if(pCharacteristic->getUUID().toString().equals(CHARACTERISTIC_UUID_left) )
    {
      pwm_left = int_val;
      ledcWrite(gpio_pwm_left, pwm_left);
    }
    if(pCharacteristic->getUUID().toString().equals(CHARACTERISTIC_UUID_right) )
    {
      pwm_right = int_val;
      ledcWrite(gpio_pwm_right, pwm_right);
    }
    
    
  }
};

enum channel
{
  left,
  right
};

int btn_pushed = 0;

void IRAM_ATTR btn1_interrupt()
{
   //Serial.println("you pushed button 1");
  btn_pushed = btn1;
}
void IRAM_ATTR btn2_interrupt()
{
   //Serial.println("you pushed button 2");
  btn_pushed = btn2;
}
void IRAM_ATTR btn3_interrupt()
{
  //Serial.println("you pushed button 3");
  btn_pushed = btn3;
}
void IRAM_ATTR btn4_interrupt()
{
   //Serial.println("you pushed button 4");
  btn_pushed = btn4;
}


void setup() {
  Serial.begin(115200);

  #ifdef waitserial
  while(!Serial)
  {
    delay(10);
  }
  #endif

  Serial.println("Starting BLE work!");

  BLEDevice::init("PWM Controller");
  /*
   * Required in authentication process to provide displaying and/or input passkey or yes/no butttons confirmation
   */
  //create the server
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());
  //create the service
  BLEService *pService = pServer->createService(SERVICE_UUID);
  //create our 2 properties
  BLECharacteristic *pCharist_left = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_left,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  pCharist_left->setValue(pwm_left);
  pCharist_left->setCallbacks(new MyCharacteristicCallBacks());

  BLECharacteristic *pCharist_right = pService->createCharacteristic(
                                         CHARACTERISTIC_UUID_right,
                                         BLECharacteristic::PROPERTY_READ |
                                         BLECharacteristic::PROPERTY_WRITE |
                                         BLECharacteristic::PROPERTY_NOTIFY
                                       );
  pCharist_right->setValue(pwm_right);
  pCharist_right->setCallbacks(new MyCharacteristicCallBacks());


  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  pAdvertising->setMinPreferred(0x06);  // functions that help with iPhone connections issue
  pAdvertising->setMinPreferred(0x12);
  

  Serial.println("Characteristic defined! Now you can read it in your phone!");

    // configure LED PWM functionalitites
  ledcAttachChannel(gpio_pwm_left, freq, resolution, ledChannel);
  ledcAttachChannel(gpio_pwm_right, freq, resolution, ledChanne2);
  ledcWrite(gpio_pwm_left, 0);
  ledcWrite(gpio_pwm_right, 0);

  int btn_pins[] = {btn1,btn2,btn3,btn4};
  for (int i = 0; i < sizeof(btn_pins)/sizeof(btn_pins[0]); i++)
  {
    pinMode(btn_pins[i],INPUT_PULLUP);
  }
  attachInterrupt(digitalPinToInterrupt(btn1), btn1_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(btn2), btn2_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(btn3), btn3_interrupt, FALLING);
  attachInterrupt(digitalPinToInterrupt(btn4), btn4_interrupt, FALLING);


}

void loop() {
  if(btn_pushed != 0)
  {
    buttonpush(btn_pushed);
    btn_pushed = 0;
  }

}



void buttonpush(int btnid)
{
  Serial.print("you pushed button ");
  Serial.println(btnid);
  //first give some time to debounce, and push the second pin
  delay(500);

  //lets check if we are trying to go into pairing mode
  if(digitalRead(btn1) == LOW && digitalRead(btn2) == LOW )
  {
    Serial.println("bluetoothe enabled");
    //we want to enable pairing
    BLEDevice::startAdvertising();
    //alow some long debouce so no other buttons get registered
    delay(500);
    return;
  }

  //we are not trying to pair so lets check what button got pushed and what we should do if the button is being held down.
  switch (btnid)
  {
    case btn1:
    {
      adjustheat_stepped(btn1,false,channel::right);
      break;
    }
    case btn2:
    {
      adjustheat_stepped(btn2,false,channel::left);
      break;
    }
    case btn3:
    {
      adjustheat_stepped(btn3,true,channel::right);
      break;
    }
    case btn4:
    {
      adjustheat_stepped(btn4,true,channel::left);
      break;
    }
  }
}

void adjustheat_stepped(int pin, bool up, channel side)
{
  int step = 25;
  if(up == false)
  {
    step = step * -1;
  }
  if(side == channel::left)
  {
    adjustint_withoutoverflow(&pwm_left, step);
    Serial.print("left is now ");
    Serial.println(pwm_left);
    while(digitalRead(pin) == LOW)
    {
      delay(500);
      adjustint_withoutoverflow(&pwm_left, step);
      ledcWrite(gpio_pwm_left, pwm_left);
      Serial.print("left is now ");
      Serial.println(pwm_left);
    }
  }
  if(side == channel::right)
  {
    adjustint_withoutoverflow(&pwm_right, step);
    Serial.print("right is now ");
    Serial.println(pwm_right);
    while(digitalRead(pin) == LOW)
    {
      delay(500);
      adjustint_withoutoverflow(&pwm_right, step);
      ledcWrite(gpio_pwm_right, pwm_right);
      Serial.print("right is now ");
      Serial.println(pwm_right);
    }
  }
  ledcWrite(gpio_pwm_right, pwm_right);
  ledcWrite(gpio_pwm_left, pwm_left);
}

void adjustint_withoutoverflow(int *adj, int adjustment)
{
  if(*adj + adjustment > 254)
  {
    *adj = 254;
    return;
  }
  if(*adj + adjustment < 0)
  {
    *adj = 0;
    return;
  }
  *adj = *adj + adjustment;
}




