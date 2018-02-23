# Arduino LoRa

[![Build Status](https://travis-ci.org/sandeepmistry/arduino-LoRa.svg?branch=master)](https://travis-ci.org/sandeepmistry/arduino-LoRa)

An [Arduino](http://arduino.cc/) library for sending and receiving data using [LoRa](https://www.lora-alliance.org/) radios.

## Compatible Hardware

 * Heltec WIFI_LoRa_32_433-470
 * Heltec WIFI_LoRa_32_868-915

### Semtech SX1276/77/78/79 wiring

| Semtech SX1276/77/78/79 | Arduino |
| :---------------------: | :------:|
| VCC | 3.3V |
| GND | GND |
| SCK | GPIO15 |
| MISO | GPIO19 |
| MOSI | GPIO27 |
| NSS | GPIO18 |
| NRESET | GPIO14 |
| DIO0 | GPIO26 |
| DIO1 | GPIO33 |
| DIO2 | GPIO32 |


`NSS`, `NRESET`, and `DIO0` pins can be changed by using `LoRa.setPins(ss, reset, dio0)`. `DIO0` pin is optional, it is only needed for receive callback mode.

## Installation

### Using the Arduino IDE Library Manager

1. Choose `Sketch` -> `Include Library` -> `Manage Libraries...`
2. Type `LoRa` into the search box.
3. Click the row to select the library.
4. Click the `Install` button to install the library.

### Using Git

```sh
cd ~/Documents/Arduino/libraries/
git clone https://github.com/sandeepmistry/arduino-LoRa LoRa
```

## API

See [API.md](API.md).

## Examples

See [examples](examples) folder.

## License

This libary is [licensed](LICENSE) under the [MIT Licence](http://en.wikipedia.org/wiki/MIT_License).
