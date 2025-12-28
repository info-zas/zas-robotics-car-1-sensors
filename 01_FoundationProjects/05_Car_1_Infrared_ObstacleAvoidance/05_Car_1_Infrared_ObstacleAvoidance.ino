// ============================================
// IR OBSTACLE AVOIDANCE - SLOW & CAREFUL MODE
// Prioritizes obstacle detection over speed
// ============================================

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
// MOTOR PINS
// ============================================
#define IN1 8
#define IN2 7
#define ENA 6
#define IN3 4
#define IN4 3
#define ENB 5

// ============================================
// IR SENSOR PINS
// ============================================
#define IR_LEFT         A0
#define IR_LEFT_CENTER  A1
#define IR_RIGHT_CENTER A2
#define IR_RIGHT        A3

// ============================================
// SENSOR LOGIC
// ============================================
int OBSTACLE = 0;
int CLEAR = 1;

// ============================================
// 🐢 SLOW & CAREFUL SETTINGS
// ============================================
#define MOTOR_SPEED 100           // Slow, controlled speed
#define TURN_SPEED 110            // Precise turning
#define SHARP_TURN_DELAY 400      // Complete 90° turns
#define SLIGHT_TURN_DELAY 200     // Careful adjustments
#define BACKUP_DELAY 350          // Safe backup distance
#define SCAN_DELAY 15             // Stable scanning
#define VERIFICATION_DELAY 200    // Check after every turn

// Display & Serial throttling
#define DISPLAY_UPDATE_INTERVAL 300
#define SERIAL_UPDATE_INTERVAL 500

// Obstacle detection - IMMEDIATE RESPONSE
#define CONFIDENCE_THRESHOLD 1    // React on first detection
#define STUCK_TIMEOUT 3000        // Patient 3-second timeout

// ============================================
// GLOBAL VARIABLES
// ============================================
int sensorLeft = 0;
int sensorLeftCenter = 0;
int sensorRightCenter = 0;
int sensorRight = 0;

int prevLeft = 0;
int prevLC = 0;
int prevRC = 0;
int prevRight = 0;

String currentAction = "Initializing...";
unsigned long loopCounter = 0;

unsigned long lastDisplayUpdate = 0;
unsigned long lastSerialUpdate = 0;
unsigned long lastMovementTime = 0;

int obstacleConfidence = 0;

// ============================================
// AUTO-CALIBRATION
// ============================================

void autoCalibrateSensors() {
  Serial.println(F("\n╔════════════════════════════════════════╗"));
  Serial.println(F("║   SLOW & CAREFUL MODE - CALIBRATION    ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  
  Serial.println(F("→ Place robot in OPEN SPACE"));
  Serial.println(F("→ Calibrating in 2 seconds..."));
  
  delay(2000);
  
  int openLeft = digitalRead(IR_LEFT);
  int openLC = digitalRead(IR_LEFT_CENTER);
  int openRC = digitalRead(IR_RIGHT_CENTER);
  int openRight = digitalRead(IR_RIGHT);
  
  int clearCount = openLeft + openLC + openRC + openRight;
  
  if (clearCount >= 2) {
    CLEAR = 1;
    OBSTACLE = 0;
    Serial.println(F("✅ ACTIVE LOW (CLEAR=1, OBSTACLE=0)"));
  } else {
    CLEAR = 0;
    OBSTACLE = 1;
    Serial.println(F("✅ ACTIVE HIGH (CLEAR=0, OBSTACLE=1)"));
  }
  
  Serial.println(F("✅ Calibration Complete!"));
  Serial.println(F("🐢 SLOW & CAREFUL MODE ACTIVATED"));
  Serial.println();
  delay(1000);
}

// ============================================
// OLED DISPLAY UPDATE
// ============================================

void updateDisplayFast() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastDisplayUpdate < DISPLAY_UPDATE_INTERVAL) {
    return;
  }
  
  lastDisplayUpdate = currentTime;
  
  display.clearDisplay();
  
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(0, 0);
  display.println(F("CAREFUL MODE"));
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // Sensor status
  display.setCursor(0, 14);
  display.print(F("L:"));
  display.print(sensorLeft == CLEAR ? "OK" : "!!");
  display.print(F(" LC:"));
  display.print(sensorLeftCenter == CLEAR ? "OK" : "!!");
  display.print(F(" RC:"));
  display.print(sensorRightCenter == CLEAR ? "OK" : "!!");
  display.print(F(" R:"));
  display.print(sensorRight == CLEAR ? "OK" : "!!");
  
  // Visual bars
  int y = 28;
  int barWidth = 20;
  int spacing = 26;
  
  // Left
  if (sensorLeft == CLEAR) {
    display.fillRect(5, y, barWidth, 15, SSD1306_WHITE);
  } else {
    display.drawRect(5, y, barWidth, 15, SSD1306_WHITE);
  }
  display.setCursor(10, y+4);
  display.setTextColor(sensorLeft == CLEAR ? SSD1306_BLACK : SSD1306_WHITE);
  display.print("L");
  display.setTextColor(SSD1306_WHITE);
  
  // LC
  if (sensorLeftCenter == CLEAR) {
    display.fillRect(5 + spacing, y, barWidth, 15, SSD1306_WHITE);
  } else {
    display.drawRect(5 + spacing, y, barWidth, 15, SSD1306_WHITE);
  }
  display.setCursor(8 + spacing, y+4);
  display.setTextColor(sensorLeftCenter == CLEAR ? SSD1306_BLACK : SSD1306_WHITE);
  display.print("LC");
  display.setTextColor(SSD1306_WHITE);
  
  // RC
  if (sensorRightCenter == CLEAR) {
    display.fillRect(5 + spacing*2, y, barWidth, 15, SSD1306_WHITE);
  } else {
    display.drawRect(5 + spacing*2, y, barWidth, 15, SSD1306_WHITE);
  }
  display.setCursor(8 + spacing*2, y+4);
  display.setTextColor(sensorRightCenter == CLEAR ? SSD1306_BLACK : SSD1306_WHITE);
  display.print("RC");
  display.setTextColor(SSD1306_WHITE);
  
  // Right
  if (sensorRight == CLEAR) {
    display.fillRect(5 + spacing*3, y, barWidth, 15, SSD1306_WHITE);
  } else {
    display.drawRect(5 + spacing*3, y, barWidth, 15, SSD1306_WHITE);
  }
  display.setCursor(10 + spacing*3, y+4);
  display.setTextColor(sensorRight == CLEAR ? SSD1306_BLACK : SSD1306_WHITE);
  display.print("R");
  display.setTextColor(SSD1306_WHITE);
  
  // Action
  display.drawLine(0, 48, 128, 48, SSD1306_WHITE);
  display.setCursor(0, 52);
  display.print(F("Act: "));
  display.println(currentAction);
  
  display.display();
}

