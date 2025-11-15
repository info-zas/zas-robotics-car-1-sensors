#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Ultrasonic pins
#define TRIG_PIN 10
#define ECHO_PIN 11

// Servo pin
#define SERVO_PIN 9
Servo myServo;

// Motor driver pins (TB6612)
#define PWMA 5
#define AIN1 3
#define AIN2 4

void setup() {
  Serial.begin(9600);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println("OLED not found!");
    for (;;);
  }
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Servo init
  myServo.attach(SERVO_PIN);

  // Motor setup
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);

  // Ultrasonic setup
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
}

long readUltrasonic() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);

  long duration = pulseIn(ECHO_PIN, HIGH);
  long distance = duration * 0.034 / 2; // in cm
  return distance;
}

void loop() {
  // Read ultrasonic distance
  long distance = readUltrasonic();

  // Move servo (for demo, sweep slowly)
  static int angle = 0;
  myServo.write(angle);
  angle += 2;
  if (angle > 180) angle = 0;

  // Motor rotates in 1 direction at PWM = 100
  digitalWrite(AIN1, HIGH);
  digitalWrite(AIN2, LOW);
  analogWrite(PWMA, 100);

  // Display on OLED
  display.clearDisplay();
  display.setCursor(0, 10);
  display.print("Distance: ");
  display.print(distance);
  display.println(" cm");

  display.setCursor(0, 30);
  display.print("Servo Angle: ");
  display.print(angle);

  display.display();

  delay(200);
}
