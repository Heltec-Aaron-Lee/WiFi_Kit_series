/* 
  External interrupt sample test.
 
  by Aaron.Lee from HelTec AutoMation, ChengDu, China
  成都惠利特自动化科技有限公司
  www.heltec.cn
*/


#define LED     25    // GPIO5  -- SX127x's SCK
#define Key     0
unsigned int state = LOW;

void setup() {
  pinMode(LED, OUTPUT);
  digitalWrite(LED,LOW);
  pinMode(Key, INPUT);
  //noInterrupts(); //关闭总中断，类似51单片机里面的EA=0
  attachInterrupt(Key,function,RISING);
/*
attachInterrupt(interrupt, function, mode) //Arduino 中配置中断的函数

interrupt -- 电平信号、按键等外部中断
function  -- 中断服务函数
mode      -- 触发模式，有LOW、CHANGE、RISING、FALLING四中模式，功能见下表
---------------------------------------
       LOW     |  低电平触发
       CHANGE  |  电平发生改变时触发
       RISING  |  上升沿触发
       FALLING |  下降沿触发
---------------------------------------
*/
}

void loop() {
  // do nothing
}

void function()//中断服务函数
{
  state = !state;
  digitalWrite(LED,state);
}

