// BLE UUID's for Trynkit Device
// See the following for generating UUIDs:
// https://www.uuidgenerator.net/
#define SERVICE_UUID           "0000ffe0-0000-1000-8000-00805f9b34fb" // UART service UUID
#define CHARACTERISTIC_UUID_RX "80c78362-e3f9-11e8-9f32-f2801f1b9fd1"
#define CHARACTERISTIC_UUID_TX "80c784ac-e3f9-11e8-9f32-f2801f1b9fd1"

//Partition characteristics
#define APP0_ADDR 0x10000
#define APP0_SIZE 0x1C0000
#define APP1_ADDR 0x1D0000
#define APP1_SIZE 0x1C0000

// Global Variables
BLEServer *pServer = NULL;
BLECharacteristic * pTxCharacteristic;

bool deviceConnected = false;
uint8_t out_buff[7];
bool receiveImage = false;
bool transmit = false;
String image = ""; // image to flash
bool fetchingOTA = false;
bool oldDeviceConnected = false;

// Method defines
void initBLE();
void deinitBLE();
void reset();
void flashFirmware();
