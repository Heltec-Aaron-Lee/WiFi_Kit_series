/*
  ADC read voltage via GPIO13 simple test.
 
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
*/

void setup() {
  Serial.begin(115200);
  /*
  analogReadResolution(12);             // Sets the sample bits and read resolution, default is 12-bit (0 - 4095), range is 9 - 12 bits
  analogSetWidth(12);                   // Sets the sample bits and read resolution, default is 12-bit (0 - 4095), range is 9 - 12 bits
                                        //  9-bit gives an ADC range of 0-511
                                        // 10-bit gives an ADC range of 0-1023
                                        // 11-bit gives an ADC range of 0-2047
                                        // 12-bit gives an ADC range of 0-4095
  analogSetCycles(8);                   // Set number of cycles per sample, default is 8 and provides an optimal result, range is 1 - 255
  analogSetSamples(1);                  // Set number of samples in the range, default is 1, it has an effect on sensitivity has been multiplied
  analogSetClockDiv(1);                 // Set the divider for the ADC clock, default is 1, range is 1 - 255
  analogSetAttenuation(ADC_11db);       // Sets the input attenuation for ALL ADC inputs, default is ADC_11db, range is ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
  analogSetPinAttenuation(VP,ADC_11db); // Sets the input attenuation, default is ADC_11db, range is ADC_0db, ADC_2_5db, ADC_6db, ADC_11db
                                        // ADC_0db provides no attenuation so IN/OUT = 1 / 1 an input of 3 volts remains at 3 volts before ADC measurement
                                        // ADC_2_5db provides an attenuation so that IN/OUT = 1 / 1.34 an input of 3 volts is reduced to 2.238 volts before ADC measurement
                                        // ADC_6db provides an attenuation so that IN/OUT = 1 / 2 an input of 3 volts is reduced to 1.500 volts before ADC measurement
                                        // ADC_11db provides an attenuation so that IN/OUT = 1 / 3.6 an input of 3 volts is reduced to 0.833 volts before ADC measurement
  adcAttachPin(VP);                     // Attach a pin to ADC (also clears any other analog mode that could be on), returns TRUE/FALSE result 
  adcStart(VP);                         // Starts an ADC conversion on attached pin's bus
  adcBusy(VP);                          // Check if conversion on the pin's ADC bus is currently running, returns TRUE/FALSE result 
  adcEnd(VP);                           // Get the result of the conversion (will wait if it have not finished), returns 16-bit integer result
  */
  adcAttachPin(13);
  analogSetClockDiv(255); // 1338mS
}

void loop() {
   int timer  = micros();
   Serial.print(analogRead(13));
   Serial.print("  ");
   Serial.println(micros() - timer);
}

//See more APIs about ADC here: https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/blob/master/esp32/cores/esp32/esp32-hal-adc.h

