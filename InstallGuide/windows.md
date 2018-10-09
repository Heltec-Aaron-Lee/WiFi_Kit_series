# Steps to install Arduino ESP32 support on Windows
### Tested on 32 and 64 bit Windows 10 machines

1. Download and install the latest Arduino IDE ```Windows Installer``` from [arduino.cc](https://www.arduino.cc/en/Main/Software)
2. Download and install Git from [git-scm.com](https://git-scm.com/download/win)
-  If you don't install Git,you choose ```dowload zip``` from the home page and Unzip the file to ```/Documents/Arduino/hardware/heltec```  ,Skip steps three to step four
3. Execute the command under the **\Documents\Arduino\hardware** path
   Start ```Git Bash``` and run through the following steps:

    - Input ```git clone https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series.git heltec ```

        ![Step 3](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/location.png)

4. Open ```/Documents/Arduino/hardware/heltec/esp32/tools``` and double-click ```get.exe```

     ![Step 4](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/win-gui-6.png)

5. When ```get.exe``` finishes, you should see the following files in the directory

     ![Step 5](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/esp32-1.png)

6. Plug your ESP32 board and wait for the drivers to install (or install manually any that might be required)
7. Start Arduino IDE
8. Select your board in ```Tools > Board``` menu ```WiFi_Kit_32```or ```WiFi_LoRa_32```
9. Select the COM port that the board is attached to
10. Compile and upload (You might need to hold the ```PRG``` button while uploading)

    ![Arduino IDE Example](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/arduino-ide.png)
### How to update to the latest code

1. Open ```/Documents/Arduino/hardware/heltec``` created start ```Git Bash``` and input ```git pull```!

    ![Update Step 1](https://github.com/lxyzn/picture/blob/master/git.pull.png)

2. Wait for git to pull any changes and close ```Git Bash```
3. Open ```/Documents/Arduino/hardware/heltec/esp32/tools``` and double-click ```get.exe```

    ![Step 4](https://github.com/lxyzn/picture/blob/master/git.pull.png)


# Steps to install Arduino ESP8266 support on Windows
[Please click here](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/esp8266/README.md)
