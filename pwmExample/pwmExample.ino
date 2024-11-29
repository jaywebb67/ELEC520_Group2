#define RedPin_1    23
#define GreenPin    22
// setting PWM properties
const int freq = 4;
const int resolution = 8;
 
void setup(){
  Serial.begin(9600);
  // configure LED PWM
  ledcAttach(RedPin_1, 4, resolution);
  ledcAttach(GreenPin, 4, resolution);
  // if you want to attach a specific channel, use the following instead
  //ledcAttachChannel(ledPin, freq, resolution, 0);
}
 
void loop(){
  ledcWrite(GreenPin, 128);
  ledcWrite(RedPin_1, 64);
  while(true){
    Serial.println("I'm working");
    delay(1000);
  }
}