#include <BLEDevice.h>
#include <BLE2902.h>
#include <BLEServer.h>
#include "ArduinoJson.h"
#include "ReceiveCallBack.h"

#define SERVICE_UUID           "0000ffe0-0000-1000-8000-00805f9b34fb" // UART service UUID
#define CHARACTERISTIC_UUID_RX "80c78362-e3f9-11e8-9f32-f2801f1b9fd1"
#define CHARACTERISTIC_UUID_TX "80c784ac-e3f9-11e8-9f32-f2801f1b9fd1"

// Global Variables
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;

void initBLE() {
  BLEDevice::init("Trynkit Husky");
  pServer = BLEDevice::createServer();
  // Set up callback function for whenever something is received.
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new ReceiveCallBack());
  pService->start();
  pServer->getAdvertising()->start();
}

void deinitBLE() {
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
}

void reset() {
  ESP.restart();
}

void setup() {
  Serial.begin(115200);

  // Init BLE
  initBLE();
}

void loop() {
  
}
