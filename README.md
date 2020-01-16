# nxTransmission

A homebrew port of Transmission 2.94 for Nintendo Switch with a very basic console UI. Torrents can be added/managed by a web browser remotely, or with something like [Transdroid](https://github.com/erickok/transdroid).

![screenshot](/switch/screenshot.jpg?raw=true)

## Install

Extract the release zip to the /switch folder on your sdcard. Skip the settings.json if you don't want to revert to the release defaults.

## exFAT support

This was tested only with the FAT32 file-system. Using exFAT could lead to **DATA CORRUPTION**, especially if nxTransmission crashes.

## FAT32 limit

FAT32 has a file size limit of 4GB, files bigger than this limit are split automatically to a folder matching the original file name, and the file concatenation attribute is applied to the folder when the torrent fully completes. The file-system is detected based on the firmware version, and splitting is on by default if exFAT is not supported. The split setting can be forced from settings.json by adding entry 

```
    "nx-split-files": true[or false]
```

## Notes

* Torrents are downloaded by default to the **/Downloads** folder on the sdcard.
* You can customize the settings by editing **/switch/nxTransmission/settings.json**, or in the browser with the web client. Be warned this can lead to speed and/or stability issues.

## Known issues

* enabling DHT could lead to crashes on exit.
* Sleep mode and network changes lead to broken pipe errors.

## Building

The following pacman packages are required:

* switch-curl
* switch-mbedtls
* switch-miniupnpc
* switch-zlib

Build:
```
mkdir build
cd build
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake.switch/devkita64-libnx.cmake
make
```

## License

This project is licensed under the GNU GPL versions 2 or 3 - see the [COPYING](COPYING) file for details.

## Credits

* [The Transmission Project](https://transmissionbt.com)
* [libnx](https://switchbrew.org/)
* [libnx-template](https://github.com/vbe0201/libnx-template)
