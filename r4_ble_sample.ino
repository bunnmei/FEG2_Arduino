#include <ArduinoBLE.h>
#include <EEPROM.h>

#include <Adafruit_MAX31856.h>

#include <string>

const char firmwwareVersion[] = "1.0.0";

//温度計のサービスUUID
const char * deviceServiceUuid = "811b864d-718c-b035-804d-92c4761243c0";

//温度データをやり取りするキャラクタリスティックのUUID
const char * tempF_ResponseCharacteristicUuid = "0601a001-0eca-b5ab-edc2-887fd5f32b84";
const char * tempS_ResponseCharacteristicUuid = "620321e5-17c6-ca12-8328-7db87b02fbe5";
//7セグの輝度、温度のキャリブレーションのUUID
const char * brightnessCharacteristicUuid = "8a959d93-3d42-0c75-eaa3-3b37f5275fff";
const char * tempF_CaribCharacteristicUuid = "324010a6-eede-268f-8c88-c5632308eb41";
const char * tempS_CaribCharacteristicUuid = "04525bfa-46e2-3540-1174-afaba112e062";

//サービスとキャラクタを設定していく
BLEService tempService(deviceServiceUuid);
BLEFloatCharacteristic tempF_ResponseCharacteristic(tempF_ResponseCharacteristicUuid, BLENotify);
BLEFloatCharacteristic tempS_ResponseCharacteristic(tempS_ResponseCharacteristicUuid, BLENotify);

BLEByteCharacteristic brightnessCharacteristic(brightnessCharacteristicUuid, BLERead | BLEWrite);
BLEByteCharacteristic tempF_Characteristic(tempF_CaribCharacteristicUuid, BLERead | BLEWrite);
BLEByteCharacteristic tempS_Characteristic(tempS_CaribCharacteristicUuid, BLERead | BLEWrite);


//デバイス情報サービスのUUID
BLEService deviceInfoService("180A");
BLEStringCharacteristic firmwareRevChar("2A26",  // UUID
                                        BLERead, // 読み取り専用
                                        20);

const int ADDR_MAGIC = 0;  // 初期化済みか確認するフラグ
const int ADDR_CALIB_F = 1;  // 値を保存するアドレス
const int ADDR_CALIB_S = 2;
const int ADDR_BRIGHTNESS = 3;

int8_t brightness = 5;
int8_t calibF_value = 0; //-50 ~ 50
int8_t calibS_value = 0; //-50 ~ 50

// Use software SPI:                           CS, DI, DO, CLK
Adafruit_MAX31856 maxthermo = Adafruit_MAX31856(10, 11, 12, 13);
Adafruit_MAX31856 maxthermo2 = Adafruit_MAX31856(9, 11, 12, 13);

void setup() {
  
  Serial.begin(115200);
  eeprom_check();

  maxthermo.begin();
  maxthermo.setThermocoupleType(MAX31856_TCTYPE_K);

  maxthermo2.begin();
  maxthermo2.setThermocoupleType(MAX31856_TCTYPE_K);

  BLE.setLocalName("FEG2"); //アドバタイズで見える名前

  if(!BLE.begin()) {
    Serial.println("BLE Start failed!");
    while(1);
  }
  BLE.setDeviceName("FEG2_A"); //接続後に確認できる名前A=Arduino, P=Pico(raspberry pi pico) E=ESP32

  BLE.setAdvertisedService(tempService);

  //温度計サービスの登録
  tempService.addCharacteristic(tempF_ResponseCharacteristic);
  tempService.addCharacteristic(tempS_ResponseCharacteristic);
  tempService.addCharacteristic(brightnessCharacteristic);
  tempService.addCharacteristic(tempF_Characteristic);
  tempService.addCharacteristic(tempS_Characteristic);

  BLE.addService(tempService);

  //デバイス情報のサービスを登録
  deviceInfoService.addCharacteristic(firmwareRevChar);
  BLE.addService(deviceInfoService);

  //firmwareのバージョンを書き込み
  firmwareRevChar.writeValue(firmwwareVersion);

  //温度計のキャラの呼び出し時のコールバックを登録
  brightnessCharacteristic.setEventHandler(BLEWritten, onWirteCallBack);
  tempF_Characteristic.setEventHandler(BLEWritten, onWirteCallBack);
  tempS_Characteristic.setEventHandler(BLEWritten, onWirteCallBack);

  brightnessCharacteristic.setEventHandler(BLERead, onReadCallBack);
  tempF_Characteristic.setEventHandler(BLERead, onReadCallBack);
  tempS_Characteristic.setEventHandler(BLERead, onReadCallBack);

  //アドバタイズ開示
  BLE.advertise();

  Serial.println("Arduino R4 WiFi BLE (Peripheral Device)");
  Serial.println(" ");
}

