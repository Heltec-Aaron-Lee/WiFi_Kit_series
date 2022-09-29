/* The true ESP32 chip ID is essentially its MAC address.

created 2020-06-07 by cweinhofer
with help from Cicicok */
  
uint64_t chipId = 0;

void setup() {
  Serial.begin(115200);
}

void loop() {
  chipId=ESP.getEfuseMac();
  Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
  Serial.printf("This chip has %d cores\n", ESP.getChipCores());
  Serial.print("Chip ID: "); Serial.println(chipId);
  
  delay(3000);
}