// ============================================
// SERIAL STATUS PRINT
// ============================================

void printStatusFast() {
  unsigned long currentTime = millis();
  
  if (currentTime - lastSerialUpdate < SERIAL_UPDATE_INTERVAL) {
    return;
  }
  
  lastSerialUpdate = currentTime;
  
  Serial.print(F("Loop:"));
  Serial.print(loopCounter);
  Serial.print(F(" | L:"));
  Serial.print(sensorLeft == CLEAR ? "✓" : "✗");
  Serial.print(F(" LC:"));
  Serial.print(sensorLeftCenter == CLEAR ? "✓" : "✗");
  Serial.print(F(" RC:"));
  Serial.print(sensorRightCenter == CLEAR ? "✓" : "✗");
  Serial.print(F(" R:"));
  Serial.print(sensorRight == CLEAR ? "✓" : "✗");
  Serial.print(F(" | Action: "));
  Serial.println(currentAction);
}

// ============================================
// SENSOR READING
// ============================================

inline void readSensorsFast() {
  prevLeft = sensorLeft;
  prevLC = sensorLeftCenter;
  prevRC = sensorRightCenter;
  prevRight = sensorRight;
  
  sensorLeft = digitalRead(IR_LEFT);
  sensorLeftCenter = digitalRead(IR_LEFT_CENTER);
  sensorRightCenter = digitalRead(IR_RIGHT_CENTER);
  sensorRight = digitalRead(IR_RIGHT);
}

// ============================================
// PATH ANALYSIS
// ============================================

inline bool isCenterClear() {
  return (sensorLeftCenter == CLEAR && sensorRightCenter == CLEAR);
}

inline bool isAllClear() {
  return (sensorLeft == CLEAR && sensorLeftCenter == CLEAR && 
          sensorRightCenter == CLEAR && sensorRight == CLEAR);
}

inline bool isCenterBlocked() {
  return (sensorLeftCenter == OBSTACLE && sensorRightCenter == OBSTACLE);
}

inline bool isLeftSideBlocked() {
  return (sensorLeft == OBSTACLE || sensorLeftCenter == OBSTACLE);
}

inline bool isRightSideBlocked() {
  return (sensorRight == OBSTACLE || sensorRightCenter == OBSTACLE);
}

// ============================================
// MOTOR CONTROL
// ============================================

