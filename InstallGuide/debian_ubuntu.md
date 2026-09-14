Installation instructions for Debian / Ubuntu OS
=================================================

- Install latest Arduino IDE from [arduino.cc](https://www.arduino.cc/en/Main/Software)
- Open Terminal and execute the following command (copy->paste and hit enter):

  ```bash
  sudo usermod -a -G dialout $USER && \
  sudo apt-get install git && \
  wget https://bootstrap.pypa.io/get-pip.py && \
  sudo python get-pip.py && \
  sudo pip install pyserial && \
  mkdir -p ~/Arduino/hardware/heltec && \
  cd ~/Arduino/hardware/heltec && \
  git clone https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series.git esp32 && \
  cd heltec/esp32 && \
  git submodule update --init --recursive && \
  cd tools && \
  python get.py && \
  ```
- Restart Arduino IDE



- If you have Arduino.app installed to /Applications/, modify the installation as follows, beginning at `mkdir -p ~/Arduino...`:

  ```bash
  cd /Applications/Arduino_*/Contents/java/
  mkdir -p hardware/heltec && \
  cd hardware/heltec && \
  git clone https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series.git esp32 && \
  cd esp32 && \
  git submodule update --init --recursive && \
  cd tools && \
  python get.py && \
  ```