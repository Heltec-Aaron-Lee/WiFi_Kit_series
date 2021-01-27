/* The true ESP32 chip ID is essentially its MAC address.
This sketch provides an alternate chip ID that matches 
the output of the ESP.getChipId() function on ESP8266 
(i.e. a 32-bit integer matching the last 3 bytes of 
the MAC address. This is less unique than the 
MAC address chip ID, but is helpful when you need 
an identifier that can be no more than a 32-bit integer 
(like for switch...case).

created 2020-06-07 by cweinhofer
with help from Cicicok */
	
uint64_t chipId = 0;

void setup() {
	Serial.begin(115200);
}

void loop() {
//	  chipId |= ((ESP.getEfuseMac() >> (40 - i)) & 0xff) << i;
  chipId=ESP.getEfuseMac();//The chip ID is essentially its MAC address(length: 6 bytes).
  Serial.printf("ESP32ChipID=%04X",(uint16_t)(chipId>>32));//print High 2 bytes
  Serial.printf("%08X\n",(uint32_t)chipId);//print Low 4bytes.

//	Serial.printf("ESP32 Chip model = %s Rev %d\n", ESP.getChipModel(), ESP.getChipRevision());
//	Serial.printf("This chip has %d cores\n", ESP.getChipCores());
//  Serial.print("Chip ID: "); Serial.println(chipId);
  
	delay(3000);

}