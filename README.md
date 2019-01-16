# Heltec ESP32 & ESP8266 Series Arduino Develop Environment

English | [简体中文](#ESP32+ESP8266开发环境)

This environment is fully clone from espressif [ESP32](https://github.com/espressif/arduino-esp32) and [ESP8266](https://github.com/esp8266/Arduino) projects, on that basis, we fixed "variants" folder and "boards.txt", for convenience of "Arduino + ESP" beginners and Heltec ESP seies Dev boards users.

*This environment not include on board OLED LoRa etc drivers and examples anymore, they had been moved to another libraries:*
For ESP32 series: https://github.com/HelTecAutomation/Heltec_ESP32
For ESP8266 series:

## Instructions
- WIFI_Kit_32,WIFI_LoRa_32, WIFI_LoRa_32_V2, Wireless_Stick use ESP32 chip,the source codes and dev_environment:[esp32](esp32/)
- WIFI_Kit_8 use esp8266 chip,the source codes and dev_environment:[esp8266](esp8266/)

## Installation Instructions

- Using Arduino IDE
  + [Instructions for Windows](InstallGuide/windows.md)
  + [Instructions for Mac](InstallGuide/mac.md)
  + [Instructions for Debian/Ubuntu Linux](InstallGuide/debian_ubuntu.md)
  + [Instructions for Fedora](InstallGuide/fedora.md)
  + [Instructions for openSUSE](InstallGuide/opensuse.md)
  
  
  - Read reference linstall guide document：[English version install manual](http://www.heltec.cn/the-installation-method-of-wifi-kit-series-products-in-arduino-development-environment/?lang=en)

## Decoding exceptions

You can use [EspExceptionDecoder](https://github.com/me-no-dev/EspExceptionDecoder) to get meaningful call trace.

## Issue/Bug report template
Before reporting an issue, make sure you've searched for similar one that was already created. Also make sure to go through all the issues labelled as [for reference](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues).

## Note:
* ESP series chips are faster to download, please make sure to use the high-quality Micro USB cable, otherwise it will be easy to download.
[Summary of common problems](http://www.heltec.cn/summary-of-common-problems-in-wifi-kit-series-continuous-update/?lang=en)

## Contact us
- website：www.heltec.cn




## ESP32+ESP8266开发环境

本项目完全是从乐鑫提供的[ESP32](https://github.com/espressif/arduino-esp32)和[ESP8266](https://github.com/esp8266/Arduino)项目上克隆下来的，在此基础上，我们修改了“variants”文件夹和“boards.txt”里面的内容（增加开发板的定义和信息），这样可以方便用户（尤其是初学者）使用我司生产的ESP32和ESP8266系列开发板。

*本项目只是开发环境，我们将板子上OLED、LoRa等资源的例程、驱动程序移到了一个独立的库里面（这样的好处是：如果您已经安装了乐鑫提供的标准的[ESP32](https://github.com/espressif/arduino-esp32)或[ESP8266](https://github.com/esp8266/Arduino)开发环境，只需要安装下面的库即可，就不再需要安装我们提供的这个开发环境了）*
ESP32系列: https://github.com/HelTecAutomation/Heltec_ESP32
ESP8266系列:

## 说明
- WIFI_Kit_32,WIFI_LoRa_32, WIFI_LoRa_32_V2, Wireless_Stick使用的是ESP32芯片，与之相关的开发环境、源码、例程，在本页面的[esp32](esp32/)文件夹内；
- WIFI Kit 8 使用的是ESP8266芯片，与之相关的开发环境、源码、例程，在本页面的[esp8266](esp8266/)文件夹内；

## 安装方法
- 首先，确保你的电脑上已经安装了最新的Arduino IDE。如果没有安装，请在https://www.arduino.cc/en/Main/Software 下载并安装；
 + [Windows操作系统 -- 安装方法](InstallGuide/windows.md)
 + [MacOS操作系统 -- 安装方法](InstallGuide/mac.md)
 + [Linux操作系统(opensuse) -- 安装方法](InstallGuide/opensuse.md)
 + [Linux操作系统(debian,ubuntu) -- 安装方法](InstallGuide/debian_ubuntu.md)
 + [Linux操作系统(fedora) -- 安装方法](InstallGuide/fedora.md)
 
 - 中文的安装教程，还可以参考这里：http://www.heltec.cn/wifi_kit_install/

## 编码规则
可以参考这篇文章来了解arduino的插件和编码规则：[EspExceptionDecoder](https://github.com/me-no-dev/EspExceptionDecoder).

## BUG报告
在报告BUG之前，请先做足够的测试，如果问题真的存在，请提交到：[问题&BUG](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues).

## 重要提示
- ESP系列芯片的下载速度较快，请确保使用了优质的Micro USB连接线，否则很容易出现无法下载的情况；
- 下载程序之前，按住板上的PRG键，可大大提高下载成功率。

## 联系我们
- 官网：www.heltec.cn


## 产品示意图
- WIFI_Kit_8

![image](InstallGuide/win-screenshots/WIFI_kit_8.png)

- WIFI_Kit_32

![image](InstallGuide/win-screenshots/WIFI_Kit_32.png)

- WIFI_LoRa_32

![image](InstallGuide/win-screenshots/WIFI_LoRa_32.png)