void setMotors(int in1, int in2, int in3, int in4, int speed) {
  digitalWrite(IN1, in1);
  digitalWrite(IN2, in2);
  digitalWrite(IN3, in3);
  digitalWrite(IN4, in4);
  analogWrite(ENA, speed);
  analogWrite(ENB, speed);
}

inline void stop() {
  setMotors(LOW, LOW, LOW, LOW, 0);
  currentAction = "STOP";
}

inline void forward(int speed) {
  setMotors(LOW, HIGH, LOW, HIGH, speed);
  currentAction = "FORWARD (Slow)";
}

inline void backward(int speed) {
  setMotors(HIGH, LOW, HIGH, LOW, speed);
  currentAction = "BACKWARD";
}

inline void turnLeft(int speed) {
  setMotors(LOW, HIGH, HIGH, LOW, speed);
  currentAction = "TURN LEFT";
}

inline void turnRight(int speed) {
  setMotors(HIGH, LOW, LOW, HIGH, speed);
  currentAction = "TURN RIGHT";
}

// ============================================
// VERIFY PATH AFTER TURN
// ============================================

bool verifyPathClear() {
  delay(VERIFICATION_DELAY);  // Wait for robot to settle
  readSensorsFast();
  updateDisplayFast();
  
  Serial.print(F("   → Verifying: L:"));
  Serial.print(sensorLeft == CLEAR ? "✓" : "✗");
  Serial.print(F(" LC:"));
  Serial.print(sensorLeftCenter == CLEAR ? "✓" : "✗");
  Serial.print(F(" RC:"));
  Serial.print(sensorRightCenter == CLEAR ? "✓" : "✗");
  Serial.print(F(" R:"));
  Serial.println(sensorRight == CLEAR ? "✓" : "✗");
  
  // Path is clear if center sensors are clear
  return isCenterClear();
}

// ============================================
// WATCHDOG - STUCK DETECTION
// ============================================

void checkIfStuck() {
  if (!isAllClear() && !isCenterClear()) {
    unsigned long currentTime = millis();
    
    if (currentTime - lastMovementTime > STUCK_TIMEOUT) {
      Serial.println(F("⚠️ STUCK DETECTED - CAREFUL ESCAPE!"));
      
      // Gentle backup
      stop();
      delay(100);
      
      backward(MOTOR_SPEED);
      delay(BACKUP_DELAY * 2);
      
      stop();
      delay(200);
      
      // Try turning right first
      turnRight(TURN_SPEED);
      delay(SHARP_TURN_DELAY * 2);
      
      stop();
      delay(200);
      
      // Verify escape worked
      if (!verifyPathClear()) {
        // Still blocked, try opposite direction
        Serial.println(F("   → Still blocked, trying left..."));
        turnLeft(TURN_SPEED);
        delay(SHARP_TURN_DELAY * 3);
        stop();
        delay(200);
      }
      
      lastMovementTime = millis();
      obstacleConfidence = 0;
    }
  } else {
    lastMovementTime = millis();
  }
}

// ============================================
// 🎯 SLOW & CAREFUL DECISION LOGIC
// ============================================

