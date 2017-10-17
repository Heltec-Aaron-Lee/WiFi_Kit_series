# WiFi_Kit_series Arduino Environment user guide
- WiFi Kit 系列arduino开发环境安装指南

## Contents
- [说明](#说明)
- [安装方法](#安装方法)
- [编码规则](#编码规则)
- [BUG报告](#BUG报告)
- [重要提示](#重要提示)

## 说明
- WIFI Kit 32、WIFI LoRa 32使用的是ESP32芯片，与之相关的开发环境、源码、例程，在本页面的[esp32](esp32/)文件夹内；
- WIFI Kit 8 使用的是ESP8266芯片，与之相关的开发环境、源码、例程，在本页面的[esp8266](esp8266/)文件夹内；

## 安装方法
- 首先，确保你的电脑上已经安装了最新的Arduino IDE。如果没有安装，请在https://www.arduino.cc/en/Main/Software 下载并安装；
  + [WIFI Kit 32、WIFI LoRa 32的安装方法](esp32/README.md)
  + [WIFI Kit 8的安装方法]()

## 编码规则
可以参考这篇文章来了解arduino的插件和编码规则：[EspExceptionDecoder](https://github.com/me-no-dev/EspExceptionDecoder).

## BUG报告
在报告BUG之前，请先做足够的测试，如果问题真的存在，请提交到：[问题&BUG](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues?utf8=%E2%9C%93&q=is%3Aissue%20label%3A%22for%20reference%22%20).

## 重要提示
-ESP系列芯片的下载速度较快，请确保使用了优质的Micro USB连接线，否则很容易出现无法下载的情况；
-下载程序之前，按住板上的PRG键，可大大提高下载成功率。
