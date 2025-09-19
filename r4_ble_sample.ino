#include <ArduinoBLE.h>
#include <string>

const char * deviceServiceUuid = "811b864d-718c-b035-804d-92c4761243c0";

const char * tempF_ResponseCharacteristicUuid = "0601a001-0eca-b5ab-edc2-887fd5f32b84";
const char * tempS_ResponseCharacteristicUuid = "620321e5-17c6-ca12-8328-7db87b02fbe5";

const char * brightnessCharacteristicUuid = "8a959d93-3d42-0c75-eaa3-3b37f5275fff";
const char * tempF_CaribCharacteristicUuid = "324010a6-eede-268f-8c88-c5632308eb41";
const char * tempS_CaribCharacteristicUuid = "04525bfa-46e2-3540-1174-afaba112e062";

BLEService tempService(deviceServiceUuid);
BLEFloatCharacteristic tempF_ResponseCharacteristic(tempF_ResponseCharacteristicUuid, BLENotify);
BLEFloatCharacteristic tempS_ResponseCharacteristic(tempS_ResponseCharacteristicUuid, BLENotify);

BLEByteCharacteristic brightnessCharacteristic(brightnessCharacteristicUuid, BLERead | BLEWrite);
BLEByteCharacteristic tempF_Characteristic(tempF_CaribCharacteristicUuid, BLERead | BLEWrite);
BLEByteCharacteristic tempS_Characteristic(tempS_CaribCharacteristicUuid, BLERead | BLEWrite);

int8_t brightness = 5;
int8_t calibF_value = 0; //-50 ~ 50
int8_t calibS_value = 0; //-50 ~ 50

void setup() {
  
  Serial.begin(115200);

  BLE.setDeviceName("FEG2_Arduino_v1.0.0");
  BLE.setLocalName("FEG2");
  

  if(!BLE.begin()) {
    Serial.println("BLE Start failed!");
    while(1);
  }

  BLE.setAdvertisedService(tempService);
  tempService.addCharacteristic(tempF_ResponseCharacteristic);
  tempService.addCharacteristic(tempS_ResponseCharacteristic);
  tempService.addCharacteristic(brightnessCharacteristic);
  tempService.addCharacteristic(tempF_Characteristic);
  tempService.addCharacteristic(tempS_Characteristic);


  BLE.addService(tempService);

  brightnessCharacteristic.setEventHandler(BLEWritten, onWirteCallBack);
  tempF_Characteristic.setEventHandler(BLEWritten, onWirteCallBack);
  tempS_Characteristic.setEventHandler(BLEWritten, onWirteCallBack);

  brightnessCharacteristic.setEventHandler(BLERead, onReadCallBack);
  tempF_Characteristic.setEventHandler(BLERead, onReadCallBack);
  tempS_Characteristic.setEventHandler(BLERead, onReadCallBack);

  BLE.advertise();

  Serial.println("Arduino R4 WiFi BLE (Peripheral Device)");
  Serial.println(" ");
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();
  Serial.println("- Discovering central device...");
  delay(500);

  if(central) {
    Serial.println("* Connected to central device!");
    Serial.print("* Device MAC address: ");
    Serial.println(central.address());
    Serial.println(" ");

    while (central.connected()) {
      BLE.poll();
      float value = genFloat();
      float value2 = genFloat();
      tempF_ResponseCharacteristic.setValue(value);
      tempS_ResponseCharacteristic.setValue(value2);
      delay(1000);
    }

    Serial.println("* Disconnected to central device!");
  }

}

void onWirteCallBack(BLEDevice central, BLECharacteristic characteristic) {
  if(characteristic.uuid() == brightnessCharacteristic.uuid()) {
    int8_t v = brightnessCharacteristic.value();
    Serial.print("brightness updated: ");
    Serial.println(v);

  } else if (characteristic.uuid() == tempF_Characteristic.uuid()) {
    int8_t v = tempF_Characteristic.value();
    Serial.print("Calib_F updated: ");
    Serial.println(v);

  } else if (characteristic.uuid() == tempS_Characteristic.uuid()) {
    int8_t v = tempS_Characteristic.value();
    Serial.print("Calib_S updated: ");
    Serial.println(v);
  }
}

void onReadCallBack(BLEDevice central, BLECharacteristic characteristic) {

  if(characteristic.uuid() == brightnessCharacteristic.uuid()) {
    
    Serial.println("read brightness");
    brightnessCharacteristic.writeValue(brightness);
  } else if (characteristic.uuid() == tempF_Characteristic.uuid()) {
    
    Serial.println("read calib_f");
    tempF_Characteristic.writeValue(calibF_value);
  } else if (characteristic.uuid() == tempS_Characteristic.uuid()) {
    
    Serial.println("read calib_s");
    tempS_Characteristic.writeValue(calibS_value);
  }

}


float genFloat() {
   // 1. 範囲を整数にスケーリング
  // 1300.00 - (-200.00) = 1500.00。範囲を整数で表現するため、100倍して150000とする。
  long range_scaled = 150000;
  
  // 2. 範囲をずらすためのオフセット
  // -200.00を100倍して-20000とする。
  long offset_scaled = -20000;

  // 3. スケーリングされた範囲内でランダムな整数を生成
  long random_scaled = random(range_scaled + 1);
  
  // 4. オフセットを加えて最終的なスケーリングされた値を計算
  long result_scaled = random_scaled + offset_scaled;

  // 5. 100で割ってフロート値に戻す
  float random_float = (float)result_scaled / 100.0;

  Serial.print("Generated random float: ");
  Serial.println(random_float, 2); // 小数点以下2桁で表示

  return random_float;
}
