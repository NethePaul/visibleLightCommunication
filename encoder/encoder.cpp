#include<Arduino.h>
#include<WiFi.h>
#include<WebServer.h>
#include<Preferences.h>

#define LED 5
// #define START 22
// #define END 23

class Data{
  unsigned bi=0;
  int8_t ci=7+8;
  String data="";
  unsigned len=0;
public:
  bool end(){return bi>len+1;};
  void set(String data){
    len=data.length();
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
WebServer server(80);

Preferences p;

void set_data(){
  if(server.args()==0)return;
  auto a1=server.arg(0);
  data.set(a1.c_str());
  p.putString("data",a1);
  server.send(200);
};

void not_found(){
  server.send(404);
}

void setup(){
  p.begin("my-prefrence");
  WiFi.begin("ssid","password");
  server.on("/set", set_data);
  server.onNotFound(not_found);
  server.begin(80);
  pinMode(LED, OUTPUT);
  // pinMode(END, OUTPUT);
  // pinMode(START, INPUT);
  
  digitalWrite(LED, LOW);
  String str = p.getString("data","Hello World");
  data.set(str);
  start=millis();
}

void set(bool a){
  int led=LED;
  digitalWrite(led, a?LOW:HIGH);
 }


void loop(){
  server.handleClient();
  static bool a=true;
  // if(digitalRead(START)==LOW)return;
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

  if(data.end()){
    data.reset();
    // digitalWrite(END, HIGH);
    // delay(1);
    // digitalWrite(END, LOW);
  }
}
