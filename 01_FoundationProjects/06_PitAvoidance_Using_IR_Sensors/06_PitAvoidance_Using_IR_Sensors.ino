// ============================================
// INTELLIGENT PIT AVOIDANCE WITH OLED DEBUG
// ============================================
// Black table = HIGH, White edge = LOW
// OLED on A4 (SDA) and A5 (SCL)
// ============================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// OLED Display settings
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// Motor pins
#define Left1 3
#define Left2 4
#define Left_Speed 5
#define Right1 7
#define Right2 8
#define Right_Speed 6

// Sensor pins
#define SENSOR_L A0
#define SENSOR_C1 A1
#define SENSOR_C2 A2
#define SENSOR_R A3

// Speed settings
int normalSpeed = 100;
int turnSpeed = 120;

// Statistics
unsigned long moveCount = 0;
unsigned long edgeDetections = 0;
String currentAction = "INIT";

void setup() {
  // Motor pins
  pinMode(Left1, OUTPUT);
  pinMode(Left2, OUTPUT);
  pinMode(Left_Speed, OUTPUT);
  pinMode(Right1, OUTPUT);
  pinMode(Right2, OUTPUT);
  pinMode(Right_Speed, OUTPUT);
  
  // Sensor pins
  pinMode(SENSOR_L, INPUT);
  pinMode(SENSOR_C1, INPUT);
  pinMode(SENSOR_C2, INPUT);
  pinMode(SENSOR_R, INPUT);
  
  Serial.begin(9600);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;);
  }
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(WHITE);
  
  // Startup screen
  showStartupScreen();
  delay(3000);
  
  Serial.println("System Ready!");
}

void loop() {
  // Read sensors (BLACK table = HIGH, WHITE edge = LOW)
  int L = digitalRead(SENSOR_L);
  int C1 = digitalRead(SENSOR_C1);
  int C2 = digitalRead(SENSOR_C2);
  int R = digitalRead(SENSOR_R);
  
  // Read analog values for display
  int aL = analogRead(SENSOR_L);
  int aC1 = analogRead(SENSOR_C1);
  int aC2 = analogRead(SENSOR_C2);
  int aR = analogRead(SENSOR_R);
  
  // Intelligent decision making
  makeDecision(L, C1, C2, R);
  
  // Update OLED display
  updateOLED(L, C1, C2, R, aL, aC1, aC2, aR);
  
  // Serial debug
  serialDebug(L, C1, C2, R);
  
  moveCount++;
  delay(50);
}

