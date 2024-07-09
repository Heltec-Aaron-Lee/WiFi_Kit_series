# Heltec ESP32 Series Arduino Develop Environment

![GitHub License](https://img.shields.io/github/license/Heltec-Aaron-Lee/WiFi_Kit_series) ![GitHub Release](https://img.shields.io/github/v/release/Heltec-Aaron-Lee/WiFi_Kit_series) ![GitHub Release Date - Published_At](https://img.shields.io/github/release-date/Heltec-Aaron-Lee/WiFi_Kit_series) ![GitHub Downloads (all assets, all releases)](https://img.shields.io/github/downloads/Heltec-Aaron-Lee/WiFi_Kit_series/total)

English | [简体中文](#简体中文)

This environment is fully cloned from espressif<sup>®</sup> [ESP32](https://github.com/espressif/arduino-esp32) projects, on that basis, we fixed "variants" folder and "boards.txt", for convenience of "Arduino + ESP" beginners and Heltec ESP32 series Dev boards users.

This development environment only includes the basic framework of `ESP32`, `ESP32-S2`, `ESP32-S3`, `ESP32-C3` MCU chips. NOT include the drivers or example code for on-board devices such as LoRa, OLED, Sensor, etc.

Here are the libraries for on-board or external devices:

[Heltec ESP32 Library](https://github.com/HelTecAutomation/Heltec_ESP32) Includes the following features:

- Device drivers on the Heltec ESP32 series development board, such as OLED displays, GPS and LoRa chips;
- LoRaWAN library compatible with ESP32 Arduino environment;
- Some external sensor examples;
- Some examples of practical functions.

[E-Ink Library](https://github.com/HelTecAutomation/Heltec-E-Ink) includes the drivers and examples for multi size E-Ink displays.

## Contents

  - [Instructions](#Instructions)
  - [Installation Instructions](#Installation-Instructions)
  - [Decoding exceptions](#Decoding-exceptions)
  - [Issue/Bug report template](#Issue/Bug-report-template)
  - [Contact us](#Contact-us)

## Instructions

The following table lists products based on ESP32 :


|   MCU   |                       Relative boards                        |
| :-----: | :----------------------------------------------------------- |
|  ESP32-S3  | [WIFI Kit 32 (V3)](https://heltec.org/project/wifi-kit-32-v3/)<br>[WIFI LoRa 32 (V3)](https://heltec.org/project/wifi-lora-32-v3/)<br/>[Wireless Stick (V3)](https://heltec.org/project/wireless-stick-v3/)<br/>[Wireless Stick Lite (V3)](https://heltec.org/project/wireless-stick-lite-v2/)<br/>[Wireless Paper](https://heltec.org/project/wireless-paper/)<br/>[Wireless Tracker](https://heltec.org/project/wireless-tracker/)<br/>[Wireless Shell (V3)](https://heltec.org/project/wireless-shell-v3/)<br/>[Multi-Size E-Ink driver (HT-DE01)](https://heltec.org/project/e-ink-driveboard/)<br/>[Vision Master E213](https://heltec.org/project/ht-vme213/)<br/>[Vision Master E290](https://heltec.org/project/ht-vme290/)<br/>[Vision Master T190](https://heltec.org/project/ht-vmt190/) |
|ESP32-C3|[ESP32 C3 Dev-Board](https://heltec.org/project/esp32-c3/)<br/>[CT62 LoRa Module](https://heltec.org/project/ht-Ct62/)|
|ESP32-PICO |[Wireless Shell (V3)](https://heltec.org/project/wireless-shell/ )<br/>[Wireless Stick Lite](https://heltec.org/project/wireless-stick-lite/) -- *Not recommended for new designs*|
|ESP32 D0|[WIFI LoRa 32 (V2)](https://heltec.org/project/wifi-lora-32) -- *Not recommended for new designs*<br/>[Wireless Stick](https://heltec.org/project/wireless-stick/) -- *Not recommended for new designs*|

## Installation Instructions

- **Using Arduino IDE Boards Manager (preferred)**
  

  - [Install ESP32 from Boards Manager](https://docs.heltec.org/en/node/esp32/quick_start.html)
  
- **Using Git with the development repository**
  
  + [Instructions for Windows](InstallGuide/windows.md)
  + [Instructions for Mac](InstallGuide/mac.md)
  + [Instructions for Debian/Ubuntu Linux](InstallGuide/debian_ubuntu.md)
  + [Instructions for Fedora](InstallGuide/fedora.md)
  + [Instructions for openSUSE](InstallGuide/opensuse.md)
  
  
  - Read reference install guide document：[Installation Instructions](https://heltec.org/wifi_kit_install/)

## Decoding exceptions

You can use [EspExceptionDecoder](https://github.com/me-no-dev/EspExceptionDecoder) to get meaningful call trace.

## Issue/Bug report template

Before reporting an issue, make sure you've searched for similar one that was already created. Also make sure to go through all the issues labelled as [for reference](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues).

Also you can talk in our forum: [http://community.heltec.cn/](http://community.heltec.cn/)&nbsp;

## Contact us

- **Website：[https://heltec.org](https://heltec.org/)**
- **Document Page: [https://docs.heltec.cn](https://docs.heltec.cn)**
- **Forum: [http://community.heltec.cn/](http://community.heltec.cn/)**
- **Twitter: [https://twitter.com/HeltecOrg](https://twitter.com/HeltecOrg)**
- **Face Book: [https://www.facebook.com/heltec.automation.5](https://www.facebook.com/heltec.automation.5)**

&nbsp;

## 简体中文

本项目完全是从乐鑫提供的 [ESP32](https://github.com/espressif/arduino-esp32) 项目上克隆下来的，在此基础上，我们修改了“variants”文件夹和“boards.txt”里面的内容（增加开发板的定义和信息），这样可以方便用户（尤其是初学者）使用我司生产的 ESP32 系列开发板。

这个开发环境只包含开发板上已有硬件的相关驱动，其它外接设备驱动在以下库里：

传感器驱动: https://github.com/HelTecAutomation/Heltec_ESP32

墨水屏驱动: [https://github.com/HelTecAutomation/e-ink](https://github.com/HelTecAutomation/e-ink)

## 内容

  - [说明](#说明)
  - [安装指南](#安装指南)
  - [编码规则](#编码规则)
  - [问题讨论 & BUG报告](#问题讨论-&-BUG报告)
  - [联系我们](#联系我们)

## 说明

下表列出了基于ESP32芯片的产品型号：

|   MCU   |                       Relative boards                        |
| :-----: | :----------------------------------------------------------: |
|  ESP32s3  | [WIFI Kit 32(v3)](https://heltec.org/project/wifi-kit-32-v3/), [WIFI LoRa 32(v3)](https://heltec.org/project/wifi-lora-32-v3/), [Wireless Stick(v3)](https://heltec.org/project/wireless-stick-v3/), [Wireless Stick Lite(v3)](https://heltec.org/project/wireless-stick-lite-v2/), [Wireless Paper](https://heltec.org/project/wireless-paper/), [Wireless Tracker](https://heltec.org/project/wireless-tracker/), [Wireless Shell(v3)](https://heltec.org/project/wireless-shell-v3/),[HT-DE01](https://heltec.org/project/e-ink-driveboard/) |
|ESP32c3|[Esp32 c3 Dev-Board](https://heltec.org/project/esp32-c3/), [Ct62](https://heltec.org/project/ht-Ct62/)|
|ESP32-PICO |[Wireless Shell(v3)](https://heltec.org/project/wireless-shell/ ), [Wireless Stick Lite](https://heltec.org/project/wireless-stick-lite/)|
|ESP32 D0|[WIFI LoRa 32 (V2)](https://heltec.org/project/wifi-lora-32), [Wireless Stick](https://heltec.org/project/wireless-stick/)|

## 安装指南

首先，确保你的电脑上已经安装了最新的Arduino IDE。如果没有安装，请参考这篇文档：[https://docs.heltec.cn/#/zh_CN/user_manual/how_to_install_git_and_arduino](https://docs.heltec.cn/#/zh_CN/user_manual/how_to_install_git_and_arduino)

- **通过Arduino IDE的库管理器安装 （强烈推荐）**
- 
  - [安装基于ESP32芯片的开发环境](https://docs.heltec.org/en/node/esp32/esp32_general_docs/quick_start.html#via-arduino-board-manager)
- **通过Git从源码进行安装**
  - [Windows操作系统 -- 安装方法](InstallGuide/windows.md)
  - [MacOS操作系统 -- 安装方法](InstallGuide/mac.md)
  - [Linux操作系统(opensuse) -- 安装方法](InstallGuide/opensuse.md)
  - [Linux操作系统(debian,ubuntu) -- 安装方法](InstallGuide/debian_ubuntu.md)
  - [Linux操作系统(fedora) -- 安装方法](InstallGuide/fedora.md)
- 更多安装方法，还可以参考这里：[https://heltec.org/zh/wifi_kit_install/](https://heltec.org/zh/wifi_kit_install/)

## 编码规则

可以参考这篇文章来了解Arduino的插件和编码规则：[EspExceptionDecoder](https://github.com/me-no-dev/EspExceptionDecoder).

## 问题讨论 & BUG报告

在报告BUG之前，请先做详细的测试，如果问题真的存在，您可以通过以下方式报告或者讨论：

- [GitHub问题报告页](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues)
- [Heltec论坛 (仅英语交流)](http://community.heltec.cn/)

## 联系我们

- **官网：[https://heltec.org](https://heltec.org/)**
- **Heltec文档页: [https://docs.heltec.cn](https://docs.heltec.cn)**
- **Heltec论坛 (仅英语): [http://community.heltec.cn/](http://community.heltec.cn/)**
- **QQ群: 799093974(中文)**
