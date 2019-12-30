# nxTransmission

A homebrew port of Transmission 2.94 for Nintendo Switch. It has only a basic console UI, torrents can be added/managed by a web browser remotely, or with something like [Transdroid](https://github.com/erickok/transdroid).

![screenshot](/switch/screenshot.jpg?raw=true)

## Notes

* Torrents are downloaded to the /Downloads folder on the sdcard.
* You can customize the default settings by editing /switch/nxTransmission/settings.json, but this can lead to stability issues.
* As for now this was tested only with Fat32 filesystem. Using exFat could easliy lead to DATA CORRUPTION, especially if nxTransmission crashes.
* Fat32 has a file size limit of 4Gb.

## Known issues

* Enabling DHT could lead to crashes on exit.

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
