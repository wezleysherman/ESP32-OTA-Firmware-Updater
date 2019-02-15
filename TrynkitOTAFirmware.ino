#include "TrynkitOTAFirmware.h"

int flag = 0;

void setup() {
  Serial.begin(115200);
  Serial.println("[DEBUG] Start");
  // Init BLE
  initBLE();

/*
  //find "app1" partition
  const char* partName = "app1";
  PART = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, partName);*/

  Update.begin();
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
/*
void writeFirmware(uint32_t addr, uint32_t partSize) {
  Serial.println("[DEBUG] Write Firmware");
  esp_partition_erase_range(PART, addr, partSize); //erase "app1" partition
  esp_partition_write(PART, (addr - APP1_ADDR), (void*)&image, sizeof(image)); //write new firmware
}

void reset() {
  ESP.restart();
}
*/
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
      if (input.equals("0x0ZD")) {
        Serial.println("[DEBUG] Update complete");
        Update.end();
      }
      else if (image.length() <= 50000){
        image += input;
        transmitOut("0x0ZS");
      }
      else {
        //image = input;
        //Serial.println("[DEBUG] Received 0x0ZC");
        //image = image.substring(0, image.length()-5);
        //writeFirmware((APP1_ADDR + (UPDATE_SIZE * updateCount)), sizeof(input));
        
        Update.write((uint8_t*)(&image), image.length());
        //check if chunk is corrupt
        transmitOut("0x0ZS");
        Serial.println("[DEBUG] Transmit 0x0ZS");
        image = "";
        return;
      }
    }

    // Check for commands
    if(input.equals("0x0ZU")) { // Flash via BLE
      Serial.println("[DEBUG] Received 0x0ZU");
      receiveImage = true;
      transmitOut("0x0ZR");
      Serial.println("[DEBUG] Transmit 0x0ZR");
    }
  }
}

void ReceiveCallBack::transmitOut(char* output) {
  //memcpy(out_buff, output, sizeof(output));
  for(int i = 0; i < sizeof(out_buff); i++) {
    out_buff[i] = uint8_t(output[i]);
  }
  transmit = true;
}