// ============================================
// INTELLIGENT DECISION ALGORITHM
// ============================================
void makeDecision(int L, int C1, int C2, int R) {
  
  // ALL ON BLACK TABLE - SAFE
  if(L == HIGH && C1 == HIGH && C2 == HIGH && R == HIGH) {
    currentAction = "FORWARD";
    forward(normalSpeed);
  }
  
  // ═══════════════════════════════════════════
  // SINGLE EDGE DETECTION
  // ═══════════════════════════════════════════
  
  // LEFT EDGE ONLY
  else if(L == LOW && C1 == HIGH && C2 == HIGH && R == HIGH) {
    currentAction = "L-EDGE";
    edgeDetections++;
    avoidLeft();
  }
  
  // RIGHT EDGE ONLY
  else if(L == HIGH && C1 == HIGH && C2 == HIGH && R == LOW) {
    currentAction = "R-EDGE";
    edgeDetections++;
    avoidRight();
  }
  
  // CENTER1 EDGE ONLY
  else if(L == HIGH && C1 == LOW && C2 == HIGH && R == HIGH) {
    currentAction = "C1-EDGE";
    edgeDetections++;
    avoidCenter1();
  }
  
  // CENTER2 EDGE ONLY
  else if(L == HIGH && C1 == HIGH && C2 == LOW && R == HIGH) {
    currentAction = "C2-EDGE";
    edgeDetections++;
    avoidCenter2();
  }
  
  // ═══════════════════════════════════════════
  // DOUBLE EDGE DETECTION (CORNER/DANGER)
  // ═══════════════════════════════════════════
  
  // LEFT CORNER (L + C1)
  else if(L == LOW && C1 == LOW && C2 == HIGH && R == HIGH) {
    currentAction = "L-CORNER";
    edgeDetections += 2;
    avoidLeftCorner();
  }
  
  // RIGHT CORNER (C2 + R)
  else if(L == HIGH && C1 == HIGH && C2 == LOW && R == LOW) {
    currentAction = "R-CORNER";
    edgeDetections += 2;
    avoidRightCorner();
  }
  
  // BOTH CENTERS (C1 + C2) - Narrow path ahead
  else if(L == HIGH && C1 == LOW && C2 == LOW && R == HIGH) {
    currentAction = "NARROW";
    edgeDetections += 2;
    avoidNarrowPath();
  }
  
  // BOTH SIDES (L + R) - Very narrow or stuck
  else if(L == LOW && C1 == HIGH && C2 == HIGH && R == LOW) {
    currentAction = "SIDES";
    edgeDetections += 2;
    avoidBothSides();
  }
  
  // ═══════════════════════════════════════════
  // TRIPLE EDGE DETECTION (CRITICAL)
  // ═══════════════════════════════════════════
  
  // LEFT TRIPLE (L + C1 + C2)
  else if(L == LOW && C1 == LOW && C2 == LOW && R == HIGH) {
    currentAction = "L-TRIPLE";
    edgeDetections += 3;
    emergencyRight();
  }
  
  // RIGHT TRIPLE (C1 + C2 + R)
  else if(L == HIGH && C1 == LOW && C2 == LOW && R == LOW) {
    currentAction = "R-TRIPLE";
    edgeDetections += 3;
    emergencyLeft();
  }
  
  // FRONT TRIPLE (L + C1 + R or L + C2 + R)
  else if((L == LOW && C1 == LOW && C2 == HIGH && R == LOW) ||
          (L == LOW && C1 == HIGH && C2 == LOW && R == LOW)) {
    currentAction = "F-TRIPLE";
    edgeDetections += 3;
    emergencyBackup();
  }
  
  // ═══════════════════════════════════════════
  // ALL EDGES - EMERGENCY (Robot lifted or all edges)
  // ═══════════════════════════════════════════
  
  else if(L == LOW && C1 == LOW && C2 == LOW && R == LOW) {
    currentAction = "EMERGENCY";
    edgeDetections += 4;
    fullEmergency();
  }
  
  // ═══════════════════════════════════════════
  // UNKNOWN PATTERN - SAFE DEFAULT
  // ═══════════════════════════════════════════
  
  else {
    currentAction = "UNKNOWN";
    safeDefault();
  }
}

// ============================================
// AVOIDANCE MANEUVERS
// ============================================

// Single edge avoidance
void avoidLeft() {
  backward(normalSpeed);
  delay(150);
  turnRight(turnSpeed);
  delay(250);
}

void avoidRight() {
  backward(normalSpeed);
  delay(150);
  turnLeft(turnSpeed);
  delay(250);
}

void avoidCenter1() {
  backward(normalSpeed);
  delay(150);
  turnLeft(turnSpeed);
  delay(200);
}

void avoidCenter2() {
  backward(normalSpeed);
  delay(150);
  turnRight(turnSpeed);
  delay(200);
}

// Corner avoidance
void avoidLeftCorner() {
  backward(normalSpeed);
  delay(250);
  turnRight(turnSpeed);
  delay(400);
}

void avoidRightCorner() {
  backward(normalSpeed);
  delay(250);
  turnLeft(turnSpeed);
  delay(400);
}

void avoidNarrowPath() {
  backward(normalSpeed);
  delay(200);
  turnLeft(turnSpeed);
  delay(350);
}

void avoidBothSides() {
  backward(normalSpeed);
  delay(300);
  turnLeft(turnSpeed);
  delay(450);
}

// Emergency maneuvers
void emergencyLeft() {
  Stop();
  delay(100);
  backward(turnSpeed);
  delay(400);
  turnLeft(turnSpeed);
  delay(600);
}

void emergencyRight() {
  Stop();
  delay(100);
  backward(turnSpeed);
  delay(400);
  turnRight(turnSpeed);
  delay(600);
}

void emergencyBackup() {
  Stop();
  delay(100);
  backward(turnSpeed);
  delay(500);
  turnLeft(turnSpeed);
  delay(700);
}

void fullEmergency() {
  Stop();
  delay(200);
  backward(turnSpeed);
  delay(600);
  turnLeft(turnSpeed);
  delay(800);
}

