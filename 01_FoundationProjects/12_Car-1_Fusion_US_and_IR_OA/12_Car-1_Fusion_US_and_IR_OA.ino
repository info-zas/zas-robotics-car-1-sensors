// ============================================
// FUSION OBSTACLE AVOIDANCE SYSTEM
// IR (Close Range) + Ultrasonic (Long Range)
// ============================================

#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ============================================
// OLED CONFIGURATION
// ============================================
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ============================================
// PIN DEFINITIONS
// ============================================
// Ultrasonic Sensor
#define TRIG_PIN 10
#define ECHO_PIN 11
#define SERVO_PIN 9

// Motor Pins
#define LEFT1 3        // IN1
#define LEFT2 4        // IN2
#define LEFT_SPEED 5   // ENA (PWM)
#define RIGHT1 8       // IN3
#define RIGHT2 7       // IN4
#define RIGHT_SPEED 6  // ENB (PWM)

// IR Sensors
#define IR_LEFT         A0
#define IR_LEFT_CENTER  A1
#define IR_RIGHT_CENTER A2
#define IR_RIGHT        A3

// ============================================
// SENSOR CONFIGURATION
// ============================================
// IR Thresholds
int IR_OBSTACLE = 0;
int IR_CLEAR = 1;

// Ultrasonic Thresholds (cm)
#define US_DANGER_DISTANCE 20      // Emergency stop
#define US_WARNING_DISTANCE 40     // Start planning
#define US_SCAN_DISTANCE 60        // Initiate scanning
#define US_MAX_VALID_DISTANCE 400  // Max valid reading

// ============================================
// MOTION CONFIGURATION
// ============================================
#define SPEED_NORMAL 120         // Normal cruising speed
#define SPEED_SLOW 80            // Slow speed when obstacle near
#define SPEED_TURN 130           // Turn speed
#define TURN_TIME 400            // Turn duration (ms)
#define BACKUP_TIME 350          // Backup duration (ms)

// ============================================
// DISPLAY UPDATE INTERVALS
// ============================================
#define DISPLAY_UPDATE_INTERVAL 200
#define SERIAL_UPDATE_INTERVAL 400

// ============================================
// GLOBAL VARIABLES
// ============================================
Servo scanner;

// Sensor readings
int irLeft = 0;
int irLeftCenter = 0;
int irRightCenter = 0;
int irRight = 0;
int usDistance = 0;
int usLeft = 0;
int usRight = 0;

// State tracking
String currentAction = "Initializing";
String sensorPriority = "None";
unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialUpdate = 0;
unsigned long loopCount = 0;

// ============================================
// ULTRASONIC SENSOR FUNCTION
// ============================================
int getUltrasonicDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  
  if (duration == 0) {
    return US_MAX_VALID_DISTANCE; // No echo
  }
  
  int distance = duration * 0.034 / 2;
  
  // Validate reading
  if (distance > US_MAX_VALID_DISTANCE || distance < 2) {
    return US_MAX_VALID_DISTANCE;
  }
  
  return distance;
}

