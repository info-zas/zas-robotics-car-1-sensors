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
#define IN1 4  
#define IN2 3  
#define ENA 5  
#define IN3 8  
#define IN4 7  
#define ENB 6  

// #define Left1 7
// #define Left2 8
// #define Left_Speed 6

// #define Right1 3
// #define Right2 4
// #define Right_Speed 5



void setup() {
  // put your setup code here, to run once:
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

  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  display.setCursor(0,0);
  display.println("System Initialized");
  display.display();

}

void loop() {
  // put your main code here, to run repeatedly:

  showText("forward");
  forward(150); delay(1000);
  
  showText("backward");
  backward(150); delay(1000);
  
  showText("left");
  left(150); delay(1000);
  
  showText("right");
  right(150); delay(1000);

  showText("stop");
  stop(); delay(1000);
  
}

void showText(const char* text) {
    Serial.println(text);
    display.clearDisplay();
    display.setCursor(0, 0);
    display.println("Current Mode:");
    display.println(text);
    display.display();
  
}

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
/*
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
*/