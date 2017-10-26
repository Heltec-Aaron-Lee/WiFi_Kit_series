## Steps to install Arduino ESP32 support on Windows
### Tested on 32 and 64 bit Windows 10 machines

1. Download and install the latest Arduino IDE ```Windows Installer``` from [arduino.cc](https://www.arduino.cc/en/Main/Software)
2. Download and install Git from [git-scm.com](https://git-scm.com/download/win)
-  If you don't install Git,you choose ```dowload zip``` from the home page and Unzip the file to ```/Documents/Arduino/hardware/heltec```  ,Skip steps three to step four
3. Start ```Git GUI``` and run through the following steps:
    - Select ```Clone Existing Repository```

        ![Step 1](win-screenshots/win-gui-1.png)

    - Select source and destination
        - Source Location: ```https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series.git```
        - Target Directory: ```/Documents/Arduino/hardware/heltec```
        - Change this to your Sketchbook Location if you have a different directory listed underneath the "Sketchbook location" in Arduino preferences.
        - Click ```Clone``` to start cloning the repository

            ![Step 2](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/win_gui_7%20(1).png)
            ![Step 3](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/win-gui-3.png)

4. Open ```/Documents/Arduino/hardware/heltec/esp32/tools``` and double-click ```get.exe```

     ![Step ](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/win-gui-6.png)

5. When ```get.exe``` finishes, you should see the following files in the directory

     ![Step 5](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/esp32-1.png)

6. Plug your ESP32 board and wait for the drivers to install (or install manually any that might be required)
7. Start Arduino IDE
8. Select your board in ```Tools > Board``` menu ```WiFi_Kit_32```or ```WiFi_LoRa_32```
9. Select the COM port that the board is attached to
10. Compile and upload (You might need to hold the ```PRG``` button while uploading)

    ![Arduino IDE Example](win-screenshots/arduino-ide.png)


## Steps to install Arduino ESP8266 support on Windows
### Tested on 32 and 64 bit Windows 10 machine
1. Install [Python](https://www.python.org/downloads/ )

2. Open ```/Documents/Arduino/hardware/heltec/esp8266/tools``` and double-click ```get.py```   
-  It can't work without ```Python```

   ![Step 5](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/esp8266-2.png) 
   
3. Waiting for download...

   ![Step 5](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/win_gui_9.png)
   
4.  You'll find these files in ```dist``` after downloading

   ![Step 5](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/esp8266-1.png)
   
5. Plug your ESP32 board and wait for the drivers to install (or install manually any that might be required)

6. Start Arduino IDE

7. Select your board in ```Tools > Board``` menu ```WiFi_Kit_8```

8. Select the COM port that the board is attached to

9. Compile and upload (You might need to press the ```PRG``` button while uploading)

    ![Arduino IDE Example](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/esp8266-3.png)
    
    
### How to update to the latest code

1. Start ```Git GUI``` and you should see the repository under ```Open Recent Repository```. Click on it!

    ![Update Step 1](win-screenshots/win-gui-update-1.png)

2. From menu ```Remote``` select ```Fetch from``` > ```origin```

    ![Update Step 2](win-screenshots/win-gui-update-2.png)

3. Wait for git to pull any changes and close ```Git GUI```
4. Open ```/Documents/Arduino/hardware/heltec/esp32/tools``` and double-click ```get.exe```

    ![Step 4](https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/InstallGuide/win-screenshots/win-gui-6.png)
    
