// ============================================
// IR SENSOR AUTO-CALIBRATION
// ============================================
void calibrateIRSensors() {
  Serial.println(F("\n╔════════════════════════════════════════╗"));
  Serial.println(F("║   IR SENSOR CALIBRATION                ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println(F("→ Place robot in OPEN SPACE"));
  Serial.println(F("→ Calibrating in 2 seconds...\n"));
  
  delay(2000);
  
  int openLeft = digitalRead(IR_LEFT);
  int openLC = digitalRead(IR_LEFT_CENTER);
  int openRC = digitalRead(IR_RIGHT_CENTER);
  int openRight = digitalRead(IR_RIGHT);
  
  int clearCount = openLeft + openLC + openRC + openRight;
  
  if (clearCount >= 2) {
    IR_CLEAR = 1;
    IR_OBSTACLE = 0;
    Serial.println(F("✅ IR Mode: ACTIVE LOW (Clear=1, Obstacle=0)"));
  } else {
    IR_CLEAR = 0;
    IR_OBSTACLE = 1;
    Serial.println(F("✅ IR Mode: ACTIVE HIGH (Clear=0, Obstacle=1)"));
  }
  
  Serial.println(F("✅ IR Calibration Complete!\n"));
  delay(1000);
}

// ============================================
// SENSOR READING FUNCTIONS
// ============================================
void readAllSensors() {
  // Read IR sensors
  irLeft = digitalRead(IR_LEFT);
  irLeftCenter = digitalRead(IR_LEFT_CENTER);
  irRightCenter = digitalRead(IR_RIGHT_CENTER);
  irRight = digitalRead(IR_RIGHT);
  
  // Read ultrasonic (center position)
  scanner.write(90);
  delay(50);
  usDistance = getUltrasonicDistance();
}

// ============================================
// IR SENSOR ANALYSIS
// ============================================
bool irDetectObstacleLeft() {
  return (irLeft == IR_OBSTACLE || irLeftCenter == IR_OBSTACLE);
}

bool irDetectObstacleRight() {
  return (irRight == IR_OBSTACLE || irRightCenter == IR_OBSTACLE);
}

bool irDetectObstacleCenter() {
  return (irLeftCenter == IR_OBSTACLE && irRightCenter == IR_OBSTACLE);
}

bool irAllClear() {
  return (irLeft == IR_CLEAR && irLeftCenter == IR_CLEAR && 
          irRightCenter == IR_CLEAR && irRight == IR_CLEAR);
}

bool irCenterClear() {
  return (irLeftCenter == IR_CLEAR && irRightCenter == IR_CLEAR);
}

// ============================================
// MOTOR CONTROL FUNCTIONS
// ============================================
void setMotors(int in1, int in2, int in3, int in4, int speedL, int speedR) {
  digitalWrite(LEFT1, in1);
  digitalWrite(LEFT2, in2);
  digitalWrite(RIGHT1, in3);
  digitalWrite(RIGHT2, in4);
  analogWrite(LEFT_SPEED, speedL);
  analogWrite(RIGHT_SPEED, speedR);
}

void stopMotors() {
  setMotors(LOW, LOW, LOW, LOW, 0, 0);
  currentAction = "STOP";
}

void forward(int speed) {
  setMotors(HIGH, LOW, LOW, HIGH, speed, speed);
  currentAction = "FORWARD";
}

void backward(int speed) {
  setMotors(LOW, HIGH, HIGH, LOW, speed, speed);
  currentAction = "BACKWARD";
}

void turnLeft(int speed) {
  setMotors(LOW, HIGH, LOW, HIGH, speed, speed);
  currentAction = "TURN LEFT";
}

void turnRight(int speed) {
  setMotors(HIGH, LOW, HIGH, LOW, speed, speed);
  currentAction = "TURN RIGHT";
}

// ============================================
// ULTRASONIC SCANNING FUNCTION
// ============================================
void scanWithUltrasonic() {
  Serial.println(F("\n🔍 ULTRASONIC SCANNING..."));
  
  stopMotors();
  delay(200);
  
  // Scan Right (45°)
  Serial.print(F("  → Scanning Right (45°)... "));
  scanner.write(45);
  delay(400);
  usRight = getUltrasonicDistance();
  Serial.print(usRight);
  Serial.println(F(" cm"));
  
  // Return to center
  scanner.write(90);
  delay(300);
  
  // Scan Left (135°)
  Serial.print(F("  → Scanning Left (135°)... "));
  scanner.write(135);
  delay(400);
  usLeft = getUltrasonicDistance();
  Serial.print(usLeft);
  Serial.println(F(" cm"));
  
  // Return to center
  scanner.write(90);
  delay(300);
  
  Serial.println(F("\n📊 Scan Results:"));
  Serial.print(F("  Left:  ")); Serial.print(usLeft); Serial.println(F(" cm"));
  Serial.print(F("  Right: ")); Serial.print(usRight); Serial.println(F(" cm"));
}

// ============================================
// DECISION MAKING - BASED ON SCAN
// ============================================
void makeUltrasonicDecision() {
  // Both paths blocked
  if (usLeft < US_WARNING_DISTANCE && usRight < US_WARNING_DISTANCE) {
    Serial.println(F("⚠️ Both paths blocked! 180° turn..."));
    
    backward(SPEED_NORMAL);
    delay(BACKUP_TIME);
    stopMotors();
    delay(200);
    
    turnRight(SPEED_TURN);
    delay(TURN_TIME * 2); // 180°
    stopMotors();
    delay(200);
  }
  // Right is clearer
  else if (usRight > usLeft) {
    Serial.println(F("✅ RIGHT path clearer"));
    
    turnRight(SPEED_TURN);
    delay(TURN_TIME);
    stopMotors();
    delay(200);
  }
  // Left is clearer
  else {
    Serial.println(F("✅ LEFT path clearer"));
    
    turnLeft(SPEED_TURN);
    delay(TURN_TIME);
    stopMotors();
    delay(200);
  }
  
  Serial.println(F("🚀 Resuming movement\n"));
}

// ============================================
// 🔥 FUSION ALGORITHM - MAIN DECISION LOGIC
// ============================================
void fusionDecisionAlgorithm() {
  readAllSensors();
  loopCount++;
  
  // ═══════════════════════════════════════════
  // PRIORITY 1: IR IMMEDIATE OBSTACLE (HIGHEST)
  // Close range danger - immediate action
  // ═══════════════════════════════════════════
  
  if (!irAllClear()) {
    sensorPriority = "IR (Close)";
    
    // Left side blocked
    if (irDetectObstacleLeft() && !irDetectObstacleRight()) {
      Serial.println(F("🚨 IR: LEFT OBSTACLE - Immediate right turn"));
      
      stopMotors();
      delay(100);
      
      backward(SPEED_NORMAL);
      delay(BACKUP_TIME);
      stopMotors();
      delay(100);
      
      turnRight(SPEED_TURN);
      delay(TURN_TIME);
      stopMotors();
      delay(200);
      return;
    }
    
    // Right side blocked
    if (irDetectObstacleRight() && !irDetectObstacleLeft()) {
      Serial.println(F("🚨 IR: RIGHT OBSTACLE - Immediate left turn"));
      
      stopMotors();
      delay(100);
      
      backward(SPEED_NORMAL);
      delay(BACKUP_TIME);
      stopMotors();
      delay(100);
      
      turnLeft(SPEED_TURN);
      delay(TURN_TIME);
      stopMotors();
      delay(200);
      return;
    }
    
    // Both sides blocked
    if (!irCenterClear()) {
      Serial.println(F("🚨 IR: CENTER BLOCKED - Emergency backup"));
      
      stopMotors();
      delay(100);
      
      backward(SPEED_NORMAL);
      delay(BACKUP_TIME * 2);
      stopMotors();
      delay(200);
      
      // Use ultrasonic to decide direction
      scanWithUltrasonic();
      makeUltrasonicDecision();
      return;
    }
  }
  
  // ═══════════════════════════════════════════
  // PRIORITY 2: ULTRASONIC DANGER ZONE
  // Medium range - plan ahead
  // ═══════════════════════════════════════════
  
  if (usDistance <= US_DANGER_DISTANCE && usDistance > 0) {
    sensorPriority = "US (Danger)";
    Serial.print(F("⚠️ US: DANGER ZONE! Distance: "));
    Serial.print(usDistance);
    Serial.println(F(" cm"));
    
    // Verify with IR
    if (irCenterClear()) {
      Serial.println(F("  → IR confirms clear - US might be false positive"));
      Serial.println(F("  → Slowing down and continuing"));
      forward(SPEED_SLOW);
      delay(100);
      return;
    } else {
      Serial.println(F("  → IR also detects obstacle - stopping"));
      stopMotors();
      delay(200);
      
      scanWithUltrasonic();
      makeUltrasonicDecision();
      return;
    }
  }
  
  // ═══════════════════════════════════════════
  // PRIORITY 3: ULTRASONIC WARNING ZONE
  // Moderate distance - start planning
  // ═══════════════════════════════════════════
  
  if (usDistance <= US_WARNING_DISTANCE && usDistance > US_DANGER_DISTANCE) {
    sensorPriority = "US (Warning)";
    
    if (irAllClear()) {
      Serial.print(F("⚠️ US: Obstacle at "));
      Serial.print(usDistance);
      Serial.println(F(" cm - slowing down"));
      forward(SPEED_SLOW);
      delay(50);
      return;
    }
  }
  
  // ═══════════════════════════════════════════
  // PRIORITY 4: ULTRASONIC SCAN ZONE
  // Far distance - initiate scanning
  // ═══════════════════════════════════════════
  
  if (usDistance <= US_SCAN_DISTANCE && usDistance > US_WARNING_DISTANCE) {
    sensorPriority = "US (Scan)";
    
    if (irAllClear()) {
      Serial.print(F("📡 US: Obstacle ahead at "));
      Serial.print(usDistance);
      Serial.println(F(" cm - scanning..."));
      
      scanWithUltrasonic();
      
      // If clearer path found, turn proactively
      if (usLeft > usDistance + 20 || usRight > usDistance + 20) {
        makeUltrasonicDecision();
      } else {
        forward(SPEED_SLOW);
      }
      return;
    }
  }
  
  // ═══════════════════════════════════════════
  // PRIORITY 5: ALL CLEAR - NORMAL OPERATION
  // ═══════════════════════════════════════════
  
  if (irAllClear() && usDistance > US_SCAN_DISTANCE) {
    sensorPriority = "Clear";
    forward(SPEED_NORMAL);
    delay(50);
    return;
  }
  
  // Default - stop and reassess
  stopMotors();
  delay(100);
}

// ============================================
// OLED DISPLAY UPDATE
// ============================================
void updateDisplay() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDisplayUpdate < DISPLAY_UPDATE_INTERVAL) {
    return;
  }
  
  lastDisplayUpdate = currentTime;
  
  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  
  // Title
  display.setCursor(0, 0);
  display.println(F("FUSION MODE"));
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // IR Sensor Status
  display.setCursor(0, 14);
  display.print(F("IR: L:"));
  display.print(irLeft == IR_CLEAR ? "OK" : "!!");
  display.print(F(" LC:"));
  display.print(irLeftCenter == IR_CLEAR ? "OK" : "!!");
  display.print(F(" RC:"));
  display.print(irRightCenter == IR_CLEAR ? "OK" : "!!");
  display.print(F(" R:"));
  display.print(irRight == IR_CLEAR ? "OK" : "!!");
  
  // Ultrasonic Distance
  display.setCursor(0, 26);
  display.print(F("US: "));
  display.print(usDistance);
  display.print(F(" cm"));
  
  // Priority & Action
  display.drawLine(0, 38, 128, 38, SSD1306_WHITE);
  display.setCursor(0, 42);
  display.print(F("Pri: "));
  display.println(sensorPriority);
  
  display.setCursor(0, 52);
  display.print(F("Act: "));
  display.println(currentAction);
  
  display.display();
}

