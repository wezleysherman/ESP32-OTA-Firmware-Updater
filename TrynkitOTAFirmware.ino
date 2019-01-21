#include <BLEDevice.h>
#include <BLE2902.h>
#include <BLEServer.h>
#include <esp_partition.h>
#include "ArduinoJson.h"
#include "ReceiveCallBack.h"
#include "TrynkitOTAFirmware.h"

void setup() {
  Serial.begin(115200);

  // Init BLE
  initBLE();
}

void loop() {
  if (deviceConnected) {
    // bluetooth stack will go into congestion, if too many packets are sent
    if(transmit == true) {
      pTxCharacteristic->setValue(out_buff, 5);
      pTxCharacteristic->notify();
      delay(10);
      uint8_t empty[6];
      transmit = false;
      memcpy(out_buff, empty, sizeof(empty));
    }
  }

  // disconnecting
  if (!deviceConnected && oldDeviceConnected) {
    delay(500); // give the bluetooth stack the chance to get things ready
    pServer->startAdvertising(); // restart advertising
    oldDeviceConnected = deviceConnected;
    image = "";
  }
  
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    image = "";
    oldDeviceConnected = deviceConnected;
  }
}

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

void flashFirmware(const char* partName) {
  //find "app1" partition
  const esp_partition_t* part = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, partName);
  if (part == NULL)
    return;
}

void writeFirmware(esp_partition_t* part, uint32_t addr, uint32_t partSize) {
  esp_partition_erase_range(part, addr, partSize); //erase "app1" partition
  esp_partition_write(part, (addr - APP1_ADDR), (void*)&image, sizeof(image)); //write new firmware
}

void reset() {
  ESP.restart();
}

void MyServerCallbacks::onConnect(BLEServer* pServer) {
  deviceConnected = true;
}

void MyServerCallbacks::onDisconnect(BLEServer* pServer) {
  deviceConnected = false;
}

// Handles BLE receives for images and flashing
void ReceiveCallBack::onWrite(BLECharacteristic *pCharacteristic) {
  std::string rxVal = pCharacteristic->getValue();
  String input = "";
  if(rxVal.length() > 0) {
    for(int i = 0; i < rxVal.length(); i++) {
      if(i % 2 == 0) {
        input += rxVal[i];
      }
    }

    // BLE Flashing mode
    if(receiveImage == true) {
      image += input;
      if(image.substring(image.length()-5, image.length()).equals("0x0FC")) {
        transmitOut("0x0FC");
        image = image.substring(0, image.length()-5);
        //write to second partition
        //jump to second partition
        return;
      }
    }

    // Check for commands
    if(input.equals("0x0FA")) { // Flash via BLE
      receiveImage = true;
    }
  }
}

void ReceiveCallBack::transmitOut(char* output) {
  //Serial.println(output);
  //memcpy(out_buff, output, sizeof(output));
  for(int i = 0; i < sizeof(out_buff); i++) {
    out_buff[i] = uint8_t(output[i]);
  }
  transmit = true;
}
