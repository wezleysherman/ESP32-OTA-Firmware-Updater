#ifndef TRYNKITOTAFIRMWARE_H
#define TRYNKITOTAFIRMWARE_H

#include <BLEDevice.h>
#include <BLE2902.h>
#include <BLEServer.h>
#include <EEPROM.h>
#include <Update.h>
#include "ArduinoJson.h"
#include "ReceiveCallBack.h"
#include "esp_ota_ops.h"
#include <esp_partition.h>

// BLE UUID's for Trynkit Device
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID           "0000ffe0-0000-1000-8000-00805f9b34fb" // UART service UUID
#define CHARACTERISTIC_UUID_RX "80c78362-e3f9-11e8-9f32-f2801f1b9fd1"
#define CHARACTERISTIC_UUID_TX "80c784ac-e3f9-11e8-9f32-f2801f1b9fd1"

// Global Variables
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;

bool deviceConnected = false;
uint8_t out_buff[7];
bool receiveImage = false;
bool transmit = false;
String image = ""; // image to flash
bool oldDeviceConnected = false;
uint8_t updateCount = 0; //increment for every write and flush of the update
const uint32_t UPDATE_SIZE = 960;
const esp_partition_t* PART;
int flag = 0;

// Method defines
void initBLE();
void deinitBLE();
void reset();
void writeFirmware(esp_partition_t* part, uint32_t addr, uint32_t partSize);

#endif
