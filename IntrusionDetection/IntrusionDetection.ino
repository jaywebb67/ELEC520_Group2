#include<Wire.h>

//// Ultrasonic Setup
// pin config
const int trigPin = 6;
const int echoPin = 7;
// variables
bool ultraSonicAlert = false;


//// LED setup
// pin config
const int redPin = 5;
// variables
long duration;


//// IMU Setup
// Stuff For Registers n' Wire etc 
const int MPU = 0x68;
// variables
float AccX, AccY, AccZ;
bool imuAlert = false;


//// Motion sensor setup
// pin config
const int mSensPin = 8;
// variables
int mSensValue = 0;


//// called on loading
void setup() {

  // configuring pins
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  pinMode(redPin, OUTPUT);
  pinMode(mSensPin, INPUT);

  //begining connection to serial with 19200 baud rate
  Serial.begin(19200);

  //initialising IMU registers
  Wire.begin();
  Wire.beginTransmission(MPU);
  Wire.write(0x6B);
  Wire.write(0x00);
  Wire.endTransmission(true);

}


// called on repeat
void loop() {
  UltraSonic();
  AccXYZ();
  MotionSensor();
  if (imuAlert || ultraSonicAlert || mSensValue){
    digitalWrite(redPin, HIGH);
  } else {
    digitalWrite(redPin, LOW);
  }
}

void AccXYZ(){
  Wire.beginTransmission(MPU);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU, 6, true);

  AccX = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccY = (Wire.read() << 8 | Wire.read()) / 16384.0;
  AccZ = (Wire.read() << 8 | Wire.read()) / 16384.0;

  if (AccZ > 1.1 || AccZ < 0.8 || AccX > 0.15 || AccX < -0.15 || AccY > 0.15 || AccY < -0.15){
    imuAlert = true;
  } else {
    imuAlert = false;
  }

}

void UltraSonic(){
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);

  //Serial.println();
  //Serial.print(duration);

  if (duration < 500){
    ultraSonicAlert = true;
  } else{
    ultraSonicAlert = false;
  }
}

void MotionSensor(){
  mSensValue = digitalRead(mSensPin);
  Serial.println(mSensValue);
}


