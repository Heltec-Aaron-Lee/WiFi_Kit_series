/* Heltec Automation txpower test example
 *
 * Function:
 * 1. Lora transmission power test.
 * 
 * 
 * this project also realess in GitHub:
 * https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series
 * */

#include "LoRaWan_APP.h"
#include "Arduino.h"

#define RF_FREQUENCY                                868000000 // Hz
#define TX_OUTPUT_POWER                             10        // 20 dBm
#define TX_TIMEOUT                                  10        // seconds (MAX value)
static RadioEvents_t RadioEvents;

void OnRadioTxTimeout( void )
{
    // Restarts continuous wave transmission when timeout expires
    Radio.SetTxContinuousWave( RF_FREQUENCY, TX_OUTPUT_POWER, TX_TIMEOUT );
}
void setup() {
  // put your setup code here, to run once:
  Serial.begin(115200);
  Mcu.begin();
  RadioEvents.TxTimeout = OnRadioTxTimeout;
  Radio.Init( &RadioEvents );

  Radio.SetTxContinuousWave( RF_FREQUENCY, TX_OUTPUT_POWER, TX_TIMEOUT );
}

void loop() {
  // put your main code here, to run repeatedly:
  Radio.IrqProcess( );
}