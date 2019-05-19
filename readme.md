### To flash partition:

python /home/$USER/esp/esp-idf/components/esptool_py/esptool/esptool.py --chip esp32 --port /dev/ttyUSB0 --baud 115200 --before default_reset --after hard_reset write_flash -z --flash_mode dio --flash_freq 40m --flash_size detect \[**partition offset**\] \[**path_to_firmware.bin**\]

### Hexadecimal partition offsets:
* 0x1000   - bootloader
* 0x8000   - partition table
* 0x10000  - firmware updater
* 0x130000 - Trynkit firmware
