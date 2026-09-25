#include <Servo.h>

const int L293D_IN1 = 2;
const int L293D_IN2 = 3;

Servo myServo;
const int servoPin = 9;

const int LEFT = 77;
const int STRAIGHT = 89;
const int RIGHT = 102;

void setup() {
  pinMode(L293D_IN1, OUTPUT);
  pinMode(L293D_IN2, OUTPUT);

  myServo.attach(servoPin);
  myServo.write(STRAIGHT);
}

void loop() {
  // Motor forward + servo straight
  myServo.write(STRAIGHT);

  digitalWrite(L293D_IN1, HIGH);
  digitalWrite(L293D_IN2, LOW);
  delay(3000);

  // Stop
  digitalWrite(L293D_IN1, LOW);
  digitalWrite(L293D_IN2, LOW);
  delay(1000);

  // Motor backward + servo left
  myServo.write(LEFT);

  digitalWrite(L293D_IN1, LOW);
  digitalWrite(L293D_IN2, HIGH);
  delay(3000);

  // Stop
  digitalWrite(L293D_IN1, LOW);
  digitalWrite(L293D_IN2, LOW);
  delay(1000);

  // Servo right
  myServo.write(RIGHT);
  delay(1000);

  // Back to straight
  myServo.write(STRAIGHT);
}