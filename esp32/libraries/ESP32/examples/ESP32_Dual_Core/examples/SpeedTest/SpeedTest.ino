#define LED1 25
long loops1 = 1000;
long loops2 = 1000;
long qq;
float t1;
int t2,t3;

TaskHandle_t Task1, Task2;
SemaphoreHandle_t baton;


void artificialLoad () {
  for ( long i = 0;i < loops1; i++){
    for ( long j = 1; j < loops2; j++) {
      qq++;
      t1 = 5000.0 * i;
      t2 = 150 * 1234 * i;
      t3 = j % 554 ;
    }
  }
}
void blink(){
      digitalWrite(LED1,HIGH);
      delay(1000);
      digitalWrite(LED1,LOW);
      delay(1000);
  }

void codeForTask1( void * parameter )
{
  for(;;){
      long start = millis();
      artificialLoad();
      Serial.print("Finish loop Task run on Core: ");
      Serial.print(xPortGetCoreID());
      Serial.print(" Time ");
      Serial.println(millis() - start);
//    blink();
   }
 }


void codeForTask2( void * parameter )
{
  for(;;){ 
//      long start = millis();
//      artificialLoad();
//      Serial.print("             Finish loop Task run on Core: ");
//      Serial.print(xPortGetCoreID());
//      Serial.print(" Time ");
//      Serial.println(millis() - start);
      delay(1000);
    }  
 }
void setup() {
   Serial.begin(115200);
   pinMode(LED1,OUTPUT);  
   baton = xSemaphoreCreateMutex();
   
   xTaskCreatePinnedToCore(
    codeForTask1,
    "Task_1",
    1000,
    NULL,
    1,
    &Task1,
    0);

  delay(500);
 
   xTaskCreatePinnedToCore(
    codeForTask2,
    "Task_2",
    1000,
    NULL,
    1,
    &Task2,
    1);  
}

void loop() {
      long start = millis();
      artificialLoad();
      Serial.print("                        Finish loop Task run on Core: ");
      Serial.print(xPortGetCoreID());
      Serial.print(" Time ");
      Serial.println(millis() - start);
      delay(10);
//      delay(5000);
}
