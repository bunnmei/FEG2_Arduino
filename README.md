# Arduino Max31856 BLE

## ライブラリ

ArduinoBLE.hとAdafruit_MAX31856.hはライブラリマネージャーからインストールする必要があります。

[ArduinoBLE.h](https://github.com/arduino-libraries/ArduinoBLE) \
[Adafruit_MAX31856.h](https://github.com/adafruit/Adafruit_MAX31856/tree/master)

## 接続

### Arduino MAX31856

一つ目の温度計は \
`CS  -> 10` \
`SDI -> 11` \
`SDO -> 12` \
`CLK -> 13` 

二つ目の温度計は \
`CS  -> 9` \
`SDI -> 11` \
`SDO -> 12` \
`CLK -> 13` 

### 熱電対

max31856モジュールのVINピンがある側が赤で、
DRDYピンがあるほうが青です。

### 注意

ブレッドボードでの接続では不安定になりやすです。
接続後スマホで0.0℃しか出ない場合接続が不安定な可能性が高いです。
(Arduino <-> max31856)の問題。

接続後スマホで1372.1℃が出てくる場合max31856と熱電対の間の未接続か接続が不安定の可能性高いです。
(max31856 <-> 熱電対)の問題


