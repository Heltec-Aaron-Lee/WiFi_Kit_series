#define LED1 25
TaskHandle_t Task1;

void codeForTask1( void * parameter )
{
  for(;;) {
          Serial.print("This Task run on Core: ");
          Serial.println(xPortGetCoreID());
          
          digitalWrite(LED1,HIGH);
          delay(1000);
          digitalWrite(LED1,LOW);
          delay(1000);
      }
}

  
void setup() {
   Serial.begin(115200);
   pinMode(LED1,OUTPUT);  

   xTaskCreatePinnedToCore(
    codeForTask1,           /*Task Function. */
    "Task_1",               /*name of task. */
    1000,                   /*Stack size of task. */
    NULL,                   /* parameter of the task. */
    1,                      /* proiority of the task. */
    &Task1,                 /* Task handel to keep tra ck of created task. */
    0);                     /* choose Core */
}

void loop() { 

}
