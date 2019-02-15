#include "TrynkitOTAFirmware.h"

int flag = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("[DEBUG] Start");
  // Init BLE
  initBLE();

  //find "app1" partition
  const char* partName = "app1";
  PART = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, partName);
}

void loop() {
  if (deviceConnected) {
    // bluetooth stack will go into congestion, if too many packets are sent
    if (!flag) {
      Serial.println(ESP.getFreeHeap());
      flag = 1;
    }
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
  Serial.println("[DEBUG] Init BLE");
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

void writeFirmware(uint32_t addr, uint32_t partSize) {
  Serial.println("[DEBUG] Write Firmware");
  esp_partition_erase_range(PART, addr, partSize); //erase "app1" partition
  esp_partition_write(PART, (addr - APP1_ADDR), (void*)&image, sizeof(image)); //write new firmware
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
  Serial.println("[DEBUG] Received Image");
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
      if(image.substring(image.length()-5, image.length()).equals("0x0ZC")) {
        Serial.println("[DEBUG] Received 0x0ZC");
        image = image.substring(0, image.length()-5);
        writeFirmware((APP1_ADDR + (UPDATE_SIZE * updateCount)), sizeof(image));
        //check if chunk is corrupt
        transmitOut("0x0FS");
        return;
      }
      //final block to be written
      else if (image.substring(image.length()-5, image.length()).equals("0x0ZD")) {
        Serial.println("[DEBUG] Received 0x0ZD");
        image = image.substring(0, image.length()-5);
        writeFirmware((APP1_ADDR + (UPDATE_SIZE * updateCount)), sizeof(image));
        //check if chunk is corrupt
        transmitOut("0x0FS");
        //jump to second partition
      }
    }

    // Check for commands
    if(input.equals("0x0FA")) { // Flash via BLE
      Serial.println("[DEBUG] Received 0x0FA");
      receiveImage = true;
    }
  }
}

void ReceiveCallBack::transmitOut(char* output) {
  Serial.println("[DEBUG] Transmit Out");
  //memcpy(out_buff, output, sizeof(output));
  for(int i = 0; i < sizeof(out_buff); i++) {
    out_buff[i] = uint8_t(output[i]);
  }
  transmit = true;
}
