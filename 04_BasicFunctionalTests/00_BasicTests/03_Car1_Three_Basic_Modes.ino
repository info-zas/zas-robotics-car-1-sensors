#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#include <Servo.h>
Servo myServo;

// OLED config
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ----------- Motor Pins (confirmed) ------------
#define IN1 8  
#define IN2 7  
#define ENA 6  
#define IN3 4  
#define IN4 3  
#define ENB 5  

// ----------- Ultrasonic Sensor Pins ------------
const int trigPin = 10;
const int echoPin = 11;

// ----------- IR Sensor Pins --------------------
#define IR_LEFT         A0
#define IR_LEFT_CENTER  A1
#define IR_RIGHT_CENTER A2
#define IR_RIGHT        A3

// ----------- Mode Selection Button ------------
const int buttonPin = 2;

// ----------- Mode Handling ---------------------
volatile int mode = 0;
const int TOTAL_MODES = 5; // 0-5 (Idle + 6 Modes)

// ----------- Setup -----------------------------

void setup() {
  Serial.begin(9600);

  /// SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  myServo.attach(9);   // Servo motor for ultrasonic
  myServo.write(90);   // Center the servo initially


  // Motor Pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);

  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);

  pinMode(IR_LEFT, INPUT);
  pinMode(IR_LEFT_CENTER, INPUT);
  pinMode(IR_RIGHT_CENTER, INPUT);
  pinMode(IR_RIGHT, INPUT);

  pinMode(buttonPin, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(buttonPin), changeMode, FALLING);

  stop();

  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("System Initialized");
  display.display();
  delay(1000);
}

// ----------- Main Loop --------------------------

void loop() {
  switch (mode) {
    case 0:
      showMode("Idle Mode");
      stop();
      delay(500);
      break;
    case 1:
      showMode("Basic Movement");
      basicMovement();
      break;
    case 2:
      showMode("Servo Sweep");
      servoSweep();
      break;
    case 3:
      showMode("Ultrasonic OA");
      ultrasonicObstacleAvoidanceServo();
      break;
    case 4:
      showMode("IR Obstacle OA");
      irObstacleAvoidance();
      break;
    default:
      mode = 0;
      break;
  }
}


// ------------- Basic Movement Mode ------------

void basicMovement() {

forward(150); delay(1000);


backward(150); delay(1000);


left(150); delay(1000);


right(150); delay(1000);


stop(); delay(1000);

}

void servoSweep(){
  // --- Sweep UP (from 45 to 135 degrees) ---
  for (int pos = 45; pos <= 135; pos += 5) { 
    // Set the servo position to the current angle
    myServo.write(pos); 
    // Wait for the servo to reach the position
    delay(50); 
  }

  // Add a small pause at the max angle before sweeping back
  delay(500); 

  // --- Sweep DOWN (from 135 to 45 degrees) ---
  for (int pos = 135; pos >= 45; pos -= 5) {
    // Set the servo position to the current angle
    myServo.write(pos); 
    // Wait for the servo to reach the position
    delay(50);
  }
  
  // Add a small pause at the min angle before sweeping up again
  delay(500);
}

void ultrasonicObstacleAvoidanceServo() {
  int centerDistance, leftDistance, rightDistance;

  // Center
  myServo.write(90);
  delay(400);
  centerDistance = getUltrasonicDistance();

  if (centerDistance < 30) {
    stop();

    // Look Left
    myServo.write(45);
    delay(400);
    leftDistance = getUltrasonicDistance();

    // Look Right
    myServo.write(135);
    delay(400);
    rightDistance = getUltrasonicDistance();

    // Reset to center
    myServo.write(90);
    delay(100);

    if (leftDistance > rightDistance) {
      left(150); delay(800);
    } else {
      right(150); delay(800);
    }
  } else {
    forward(150);
  }

  delay(100);
}

// ------------- Ultrasonic Obstacle Avoidance ------------

void ultrasonicObstacleAvoidance() {
  long duration, distance;
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  duration = pulseIn(echoPin, HIGH);
  distance = duration * 0.034 / 2;
  // Print distance to Serial for debugging
  Serial.print("Distance: ");
  Serial.print(distance);
  Serial.println(" cm");

  // Display distance on OLED
  
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0, 20);
  display.fillRect(0, 20, SCREEN_WIDTH, 10, BLACK); // Clear previous distance reading area only
  display.print("Dist: ");
  display.print(distance);
  display.print(" cm");
  display.display();

  
  /* -----------------------------
  enabling the displayAction in the if-else - causing the SSD1306 init to fail
  *************** this need investigation ********************
  */

  if (distance < 20) {
  
  stop(); backward(150); delay(500);
  right(150); delay(500);
  } else {
  forward(150);
  }
  delay(100);

}

// ------------- IR Obstacle Avoidance ------------

void irObstacleAvoidance() {
  int leftIR = digitalRead(IR_LEFT);
  int rightIR = digitalRead(IR_RIGHT);

   int leftIRCenter = digitalRead(IR_LEFT_CENTER);
  int rightIRCenter = digitalRead(IR_RIGHT_CENTER);

  if (leftIR == HIGH || rightIR == HIGH || rightIRCenter == HIGH || leftIRCenter == HIGH) {
  stop(); backward(150); delay(500);
  right(150); delay(500);
} else {
  forward(150);
}
  delay(100);
}



void bluetoothControl() {
  if (Serial.available()) {
    char cmd = Serial.read();
    Serial.print("BT Cmd:");
    Serial.println(cmd);
    switch (cmd) {
      case '0':
        
        stop();
        break;

      case '1':
        
        forward(150);
        break;

      case '2':
        
        backward(150);
        break;

      case '3':
        
        left(150);
        break;

      case '4':
        
        right(150);
        break;

      default:
        
        stop();
        break;
    }
  }

  delay(100);
}