// ============================================
// SERIAL STATUS PRINT
// ============================================
void printStatus() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSerialUpdate < SERIAL_UPDATE_INTERVAL) {
    return;
  }
  
  lastSerialUpdate = currentTime;
  
  Serial.print(F("Loop:")); Serial.print(loopCount);
  Serial.print(F(" | IR[L:")); Serial.print(irLeft == IR_CLEAR ? "✓" : "✗");
  Serial.print(F(" LC:")); Serial.print(irLeftCenter == IR_CLEAR ? "✓" : "✗");
  Serial.print(F(" RC:")); Serial.print(irRightCenter == IR_CLEAR ? "✓" : "✗");
  Serial.print(F(" R:")); Serial.print(irRight == IR_CLEAR ? "✓" : "✗");
  Serial.print(F("] US:")); Serial.print(usDistance);
  Serial.print(F("cm | ")); Serial.print(sensorPriority);
  Serial.print(F(" | ")); Serial.println(currentAction);
}

// ============================================
// SETUP
// ============================================
void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println(F("\n╔════════════════════════════════════════╗"));
  Serial.println(F("║   🔥 FUSION OBSTACLE AVOIDANCE        ║"));
  Serial.println(F("║   IR (Close) + Ultrasonic (Long)     ║"));
  Serial.println(F("╚════════════════════════════════════════╝\n"));
  
  Serial.println(F("📋 Sensor Strategy:"));
  Serial.println(F("  1. IR Priority - Immediate response (0-20cm)"));
  Serial.println(F("  2. US Danger - Emergency stop (20-40cm)"));
  Serial.println(F("  3. US Warning - Slow down (40-60cm)"));
  Serial.println(F("  4. US Scan - Plan ahead (60-100cm)"));
  Serial.println(F("  5. All Clear - Normal speed\n"));
  
  // Motor pins
  pinMode(LEFT1, OUTPUT);
  pinMode(LEFT2, OUTPUT);
  pinMode(LEFT_SPEED, OUTPUT);
  pinMode(RIGHT1, OUTPUT);
  pinMode(RIGHT2, OUTPUT);
  pinMode(RIGHT_SPEED, OUTPUT);
  
  // IR sensors
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_LEFT_CENTER, INPUT);
  pinMode(IR_RIGHT_CENTER, INPUT);
  pinMode(IR_RIGHT, INPUT);
  
  // Ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Servo
  scanner.attach(SERVO_PIN);
  scanner.write(90); // Center position
  
  // OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("⚠️ OLED init failed - continuing without display"));
  } else {
    Serial.println(F("✅ OLED initialized"));
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(20, 20);
    display.println(F("FUSION MODE"));
    display.setCursor(15, 35);
    display.println(F("Calibrating..."));
    display.display();
  }
  
  // Calibrate IR sensors
  calibrateIRSensors();
  
  // Test ultrasonic
  Serial.println(F("🔊 Testing Ultrasonic..."));
  int testDist = getUltrasonicDistance();
  Serial.print(F("  → Distance: "));
  Serial.print(testDist);
  Serial.println(F(" cm"));
  
  if (testDist < US_MAX_VALID_DISTANCE && testDist > 0) {
    Serial.println(F("✅ Ultrasonic working!\n"));
  } else {
    Serial.println(F("⚠️ Ultrasonic may have issues\n"));
  }
  
  currentAction = "READY";
  sensorPriority = "Standby";
  updateDisplay();
  
  Serial.println(F("✅ FUSION SYSTEM READY!"));
  Serial.println(F("🚦 Starting in 3 seconds...\n"));
  Serial.println(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━\n"));
  delay(3000);
  
  loopCount = 0;
  lastDisplayUpdate = millis();
  lastSerialUpdate = millis();
}

// ============================================
// MAIN LOOP
// ============================================
void loop() {
  fusionDecisionAlgorithm();
  updateDisplay();
  printStatus();
  delay(20); // Small delay for stability
}
