#include "TrynkitOTAFirmware.h"

void initBLE();
void deinitBLE();
void onConnect(BLEServer* pServer);
void onDisconnect(BLEServer* pServer);
void onWrite(BLECharacteristic *pCharacteristic);
void tansmitOut(char* output);
void updateLED(void * pvParameters);

void setup() {
  //Serial.begin(115200);
  //Serial.println("[DEBUG] Started");
  ledcSetup(0, 5000, 8);
  ledcAttachPin(12, 0);
  TaskHandle_t LEDTask;
  xTaskCreatePinnedToCore(updateLED, "LEDTask", 10000, NULL, 1, &LEDTask, 0);
  // Init BLE
  initBLE();
}

void updateLED(void * pvParameters) {
  while(1) {
    vTaskDelay(20);
    if(!deviceConnected) {
      for(int i = 0; i < 5; i++) {
        ledcWrite(0, 20);
        delay(100);
        ledcWrite(0, 0);
        delay(100);
      }
    } else {
      ledcWrite(0, indexBlink);
       if(dir == 0) {
          indexBlink ++;
       } else {
         indexBlink --;
       }
    
       if(indexBlink >= 100) {
         dir = 1;
       } else if(indexBlink <= 0) {
         dir = 0;
       }
    } 
  }
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
    ESP.restart();
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
    if(receiveImage == false) {
      for(int i = 0; i < rxVal.length(); i++) {
        if(i % 2 == 0) {
          input += rxVal[i];
        }
      }
    } else {
      for(int i = 0; i < rxVal.length(); i++) {
        if(i % 2 == 0) {
          input += rxVal[i];
        }
        uint8_t outChar = rxVal[i];
        fImage.push_back(outChar);
      }
    }

    
    // BLE Flashing mode
    if(receiveImage == true) {
      if (input.equals("0x0ZD")) { // update complete
     
        uint8_t *dataF = fImage.data();
        Serial.println(Update.write(dataF, fImage.size()));
        fImage.clear();
        if (!Update.hasError()) {
          Serial.println("Wrote chunk!");
          transmitOut("0x0ZS"); //success
        } else {
          Update.printError(Serial);
          transmitOut("0x0ZE"); //corrupt chunk
        }
        
        Serial.println(Update.end(true)); // end update
        
        if (!Update.hasError()) {
          Serial.println("Wrote chunk!");
          transmitOut("0x0ZS"); //success
        }
        else {
          Update.printError(Serial);
          transmitOut("0x0ZE"); //corrupt chunk
        }
        
        Serial.println("[DEBUG] Update complete");
        const char* partName = "firm";
        PART = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, partName);
        esp_ota_set_boot_partition(PART);
        ESP.restart(); // restart ESP
      }
      if (fImage.size() >= UPDATE_SIZE) {
        uint8_t *dataF = fImage.data();

        Serial.println(Update.write(dataF, fImage.size()));
        fImage.clear();

        if (!Update.hasError()) {
          Serial.println("Wrote chunk!");
          transmitOut("0x0ZS"); //success
        }
        else {
          Update.printError(Serial);
          transmitOut("0x0ZE"); //corrupt chunk
        }
        Serial.println("[DEBUG] Transmit 0x0ZS");
        return;
      }
    }

    // Check for start command
    if(input.equals("0x0ZU ")) { 
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