void externalWallFollowingLeft(){
  int L = digitalRead(IR_LEFT);
  if (L == 1) {
    // Wall detected, follow forward
    forward(150);
  } else {
    // Wall lost, turn left to search for wall
    forward(150);
    delay(120);
    left(150);
    delay(150);
  }
}

void externalWallFollowingRight(){
  int R = digitalRead(IR_RIGHT);
  if (R == 1) {
    // Wall detected, follow forward
    forward(150);
  } else {
    // Wall lost, turn left to search for wall
    forward(150);
    delay(120);
    right(150);
    delay(150);
  }

}

void internalWallFollowingRight(){
   int L = digitalRead(A0);
  int LC = digitalRead(A1);
  int RC = digitalRead(A2);
  int R = digitalRead(A3);


  if(L == LOW && LC == LOW){
  stpLeft(127);
}
else if(L == HIGH && LC == LOW){
  stpRight(127);
}
else if(L == LOW && LC == HIGH){
  right(127);
  delay(500);
}
else if(L == HIGH && LC == HIGH){
  right(127);
  delay(500);
}

}

void internalWallFollowingLeft(){
   int L = digitalRead(A0);
  int LC = digitalRead(A1);
  int RC = digitalRead(A2);
  int R = digitalRead(A3);


  if(R == LOW && RC == LOW){
  stpRight(127);
}
else if(R == HIGH && RC == LOW){
  stpLeft(127);
}
else if(R == LOW && RC == HIGH){
  left(127);
  delay(500);
}
else if(R == HIGH && RC == HIGH){
  left(127);
  delay(500);
}

}

int getUltrasonicDistance() {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH);
  int distance = duration * 0.034 / 2;
  return distance;
}


// ----------- ISR for Mode Change --------------

void changeMode() {
  mode++;
  if (mode >= TOTAL_MODES) mode = 0;
  delay(300);
}

// ----------- Show Mode (Serial + OLED) --------------

void showMode(const char* text) {
  static int lastMode = -1;
  if (lastMode != mode) {
    Serial.println(text);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Current Mode:");
    display.println(text);
    display.display();
    lastMode = mode;
  }
}

// ----------- Motor Driver Functions --------------

void stop() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
}

void forward(int speed) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void backward(int speed) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void left(int speed) {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void right(int speed) {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void stpRight(int speed) { 
  digitalWrite(IN1, HIGH); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);   
  digitalWrite(IN4, LOW); 
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

void stpLeft(int speed) { 
  digitalWrite(IN1, LOW); 
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, HIGH);   
  digitalWrite(IN4, LOW); 
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}




void lineFollowing(){
int rc = digitalRead(IR_RIGHT_CENTER);
int lc = digitalRead(IR_LEFT_CENTER);
int l  = digitalRead(IR_LEFT);
int r = digitalRead(IR_RIGHT);

Serial.print("right: ");
Serial.print(r);
Serial.print("left: ");
Serial.println(l);

if((digitalRead(IR_RIGHT_CENTER) == 1)&&(digitalRead(IR_LEFT_CENTER) == 1)){forward(100);}   //if Right Sensor and Left Sensor are at White color then it will call forword function

if((digitalRead(IR_RIGHT_CENTER) == 0)&&(digitalRead(IR_LEFT_CENTER) == 1)){right(100);} //if Right Sensor is Black and Left Sensor is White then it will call turn Right function  

if((digitalRead(IR_RIGHT_CENTER) == 1)&&(digitalRead(IR_LEFT_CENTER) == 0)){left(100);}  //if Right Sensor is White and Left Sensor is Black then it will call turn Left function

if((digitalRead(IR_RIGHT_CENTER) == 0)&&(digitalRead(IR_LEFT_CENTER) == 0)){stop();} //if Right Sensor and Left Sensor are at Black color then it will call Stop function

if((digitalRead(IR_LEFT) == 0)&&(digitalRead(IR_RIGHT_CENTER) == 1)&&(digitalRead(IR_LEFT_CENTER) == 1)&&(digitalRead(IR_RIGHT) == 1)){left(100); delay(50);}

if((digitalRead(IR_RIGHT) == 0)&&(digitalRead(IR_RIGHT_CENTER) == 1)&&(digitalRead(IR_LEFT_CENTER) == 1)&& (digitalRead(IR_LEFT) == 1)){right(100);delay(50);}

}




void pitAvoidance() {
  int L = digitalRead(A0);
  int C1 = digitalRead(A1);
  int C2 = digitalRead(A2); // This pin is read but unused in your original logic; you may include it if needed
  int R = digitalRead(A3);

  if (L == HIGH && C1 == HIGH && R == HIGH) {
    forward(100);
  }
  else if (L == HIGH && C1 == LOW && R == HIGH) {
    backward(100); delay(200);
    left(150); delay(300);
  }
  else if (L == LOW && C1 == HIGH && R == HIGH) {
    right(150);
  }
  else if (L == HIGH && C1 == HIGH && R == LOW) {
    left(150);
  }
  else if (L == LOW && C1 == HIGH && R == LOW) {
    backward(150); delay(200);
    left(150); delay(300);
  }
  else if (L == LOW && C1 == LOW && R == HIGH) {
    backward(100); delay(200);
    right(150); delay(300);
  }
  else if (L == HIGH && C1 == LOW && R == LOW) {
    backward(100); delay(200);
    left(150); delay(300);
  }
  else if (L == LOW && C1 == LOW && R == LOW) {
    backward(100); delay(200);
    left(150); delay(300);
  }

  delay(100);
}






