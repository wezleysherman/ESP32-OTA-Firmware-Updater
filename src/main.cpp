/**
 * OTA update firmware for Trynkit Husky.
 * 
 * @author Wezley Sherman
 * @author Tanner Lisonbee
 * @author David Anderson
 */

#include "TrynkitOTAFirmware.h"

void initBLE();
void deinitBLE();
void onWrite(BLECharacteristic *pCharacteristic);
void transmitOut(char* output);
void onConnect(BLEServer* pServer);
void onDisconnect(BLEServer* pServer);
void updateLED(void *pvParameters);
void finishUpdate();

/**
 * Initializes serial and LED output.
 */
void setup() {
  Serial.begin(115200); // baud rate
  Serial.println("[DEBUG] Started");
  ledcSetup(0, 5000, 8);
  ledcAttachPin(12, 0);
  TaskHandle_t LEDTask;
  xTaskCreatePinnedToCore(updateLED, "LEDTask", 10000, NULL, 1, &LEDTask, 0);
  initBLE();
}

/**
 * Main loop.
 */
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
    ESP.restart(); // restart ESP module
  }
  
  // connecting
  if (deviceConnected && !oldDeviceConnected) {
    oldDeviceConnected = deviceConnected;
  }
}

/**
 * Initialize BLE radio and create server.
 */
void initBLE() {
  Serial.println("[DEBUG] Init BLE");
  BLEDevice::init("Trynkit Husky");
  pServer = BLEDevice::createServer();
  // Callback function for when something is received.
  pServer->setCallbacks(new MyServerCallbacks());
  BLEService *pService = pServer->createService(SERVICE_UUID);
  pTxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_TX, BLECharacteristic::PROPERTY_NOTIFY);
  pTxCharacteristic->addDescriptor(new BLE2902());
  BLECharacteristic * pRxCharacteristic = pService->createCharacteristic(CHARACTERISTIC_UUID_RX, BLECharacteristic::PROPERTY_WRITE);
  pRxCharacteristic->setCallbacks(new ReceiveCallBack());
  pService->start();
  pServer->getAdvertising()->start();
}

/**
 * Deinitialize BLE radio and release allocated memory.
 */
void deinitBLE() {
  esp_bt_controller_disable();
  esp_bt_controller_deinit();
  esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);
}

/**
 * Handles BLE receives for images and flashing.
 *
 * @param pCharacteristic BLE packet
 */
void ReceiveCallBack::onWrite(BLECharacteristic *pCharacteristic) {
  std::string rxVal = pCharacteristic->getValue();
  String input = ""; // buffer for current packet received
  
  // Copy packet contents into String buffer 'input'
  if(rxVal.length() > 0) {
	if (receiveImage) {
      for(int i = 0; i < rxVal.length(); i++) {
	    if(i % 2 == 0) {
	      input += rxVal[i];
        }
		uint8_t outChar = rxVal[i];
	    fImage.push_back(outChar);  
      }
	} else {
	  for(int i = 0; i < rxVal.length(); i++) {
	    if(i % 2 == 0) {
	      input += rxVal[i];
        }
      }
	}

    // BLE Flashing mode
    if(receiveImage == true) {
      if (fImage.size() >= UPDATE_SIZE) {
		uint8_t *dataF = fImage.data();
		Serial.print("[DEBUG] ");
		Serial.println(Update.write(dataF, fImage.size()));
		fImage.clear();
		
		if (!Update.hasError()) {
		  Serial.println("[DEBUG] Wrote chunk!");
		  transmitOut("0x0ZS"); //success
		} else {
		  Update.printError(Serial);
		  transmitOut("0x0ZE"); //corrupt chunk
		}
		
		if (input.equals("0x0ZD")) {
	      finishUpdate();
		} else {
		  Serial.println("[DEBUG] Transmit 0x0ZS");
		  return;
		}
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

/**
 * Copies a char array to an output buffer that will be printed over serial.
 *
 * @param output String to transmit
 */
void ReceiveCallBack::transmitOut(char* output) {
  for(int i = 0; i < sizeof(out_buff); i++) {
    out_buff[i] = uint8_t(output[i]);
  }
  transmit = true;
}

void MyServerCallbacks::onConnect(BLEServer* pServer) {
  deviceConnected = true;
}

void MyServerCallbacks::onDisconnect(BLEServer* pServer) {
  deviceConnected = false;
}

/**
 * Toggles the white LED rapidly while in the updater partition. This is done
 * to make it obvious to the end user than an OTA update is being performed.
 *
 * @param pvParameters
 */
void updateLED(void *pvParameters) {
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

/**
 * Set new boot partition and restart ESP module.
 */
void finishUpdate() {
  Serial.println("[DEBUG] Update complete");
  const char* partName = "firm";
  PART = esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_ANY, partName);
  esp_ota_set_boot_partition(PART);
  ESP.restart(); // restart ESP
}