void safeDefault() {
  backward(normalSpeed);
  delay(200);
  turnLeft(turnSpeed);
  delay(300);
}

// ============================================
// MOTOR CONTROL FUNCTIONS
// ============================================

void forward(int speed) {
  analogWrite(Left_Speed, speed);
  analogWrite(Right_Speed, speed);
  digitalWrite(Left1, HIGH);
  digitalWrite(Left2, LOW);
  digitalWrite(Right1, HIGH);
  digitalWrite(Right2, LOW);
}

void backward(int speed) {
  analogWrite(Left_Speed, speed);
  analogWrite(Right_Speed, speed);
  digitalWrite(Left1, LOW);
  digitalWrite(Left2, HIGH);
  digitalWrite(Right1, LOW);
  digitalWrite(Right2, HIGH);
}

void turnLeft(int speed) {
  analogWrite(Left_Speed, speed);
  analogWrite(Right_Speed, speed);
  digitalWrite(Left1, LOW);
  digitalWrite(Left2, HIGH);
  digitalWrite(Right1, HIGH);
  digitalWrite(Right2, LOW);
}

void turnRight(int speed) {
  analogWrite(Left_Speed, speed);
  analogWrite(Right_Speed, speed);
  digitalWrite(Left1, HIGH);
  digitalWrite(Left2, LOW);
  digitalWrite(Right1, LOW);
  digitalWrite(Right2, HIGH);
}

void Stop() {
  digitalWrite(Left1, LOW);
  digitalWrite(Left2, LOW);
  digitalWrite(Right1, LOW);
  digitalWrite(Right2, LOW);
}

// ============================================
// OLED DISPLAY FUNCTIONS
// ============================================

void showStartupScreen() {
  display.clearDisplay();
  display.setTextSize(2);
  display.setCursor(10, 5);
  display.println("PIT AVOID");
  
  display.setTextSize(1);
  display.setCursor(15, 28);
  display.println("Intelligent AI");
  
  display.setCursor(5, 45);
  display.println("Black Table Mode");
  
  display.display();
}

void updateOLED(int L, int C1, int C2, int R, int aL, int aC1, int aC2, int aR) {
  display.clearDisplay();
  
  // Title
  display.setTextSize(1);
  display.setCursor(0, 0);
  display.println("PIT AVOIDANCE DEBUG");
  display.drawLine(0, 10, 128, 10, WHITE);
  
  // Sensor status (visual representation)
  display.setCursor(0, 14);
  display.print("Sensors: [");
  display.print(L ? "1" : "0");
  display.print("][");
  display.print(C1 ? "1" : "0");
  display.print("][");
  display.print(C2 ? "1" : "0");
  display.print("][");
  display.print(R ? "1" : "0");
  display.println("]");
  
  // Visual sensor bar
  display.setCursor(0, 24);
  display.print("L:");
  drawSensorBar(12, 24, L);
  display.setCursor(38, 24);
  display.print("C1:");
  drawSensorBar(56, 24, C1);
  
  display.setCursor(0, 32);
  display.print("C2:");
  drawSensorBar(20, 32, C2);
  display.setCursor(46, 32);
  display.print("R:");
  drawSensorBar(60, 32, R);
  
  // Current action
  display.setCursor(0, 42);
  display.print("Action: ");
  display.setTextSize(1);
  display.println(currentAction);
  
  // Statistics
  display.setTextSize(1);
  display.setCursor(0, 52);
  display.print("Moves:");
  display.print(moveCount);
  display.print(" Edges:");
  display.println(edgeDetections);
  
  display.display();
}

void drawSensorBar(int x, int y, int value) {
  if(value == HIGH) {
    display.fillRect(x, y, 18, 6, WHITE);  // Full bar (on black)
  } else {
    display.drawRect(x, y, 18, 6, WHITE);  // Empty bar (at edge)
  }
}

// ============================================
// SERIAL DEBUG
// ============================================

void serialDebug(int L, int C1, int C2, int R) {
  Serial.print("Sensors:[");
  Serial.print(L);
  Serial.print("][");
  Serial.print(C1);
  Serial.print("][");
  Serial.print(C2);
  Serial.print("][");
  Serial.print(R);
  Serial.print("] Action: ");
  Serial.print(currentAction);
  Serial.print(" | Edges: ");
  Serial.print(edgeDetections);
  Serial.print(" | Moves: ");
  Serial.println(moveCount);
}