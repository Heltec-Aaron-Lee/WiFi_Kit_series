#define LED1 25

void setup() {
   Serial.begin(115200);
   pinMode(LED1,OUTPUT);  
}

void loop() {
      Serial.print("This loop Task run on Core: ");
      Serial.println(xPortGetCoreID());    /*print the current core*/
      digitalWrite(LED1,HIGH);
      delay(1000);
      digitalWrite(LED1,LOW);
      delay(1000);
}