void makeDecisionCareful() {
  readSensorsFast();
  
  // ═══════════════════════════════════════════
  // PRIORITY 1: ALL CLEAR - Slow forward movement
  // ═══════════════════════════════════════════
  if (isAllClear()) {
    forward(MOTOR_SPEED);
    obstacleConfidence = 0;
    loopCounter++;
    return;
  }
  
  // ═══════════════════════════════════════════
  // PRIORITY 2: LEFT SIDE BLOCKED - Turn RIGHT and verify
  // ═══════════════════════════════════════════
  if (isLeftSideBlocked() && !isRightSideBlocked()) {
    Serial.println(F("🚨 LEFT OBSTACLE DETECTED!"));
    
    // Immediate stop
    stop();
    updateDisplayFast();
    delay(100);
    
    // Small backup for safety
    backward(MOTOR_SPEED);
    delay(BACKUP_DELAY);
    
    stop();
    delay(100);
    
    // Turn right
    Serial.println(F("   → Turning RIGHT..."));
    turnRight(TURN_SPEED);
    delay(SHARP_TURN_DELAY);
    
    stop();
    delay(100);
    
    // Verify path is clear
    Serial.println(F("   → Verifying path..."));
    if (verifyPathClear()) {
      Serial.println(F("   ✅ Path clear, continuing"));
      forward(MOTOR_SPEED);
    } else {
      Serial.println(F("   ⚠️ Still blocked, turning more..."));
      turnRight(TURN_SPEED);
      delay(SHARP_TURN_DELAY);
      stop();
      delay(100);
    }
    
    obstacleConfidence = 0;
    loopCounter++;
    return;
  }
  
  // ═══════════════════════════════════════════
  // PRIORITY 3: RIGHT SIDE BLOCKED - Turn LEFT and verify
  // ═══════════════════════════════════════════
  if (isRightSideBlocked() && !isLeftSideBlocked()) {
    Serial.println(F("🚨 RIGHT OBSTACLE DETECTED!"));
    
    // Immediate stop
    stop();
    updateDisplayFast();
    delay(100);
    
    // Small backup for safety
    backward(MOTOR_SPEED);
    delay(BACKUP_DELAY);
    
    stop();
    delay(100);
    
    // Turn left
    Serial.println(F("   → Turning LEFT..."));
    turnLeft(TURN_SPEED);
    delay(SHARP_TURN_DELAY);
    
    stop();
    delay(100);
    
    // Verify path is clear
    Serial.println(F("   → Verifying path..."));
    if (verifyPathClear()) {
      Serial.println(F("   ✅ Path clear, continuing"));
      forward(MOTOR_SPEED);
    } else {
      Serial.println(F("   ⚠️ Still blocked, turning more..."));
      turnLeft(TURN_SPEED);
      delay(SHARP_TURN_DELAY);
      stop();
      delay(100);
    }
    
    obstacleConfidence = 0;
    loopCounter++;
    return;
  }
  
  // ═══════════════════════════════════════════
  // PRIORITY 4: BOTH SIDES BLOCKED - Careful 180° turn
  // ═══════════════════════════════════════════
  if (isLeftSideBlocked() && isRightSideBlocked()) {
    Serial.println(F("🚨 BOTH SIDES BLOCKED!"));
    
    // Stop immediately
    stop();
    updateDisplayFast();
    delay(200);
    
    // Longer backup
    Serial.println(F("   → Backing up..."));
    backward(MOTOR_SPEED);
    delay(BACKUP_DELAY * 2);
    
    stop();
    delay(200);
    
    // 180° turn (always right)
    Serial.println(F("   → Performing 180° turn..."));
    turnRight(TURN_SPEED);
    delay(SHARP_TURN_DELAY * 2);
    
    stop();
    delay(200);
    
    // Verify
    Serial.println(F("   → Verifying path..."));
    if (verifyPathClear()) {
      Serial.println(F("   ✅ Escaped successfully"));
    } else {
      Serial.println(F("   ⚠️ Still blocked, backing more..."));
      backward(MOTOR_SPEED);
      delay(BACKUP_DELAY * 2);
      stop();
      delay(200);
    }
    
    obstacleConfidence = 0;
    loopCounter++;
    return;
  }
  
  // ═══════════════════════════════════════════
  // PRIORITY 5: CENTER CLEAR - Slow forward
  // ═══════════════════════════════════════════
  if (isCenterClear()) {
    forward(MOTOR_SPEED);
    obstacleConfidence = 0;
    loopCounter++;
    return;
  }
  
  // ═══════════════════════════════════════════
  // DEFAULT: Stop and reassess
  // ═══════════════════════════════════════════
  stop();
  delay(100);
  loopCounter++;
}

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  delay(500);
  
  Serial.println(F("\n╔════════════════════════════════════════╗"));
  Serial.println(F("║   IR OBSTACLE AVOIDANCE                ║"));
  Serial.println(F("║   🐢 SLOW & CAREFUL MODE               ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("Features:"));
  Serial.println(F("  • Slow speed (100 PWM)"));
  Serial.println(F("  • Immediate obstacle response"));
  Serial.println(F("  • Verify after every turn"));
  Serial.println(F("  • Speed bump friendly"));
  Serial.println();
  
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_LEFT_CENTER, INPUT);
  pinMode(IR_RIGHT_CENTER, INPUT);
  pinMode(IR_RIGHT, INPUT);
  
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("❌ OLED initialization failed"));
  } else {
    Serial.println(F("✅ OLED initialized"));
    
    display.clearDisplay();
    display.setTextSize(1);
    display.setCursor(15, 20);
    display.println(F("CAREFUL MODE"));
    display.setCursor(20, 35);
    display.println(F("Calibrating..."));
    display.display();
  }
  
  autoCalibrateSensors();
  
  currentAction = "READY";
  updateDisplayFast();
  delay(1000);
  
  Serial.println(F("✅ System Ready - SLOW & CAREFUL MODE!"));
  Serial.println(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
  Serial.println();
  
  loopCounter = 0;
  lastDisplayUpdate = millis();
  lastSerialUpdate = millis();
  lastMovementTime = millis();
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  makeDecisionCareful();
  checkIfStuck();
  updateDisplayFast();
  printStatusFast();
  delay(SCAN_DELAY);
}