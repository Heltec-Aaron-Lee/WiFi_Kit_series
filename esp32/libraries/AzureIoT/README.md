
# ESP32_AzureIoT - Azure IoT Hub library for esp32 devices in Arduino

This library is a port of the 
[Microsoft Azure IoT device SDK for C](https://github.com/Azure/azure-iot-sdks/blob/master/c/readme.md)
 to Arduino for esp32 devices. It allows you to use several Arduino compatible ESP32 boards with Azure IoT Hub.

## Currently supported hardware
- ESP832 based boards with [esp32/arduino](https://github.com/espressif/arduino-esp32)
  - [M5Stack](http://www.M5Stack.com)

It should also work for other esp32 boards.

## Prerequisites

You should have the following ready before beginning with any board:
-   [Setup your IoT hub](https://github.com/Azure/azure-iot-device-ecosystem/blob/master/setup_iothub.md)
-   [Provision your device and get its credentials](https://github.com/Azure/azure-iot-device-ecosystem/blob/master/setup_iothub.md#create-new-device-in-the-iot-hub-device-identity-registry)
-   [Arduino IDE 1.8.5](https://www.arduino.cc/en/Main/Software)


## SimpleMQTT and GetStarted Instructions

1. Install esp32 board support into your Arduino IDE.
    * Start Arduino and open Preferences window.
    * Enter esp32 package URL into Additional Board Manager URLs field. You can add multiple URLs, separating them with commas.
    * Open Boards Manager from Tools > Board menu and install esp32 platform.
    * Select your esp32 board from Tools > Board menu after installation

1. Open the `SimpleMQTT` or `GetStarted`example from the Arduino IDE  File->Examples->ESP32 Azure IoT Arduino.
1. Update Wifi SSID/Password and IoT Hub Connection string in ino file
    * Ensure you are using a wifi network that does not require additional manual steps after connection, such as opening a web browser.
1. Access the [Get Started](https://docs.microsoft.com/en-us/azure/iot-hub/iot-hub-get-started-physical/) tutorial to learn more about how to get started with physical devices.

## Contributing
There are a couple of ways you can contribute to this repo:

- **Ideas, feature requests and bugs**: We are open to all ideas and we want to get rid of bugs! Use the Issues section to either report a new issue, provide your ideas or contribute to existing threads.
- **Documentation**: Found a typo or strangely worded sentences? Submit a PR!
- **Code**: Contribute bug fixes, features or design changes.

Contributions for code that is not esp32 Arduino-specific can be made to the 
[Azuure IoT C SDK](https://github.com/azure/azure-iot-sdk-c)

## Code of Conduct

This project has adopted the 
[Microsoft Open Source Code of Conduct](https://opensource.microsoft.com/codeofconduct/). 
For more information see the 
[Code of Conduct FAQ](https://opensource.microsoft.com/codeofconduct/faq/) or contact 
[opencode@microsoft.com](mailto:opencode@microsoft.com) with any additional questions or comments.

## License

See [LICENSE](LICENSE) file.


