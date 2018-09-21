## contents
- [The Overview](#the-overview)
- [ShowCore](#showcore)
- [MoveCore](#movecore)
- [SpeedTest](#speedtest)
- [Test information](#test_information)
# The Overview
- The ESP32 chip has three cores.
- Two cores are fast cores and one core is a low-power core.
- Which is an example of ESP32 Dual Core on Arduino IDE including Data Passing and Task Synchronization.


## ShowCore
In the first step, we need to know which core the current program is running on.
we need a code to print the current core from the serial port.
```
Serial.println(xPortGetCoreID());
```
![](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/tree/master/esp32/libraries/ESP32/examples/ESP32_Dual_Core/resources/print_core.png)
[example](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/tree/master/esp32/libraries/ESP32/examples/ESP32_Dual_Core/examples/ShowCore)
## MoveCore
- Run the program with the specified core
![](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/tree/master/esp32/libraries/ESP32/examples/ESP32_Dual_Core/resources/MoveCore.png)
[example](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/tree/master/esp32/libraries/ESP32/examples/ESP32_Dual_Core/examples/Movecore)

## SpeedTest
- Test the speed of the two cores under different conditions
![](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/tree/master/esp32/libraries/ESP32/examples/ESP32_Dual_Core/resources/SpeedTest.png)
[example](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/tree/master/esp32/libraries/ESP32/examples/ESP32_Dual_Core/examples/SpeedTest)
In this example the program is running as follows
![](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/tree/master/esp32/libraries/ESP32/examples/ESP32_Dual_Core/resources/Task_Synchronization.png)
## Test information 

![](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/PinoutDiagram/WIFI%20LoRa%2032(V2)%20.pdf)
- node: [WIFI LoRa 32 V2](https://item.taobao.com/item.htm?spm=a1z10.1-c.w4004-17008247508.4.7bdf1d6f2XG3ID&id=575190433694) 

- Arduino 18.04
