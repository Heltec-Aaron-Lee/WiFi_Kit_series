/*
 * Heltec Automation ESP32 Serial 2 example.
 * work with ESP32's IO MUX
*/

HardwareSerial Serial2(2);

void setup() {

  Serial.begin(115200);

  // Serial2.begin(unsigned long baud, uint32_t config, int8_t rxPin, int8_t txPin, bool invert)
  // The txPin & rxPin can set to any output pin
  Serial2.begin(115200, SERIAL_8N1, 18, 17);
}

void loop() {

  if(Serial.available()) {
    int ch = Serial.read();
    Serial2.write(ch);  
  }

  if(Serial2.available()) {
    int ch = Serial2.read();
    Serial.write(ch);
  }
}
