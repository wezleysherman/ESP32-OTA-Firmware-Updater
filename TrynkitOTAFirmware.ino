#include "TrynkitOTAFirmware.h"


void partloop(esp_partition_type_t part_type) {
  Serial.println("jk");
  esp_partition_iterator_t iterator = NULL;
  const esp_partition_t *next_partition = NULL;
  iterator = esp_partition_find(part_type, ESP_PARTITION_SUBTYPE_ANY, NULL);
    Serial.println("jklm");

  while (iterator) {
     next_partition = esp_partition_get(iterator);
       Serial.println("jkuy");

     if (next_partition != NULL) {
        Serial.printf("partiton addr: 0x%06x; size: 0x%06x; label: %s\n", next_partition->address, next_partition->size, next_partition->label);
          Serial.println("jklmno");
  
     iterator = esp_partition_next(iterator);
    }
  }
}

void setup() {
    /*
 // if (EEPROM.readByte(511)) { // check update flag
    const char* partName = "firm";
    PART = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, partName);
    PART = esp_partition_verify(PART);
    Serial.println(PART->label);
    Serial.println(PART->type);
    Serial.println(PART->subtype);
    Serial.println(PART->size);
    if(PART == NULL) {
      Serial.println("null partition");
    }
     PART = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, partName);
    //esp_ota_set_boot_partition(PART);
    //ESP.restart(); // restart ESP
  //}*/
  
  Serial.begin(115200);
  Serial.println("[DEBUG] Startsasd");
  //partloop(ESP_PARTITION_TYPE_APP);
  //const esp_partition_t *next_partition = esp_ota_get_next_update_partition(NULL);
  //Serial.printf("partiton addr: 0x%06x; size: 0x%06x; label: %s\n", next_partition->address, next_partition->size, next_partition->label);

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
byte hexton (byte h)
{
  if (h >= '0' && h <= '9')
    return(h - '0');
  if (h >= 'A' && h <= 'F')
    return((h - 'A') + 10);
}

std::vector<uint8_t> fImage;
int fIdx = 0;
int totalBits = 0;

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
        totalBits ++;
      //  Serial.println(fImage.at(fIdx));
        fIdx++;    
       // Serial.println(fImage.size());
      }
    }

    
    // BLE Flashing mode
    if(receiveImage == true) {
      if (input.equals("0x0ZD")) { // update complete
      //  byte buf = fImage[0];
        ////image.toCharArray(buf, image.length()); // string to char array
        //for(int i = 0; i <image.length(); i++) {
         // buf[i] = (byte) image[i];
         // Serial.println(buf[i]);
        //}
       
        //Serial.println(image);
        Serial.println(ESP.getFreeHeap());
        uint8_t *dataF = fImage.data();
        Serial.println(Update.write(dataF, fImage.size()));
       // Serial.println("Total: " + totalBits);
        fImage.clear();
         if (!Update.hasError()) {
          Serial.println("Wrote chunk!");
          transmitOut("0x0ZS"); //success
        }
        else {
          Update.printError(Serial);
          Serial.println("Corrupt chunk!");
          transmitOut("0x0ZE"); //corrupt chunk
        }
        Serial.println(Update.end(true)); // end update

        if (!Update.hasError()) {
          Serial.println("Wrote chunk!");
          transmitOut("0x0ZS"); //success
        }
        else {
          Update.printError(Serial);
          Serial.println("Corrupt chunk!");
          transmitOut("0x0ZE"); //corrupt chunk
        }
        Serial.println("[DEBUG] Update complete");
     //   if (esp_partition_verify(PART) != NULL) { // verify firmware partition
          EEPROM.writeByte(511, 0); // set update flag
          EEPROM.commit();
       // }
        const char* partName = "firm";
        PART = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, partName);
        esp_ota_set_boot_partition(PART);
        ESP.restart(); // restart ESP
      }
      if (fImage.size() >= UPDATE_SIZE) {
        //unsigned char buf = &fImage[0];
        ////image.toCharArray(buf, image.length()); // string to char array
        
       
        //Serial.println(image);
        Serial.println(ESP.getFreeHeap());
        uint8_t *dataF = fImage.data();
        /*for(int i = 0; i <fImage.size(); i++) {
         // buf[i] = (byte) image[i];
          Serial.println(dataF[i]);
        }*/
        Serial.println(fImage.size());
        Serial.println(Update.write(dataF, fImage.size()));
        fImage.clear();

        if (!Update.hasError()) {
          Serial.println("Wrote chunk!");
          transmitOut("0x0ZS"); //success
        }
        else {
          Update.printError(Serial);
          Serial.println("Corrupt chunk!");
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
