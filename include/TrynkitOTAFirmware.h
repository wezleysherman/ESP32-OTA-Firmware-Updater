#ifndef TRYNKITOTAFIRMWARE_H
#define TRYNKITOTAFIRMWARE_H

#include <BLEDevice.h>
#include <BLE2902.h>
#include <BLEServer.h>
#include <Update.h>
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

volatile int dir = 0;
volatile int indexBlink = 0;
bool deviceConnected = false;
uint8_t out_buff[7];
bool receiveImage = false;
bool transmit = false;
String image = ""; // image to flash
bool oldDeviceConnected = false;
const uint32_t UPDATE_SIZE = 960;
const esp_partition_t* PART;
std::vector<uint8_t> fImage;

// Method defines
void initBLE();
void deinitBLE();
void reset();

#endif
