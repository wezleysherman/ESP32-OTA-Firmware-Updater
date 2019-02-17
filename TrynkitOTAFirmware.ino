#include "TrynkitOTAFirmware.h"

void setup() {
  if (EEPROM.readByte(511)) { // check update flag
    const char* partName = "firm";
    PART = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, partName);
    esp_ota_set_boot_partition(PART);
    ESP.restart(); // restart ESP
  }
  
  Serial.begin(115200);
  Serial.println("[DEBUG] Start");
  // Init BLE
  initBLE();
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
      if (input.equals("0x0ZD")) { // update complete
        Serial.println("[DEBUG] Update complete");
        if (esp_partition_verify(PART) != NULL) { // verify firmware partition
          EEPROM.writeByte(511, 0); // set update flag
          EEPROM.commit();
        }
        Update.end(); // end update
        ESP.restart(); // restart ESP
      }
      else if (image.length() <= UPDATE_SIZE){
        image += input;
        transmitOut("0x0ZS");
        return;
      }
      else {
        Serial.println(ESP.getFreeHeap());
        char buf[image.length()];
        image.toCharArray(buf, image.length()); // string to char array
        image = ""; // dump string buf
        Serial.println(ESP.getFreeHeap());
        Update.write(reinterpret_cast<uint8_t*>(buf), image.length());
        if (!Update.hasError()) {
          Serial.println("Wrote chunk!");
          transmitOut("0x0ZS"); //success
        }
        else {
          Serial.println("Corrupt chunk!");
          transmitOut("0x0ZE"); //corrupt chunk
        }
        Serial.println("[DEBUG] Transmit 0x0ZS");
        return;
      }
    }

    // Check for start command
    if(input.equals("0x0ZU")) { 
      Serial.println("[DEBUG] Received 0x0ZU");
      receiveImage = true;
      Update.begin();
      transmitOut("0x0ZR"); // transmit ready
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
