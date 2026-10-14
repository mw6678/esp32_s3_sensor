#include <Arduino.h>
#include <DHT.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <BLE2902.h>

#define DHT_PIN 5       
#define WATER_PIN 6     
#define DHTTYPE DHT11   

DHT dht(DHT_PIN, DHTTYPE);

// 표준 36바이트 UUID 형식으로 수정
#define SERVICE_UUID "4fafc201-1fb5-459e-8fcc-c5c9c331914b"
#define CHARACTERISTIC_UUID "beb5483e-36e1-4688-b7f5-ea07361b26aa"

BLECharacteristic *pCharacteristic = nullptr;
bool deviceConnected = false;

class MyServerCallbacks : public BLEServerCallbacks {
    void onConnected(BLEServer* pServer) {
        deviceConnected = true;
        Serial.println("스마트폰 블루투스 연결됨!");
    }
    void onDisconnected(BLEServer* pServer) {
        deviceConnected = false;
        Serial.println("스마트폰 블루투스 연결 해제됨!");
    }
};

void setup() {
  Serial.begin(115200);
  delay(1000);
  
  dht.begin();
  pinMode(WATER_PIN, INPUT);

  BLEDevice::init("ESP32S3_Sensor_Box");
  BLEServer *pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  BLEService *pService = pServer->createService(SERVICE_UUID);
  pCharacteristic = pService->createCharacteristic(
                      CHARACTERISTIC_UUID,
                      BLECharacteristic::PROPERTY_READ   |
                      BLECharacteristic::PROPERTY_NOTIFY 
                    );
  
  pCharacteristic->addDescriptor(new BLE2902());

  pService->start();
  BLEAdvertising *pAdvertising = BLEDevice::getAdvertising();
  pAdvertising->addServiceUUID(SERVICE_UUID);
  pAdvertising->setScanResponse(true);
  BLEDevice::startAdvertising();
  
  Serial.println("BLE 기기 대기 중...");
}

void loop() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  int waterLevel = analogRead(WATER_PIN);

  if (isnan(humidity) || isnan(temperature)) {
    temperature = 0.0;
    humidity = 0.0;
  }

  String payload = "{";
  payload += "\"temp\":" + String(temperature, 1) + ",";
  payload += "\"humi\":" + String(humidity, 1) + ",";
  payload += "\"water\":" + String(waterLevel);
  payload += "}";

  Serial.println("측정 데이터: " + payload);

  if (deviceConnected && pCharacteristic != nullptr) {
    pCharacteristic->setValue(payload.c_str());
    pCharacteristic->notify();
  }

  delay(2000);
}