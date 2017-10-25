WiFi_Kit_8 User guide
===========================================

WiFi_Kit_8 使用esp8266芯片，使用Ardunio的库和代码能够直接在esp8266上运行，无需外接控制器。

ESP8266 功能包括使用用TCP和UDP无线沟通，建立HTTP，MDNS，SSDP，和DNS服务器，做OTA更新，闪存使用文件系统，用做SD卡，伺服，SPI和I2C外设..


## Contents
- [安装方法](#安装方法)
- [参考文档](#资料)
- [问题报告](#问题报告)
- [联系我们](#联系我们)  
- [产品示意图](#产品示意图)   

## 安装方法
### 使用 Git

- 确保下载最新版本的Ardunio IDE ，可在https://www.arduino.cc/en/Main/Software 下载并安装；
- 确保你的电脑上安装了Python 工具，如果没有，可在https://www.python.org/downloads/ 下载并安装；
- 使用 Git Gui 点击``` Clone Existing Repository```将仓库```git @github.com:Heltec-Aaron-Lee/WiFi_Kit_series.git ```拷贝到 ```Ardunio/hardware/heltec``` 文件夹下面；
- 如图所示

  ![Step 1](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/win-gui-1.png)
  ![Step 2](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/win-gui-2%20.png)
  ![Step 3](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/win-gui-3.png)
- 进入```Ardunio/hardware/heltec/tool```文件夹，双击```get.py```（必须使用Python 工具来执行）
- 如图显示正在下载工具，
  等待下载完成后，窗口自动关闭；
- 重启Ardunio IDE。

### 资料

以下文件持续更新中

- [参考](doc/reference.md)
- [库](doc/libraries.md)
- [文件系统](doc/filesystem.md)
- [OTA 更新](doc/ota_updates/readme.md)
- [Supported boards](doc/boards.md)


### 问题报告

在报告BUG之前，请先做足够的测试，如果问题真的存在，请提交到：[问题&BUG](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues?utf8=%E2%9C%93&q=is%3Aissue%20label%3A%22for%20reference%22%20).

你也可以进入[ESP8266 Community Forum](http://www.esp8266.com/arduino) 查找解决方法。

### 联系我们
- 官方网站：```www.heltec.cn```

### 产品示意图
- WIFI_Kit_8
  ![image](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/WIFI_kit_8.png)


