# WiFi_Kit_series Arduino Environment user guide

- WiFi Kit 系列arduino开发环境安装指南

- [English version install guide](#instructions)

## Contents
- [说明](#说明)
- [安装方法](#安装方法)
- [编码规则](#编码规则)
- [BUG报告](#BUG报告)
- [重要提示](#重要提示)
- [产品示意图](#产品示意图)
- [联系我们](#联系我们)


## 说明
- WIFI Kit 32、WIFI LoRa 32使用的是ESP32芯片，与之相关的开发环境、源码、例程，在本页面的[esp32](esp32/)文件夹内；
- WIFI Kit 8 使用的是ESP8266芯片，与之相关的开发环境、源码、例程，在本页面的[esp8266](esp8266/)文件夹内；

## 安装方法
- 首先，确保你的电脑上已经安装了最新的Arduino IDE。如果没有安装，请在https://www.arduino.cc/en/Main/Software 下载并安装；
 + [Windows操作系统 -- 安装方法](InstallGuide/windows.md)
 + [MacOS操作系统 -- 安装方法](InstallGuide/mac.md)
 + [Linux操作系统(opensuse) -- 安装方法](InstallGuide/opensuse.md)
 + [Linux操作系统(debian,ubuntu) -- 安装方法](InstallGuide/debian_ubuntu.md)
 + [Linux操作系统(fedora) -- 安装方法](InstallGuide/fedora.md)

## 编码规则
可以参考这篇文章来了解arduino的插件和编码规则：[EspExceptionDecoder](https://github.com/me-no-dev/EspExceptionDecoder).

## BUG报告
在报告BUG之前，请先做足够的测试，如果问题真的存在，请提交到：[问题&BUG](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues).

## 重要提示
- ESP系列芯片的下载速度较快，请确保使用了优质的Micro USB连接线，否则很容易出现无法下载的情况；
- 下载程序之前，按住板上的PRG键，可大大提高下载成功率。

## 联系我们
- 官网：www.heltec.cn

# Arduino Environment install guide

## Instructions
- WIFI_Kit_32,WIFI_LoRa_32 use esp 32 chip,the source codes ang dev_environment are in [esp32](esp32/)
- WIFI_Kit_8 use esp8266 chip,the source codes ang dev_environment are in [esp8266](esp8266/)
## Contents
- [Installation Instructions](#installation-instructions)
- [Decoding Exceptions](#decoding-exceptions)
- [Issue/Bug report template](#issuebug-report-template)
- [Board Picture](#产品示意图)
- [Contact us](#联系我们)


## Installation Instructions

- Using Arduino IDE
  + [Instructions for Windows](InstallGuide/windows.md)
  + [Instructions for Mac](InstallGuide/mac.md)
  + [Instructions for Debian/Ubuntu Linux](InstallGuide/debian_ubuntu.md)
  + [Instructions for Fedora](InstallGuide/fedora.md)
  + [Instructions for openSUSE](InstallGuide/opensuse.md)


## Decoding exceptions

You can use [EspExceptionDecoder](https://github.com/me-no-dev/EspExceptionDecoder) to get meaningful call trace.

## Issue/Bug report template
Before reporting an issue, make sure you've searched for similar one that was already created. Also make sure to go through all the issues labelled as [for reference](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues).


## Important tips
- The download speed is very fast ,please use high-quality Micro USB
- Hold down the PRG key to improve download success before downloading 

## Contact us
- website：www.heltec.cn

## 产品示意图
- WIFI_Kit_8
  ![image](InstallGuide/win-screenshots/WIFI_kit_8.png)
- WIFI_Kit_32
  ![image](InstallGuide/win-screenshots/WIFI_Kit_32.png)
- WIFI_LoRa_32
  ![image](InstallGuide/win-screenshots/WIFI_LoRa_32.png)

