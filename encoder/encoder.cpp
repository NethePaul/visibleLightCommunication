#include<Arduino.h>

#define LED 5

class Data{
  unsigned bi=0;
  int8_t ci=7+8;
  const char*data=0;
  unsigned len=0;
public:
  bool end(){return bi>len;};
  void set(const char*data){
    len=strlen(data)+1;
    this->data=data;
    reset();
  }
  void reset(){
    bi=0;
    ci=7+12;
  }
  bool getNextBit(){
    if(ci>=8){
      ci--;
      return 1;
    }
    bool d=0;
      
    
    if(end())return 0;
    
    if(bi<len){
      d = (data[bi]>>ci)&1;
    }
    ci--;

    if(ci<0){
      ci=7;
      bi++;
    }
    
    return d;
  }
};

Data data;

unsigned long start;

void setup(){
  pinMode(LED, OUTPUT);

  digitalWrite(LED, LOW);
  data.set("Hello World");
  start=millis();
}

void set(bool a){
  int led=LED;
  digitalWrite(led, a?LOW:HIGH);
 }


void loop(){
  static bool a=true;
  set(a);
  a=!a;

  bool bit=data.getNextBit();
  
  if(bit){
    start+=130;
    unsigned long d=millis();
    delay(start-d);
  }
  else{
    start+=60;
    unsigned long d=millis();
    delay(start-d);
  }

  if(data.end())data.reset();
}
