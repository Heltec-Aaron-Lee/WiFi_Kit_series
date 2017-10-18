Installation instructions for openSUSE
======================================

- Install the latest Arduino IDE from [arduino.cc](https://www.arduino.cc/en/Main/Software).
- Open Terminal and execute the following command (copy->paste and hit enter):

  ```bash
  sudo usermod -a -G dialout $USER && \
  if [ `python --version 2>&1 | grep '2.7' | wc -l` = "1" ]; then \
  sudo zypper install git python-pip python-pyserial; \
  else \
  sudo zypper install git python3-pip python3-pyserial; \
  fi && \
  mkdir -p ~/Arduino/hardware && \
  cd ~/Arduino/hardware && \
  git clone https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series.git heltec && \
  cd esp32 && \
  git submodule update --init --recursive && \
  cd tools && \
  python get.py && \
  cd ~/Arduino/hardware/heltec/esp8266 && \
  git submodule update --init --recursive && \
  cd tools && \
  python get.py
  ```
- Restart Arduino IDE