void loop() {
  // put your main code here, to run repeatedly:
  BLEDevice central = BLE.central();
  Serial.println("- Discovering central device...");
  delay(500);

  //接続後開始される
  if(central) {
    Serial.println("* Connected to central device!");
    Serial.print("* Device MAC address: ");
    Serial.println(central.address());
    Serial.println(" ");

    //接続後1秒おきに実行される。
    while (central.connected()) {
      BLE.poll(); //READ,WRITEなどがあるかチェックしてくれるみたい。あれば登録しておいたコールバックが呼ばれる
      float value = maxthermo.readThermocoupleTemperature() + validate(calibF_value);
      float value2 = maxthermo2.readThermocoupleTemperature() + validate(calibS_value);
      
      printf("temp %0.2f", value);
      tempF_ResponseCharacteristic.setValue(value);
      tempS_ResponseCharacteristic.setValue(value2);
      delay(1000);
    }

    Serial.println("* Disconnected to central device!");
  }

}

//キャリブレーションは-50 ~ 50の値で実際には/10して、-5.0℃ ~ 5.0℃に変換するための関数
float validate(int8_t calib){
  return (float)(calib) / 10;
}

void onWirteCallBack(BLEDevice central, BLECharacteristic characteristic) {
  if(characteristic.uuid() == brightnessCharacteristic.uuid()) {
    int8_t v = brightnessCharacteristic.value();
    Serial.print("brightness updated: ");
    Serial.println(v);
    brightness = eeprom_calib_write_valid(v);
  } 
  if (characteristic.uuid() == tempF_Characteristic.uuid()) {
    int8_t v = tempF_Characteristic.value();
    Serial.print("Calib_F updated: ");
    Serial.println(v);
    calibF_value = eeprom_calib_write_valid(v, ADDR_CALIB_F);

  } 
  if (characteristic.uuid() == tempS_Characteristic.uuid()) {
    int8_t v = tempS_Characteristic.value();
    Serial.print("Calib_S updated: ");
    Serial.println(v);
    calibS_value = eeprom_calib_write_valid(v, ADDR_CALIB_S);
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

int8_t eeprom_calib_write_valid(int8_t receValue, int addr) {
  
  int8_t newValue = receValue;
  if(receValue > 50) {
    newValue = 50;
  } else if (receValue < -50) {
    newValue = -50;
  }

  EEPROM.write(addr, newValue);

  return newValue;
}

int8_t eeprom_calib_write_valid(int8_t receValue) {

  int8_t newValue = receValue;
  if(receValue > 8) {
    newValue = 8;
  } else if (receValue < 0) {
    newValue = 0;
  }

  EEPROM.write(ADDR_BRIGHTNESS, newValue);

  return newValue;
}

void eeprom_check() {
  const byte MAGIC = 0x42;
  if (EEPROM.read(ADDR_MAGIC) != MAGIC) {
    // 初回起動 → 初期値を書き込む
    int8_t calibF_DefaultValue = 0;
    int8_t calibS_DefaultValue = 0;
    int8_t brightnessDefaultValue = 3;  
    EEPROM.write(ADDR_CALIB_F, calibF_DefaultValue);
    EEPROM.write(ADDR_CALIB_S, calibS_DefaultValue);
    EEPROM.write(ADDR_BRIGHTNESS, brightnessDefaultValue);
    EEPROM.write(ADDR_MAGIC, MAGIC);

    Serial.println("init eeprom");
  }

  
  calibF_value = EEPROM.read(ADDR_CALIB_F);
  calibS_value = EEPROM.read(ADDR_CALIB_S);
  brightness = EEPROM.read(ADDR_BRIGHTNESS);

}

//タミーの値をランダムに生成し、BLEの接続可能かチェックするための関数
// float genFloat() {
//    // 1. 範囲を整数にスケーリング
//   // 1300.00 - (-200.00) = 1500.00。範囲を整数で表現するため、100倍して150000とする。
//   long range_scaled = 150000;
  
//   // 2. 範囲をずらすためのオフセット
//   // -200.00を100倍して-20000とする。
//   long offset_scaled = -20000;

//   // 3. スケーリングされた範囲内でランダムな整数を生成
//   long random_scaled = random(range_scaled + 1);
  
//   // 4. オフセットを加えて最終的なスケーリングされた値を計算
//   long result_scaled = random_scaled + offset_scaled;

//   // 5. 100で割ってフロート値に戻す
//   float random_float = (float)result_scaled / 100.0;

//   Serial.print("Generated random float: ");
//   Serial.println(random_float, 2); // 小数点以下2桁で表示

//   return random_float;
// }


