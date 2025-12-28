// ============================================
// WALL FOLLOWER - 23cm Car Safe Corners
// FIXED: First-time gap creation guaranteed
// ============================================

// Motor Pins
#define IN1 3
#define IN2 4
#define ENA 5
#define IN3 8
#define IN4 7
#define ENB 6

// IR Sensors
#define IR_FRONT_LEFT  A0
#define IR_FRONT_RIGHT A1
#define IR_LEFT        A2
#define IR_RIGHT       A3

// Calibrated baselines
int baselineFL = 600;
int baselineFR = 600;
int baselineL = 600;
int baselineR = 600;

// ========== THRESHOLDS ==========
int frontObstacle = 200;      
int wallPresent = 100;        
int wallSafeDistance = 60;    // REDUCED - Further from wall (was 65)
int wallTooClose = 95;        
int wallGone = 45;            

// ========== CAR DIMENSIONS ==========
#define CAR_LENGTH_CM 23
#define CLEARANCE_TIME 350

// ========== SPEEDS ==========
#define SPEED_NORMAL    65
#define SPEED_TURN      70
#define SPEED_GAP       55    // SLOWER for better control (was 60)

// Sensor readings
int sFL, sFR, sL, sR;

// State tracking
bool wallFound = false;
bool gapCreated = false;
int noWallCount = 0;
int gapAttempts = 0;
int turnNumber = 0;

void setup() {
  Serial.begin(9600);
  
  // Motor pins
  pinMode(IN1, OUTPUT);
  pinMode(IN2, OUTPUT);
  pinMode(ENA, OUTPUT);
  pinMode(IN3, OUTPUT);
  pinMode(IN4, OUTPUT);
  pinMode(ENB, OUTPUT);
  
  stopMotors();
  
  printBanner();
  
  Serial.println(F("⏳ Calibrating sensors (3 sec)..."));
  delay(1000);
  
  calibrateSensors();
  
  Serial.println(F("\n✅ Calibration Complete!"));
  Serial.print(F("  FL=")); Serial.print(baselineFL);
  Serial.print(F(" FR=")); Serial.print(baselineFR);
  Serial.print(F(" L=")); Serial.print(baselineL);
  Serial.print(F(" R=")); Serial.println(baselineR);
  
  Serial.println(F("\n📏 Thresholds:"));
  Serial.print(F("  Wall Present: ")); Serial.println(wallPresent);
  Serial.print(F("  Safe Distance: ")); Serial.println(wallSafeDistance);
  Serial.print(F("  Wall Gone: ")); Serial.println(wallGone);
  
  Serial.println(F("\n🚗 Starting in 3 seconds...\n"));
  delay(3000);
}

void loop() {
  readSensors();
  
  int flDiff = baselineFL - sFL;
  int frDiff = baselineFR - sFR;
  int rDiff = baselineR - sR;
  
  // Print sensor values
  Serial.print(F("FL:"));
  Serial.print(flDiff);
  Serial.print(F(" FR:"));
  Serial.print(frDiff);
  Serial.print(F(" R:"));
  Serial.print(rDiff);
  Serial.print(F(" | "));
  
  // ========== EMERGENCY STOP ==========
  if (flDiff > frontObstacle || frDiff > frontObstacle) {
    Serial.println(F("🛑 FRONT BLOCKED - Reversing"));
    stopMotors();
    delay(300);
    
    int reverseTime = 0;
    while (reverseTime < 50) {
      readSensors();
      flDiff = baselineFL - sFL;
      frDiff = baselineFR - sFR;
      
      if (flDiff < 100 && frDiff < 100) {
        stopMotors();
        delay(300);
        Serial.println(F("✅ Clear"));
        break;
      }
      
      reverse();
      reverseTime++;
      delay(50);
    }
    
    return;
  }
  
  // ========== STEP 1: APPROACH WALL ==========
  if (!wallFound) {
    if (rDiff > wallPresent) {
      Serial.print(F("✅ WALL FOUND! R="));
      Serial.println(rDiff);
      Serial.println(F("   Stopping to prepare gap creation..."));
      
      stopMotors();
      delay(500);  // Give time to stabilize
      
      wallFound = true;
      gapCreated = false;  // IMPORTANT: Ensure gap creation will happen
      gapAttempts = 0;
      
      Serial.println(F("   Ready to create safe gap!\n"));
      
    } else {
      Serial.println(F("➡️ Searching..."));
      approachRight();
    }
    delay(50);
    return;
  }
  
  // ========== STEP 2: CREATE SAFE GAP (FIXED!) ==========
  if (wallFound && !gapCreated) {
    
    // Re-read sensors for accurate gap creation
    readSensors();
    rDiff = baselineR - sR;
    
    Serial.print(F("⬅️ GAP CREATION: R="));
    Serial.print(rDiff);
    Serial.print(F(" → Target="));
    Serial.print(wallSafeDistance);
    Serial.print(F(" | Attempt #"));
    Serial.print(gapAttempts + 1);
    Serial.print(F(" | "));
    
    // Calculate progress
    int progress = 0;
    if (rDiff > wallSafeDistance) {
      // Still need to move away
      progress = map(rDiff, wallPresent, wallSafeDistance, 0, 100);
      if (progress > 100) progress = 0;  // Haven't started yet
      if (progress < 0) progress = 0;
    } else {
      progress = 100;  // Already at target
    }
    
    Serial.print(F("Progress: "));
    Serial.print(progress);
    Serial.println(F("%"));
    
    // Check if gap achieved
    if (rDiff <= wallSafeDistance) {
      Serial.println(F("\n╔════════════════════════════════╗"));
      Serial.println(F("║  ✅ SAFE GAP CREATED!         ║"));
      Serial.println(F("╚════════════════════════════════╝"));
      Serial.print(F("   Initial R: "));
      Serial.print(wallPresent);
      Serial.print(F(" → Final R: "));
      Serial.println(rDiff);
      Serial.print(F("   Gap creation took "));
      Serial.print(gapAttempts);
      Serial.println(F(" attempts\n"));
      
      stopMotors();
      delay(1000);  // Longer pause to confirm
      gapCreated = true;
      
      Serial.println(F("🚀 Starting wall following mode!\n"));
      
    } else if (gapAttempts >= 60) {
      // Timeout protection
      Serial.println(F("\n⏱️ GAP CREATION TIMEOUT!"));
      Serial.print(F("   After 60 attempts, R still at: "));
      Serial.println(rDiff);
      Serial.println(F("   Proceeding anyway...\n"));
      
      stopMotors();
      delay(500);
      gapCreated = true;
      
    } else {
      // Keep creating gap - STRONGER steering
      Serial.println(F("   → Steering LEFT (away from wall)"));
      
      moveLeftStrong();  // Use stronger function
      gapAttempts++;
    }
    
    delay(100);  // Slower loop for precise control
    return;
  }
  
  // ========== STEP 3: FOLLOW WALL ==========
  
  if (rDiff < wallGone) {
    noWallCount++;
    Serial.print(F("🔍 Wall ending ["));
    Serial.print(noWallCount);
    Serial.println(F("/5]"));
    
    if (noWallCount >= 5) {
      turnNumber++;
      
      Serial.println(F("\n╔══════════════════════════════════╗"));
      Serial.print(F("║  CORNER #"));
      Serial.print(turnNumber);
      Serial.println(F(" DETECTED!          ║"));
      Serial.println(F("╚══════════════════════════════════╝"));
      Serial.print(F("🚗 Clearing "));
      Serial.print(CAR_LENGTH_CM);
      Serial.println(F("cm car body..."));
      
      stopMotors();
      delay(200);
      
      // ===== DRIVE FORWARD TO CLEAR CAR BODY =====
      unsigned long clearStart = millis();
      Serial.print(F("⏩ Forward "));
      Serial.print(CLEARANCE_TIME);
      Serial.print(F("ms... "));
      
      int progressDots = 0;
      while (millis() - clearStart < CLEARANCE_TIME) {
        readSensors();
        flDiff = baselineFL - sFL;
        frDiff = baselineFR - sFR;
        
        if (flDiff > frontObstacle || frDiff > frontObstacle) {
          Serial.println(F("\n⚠️ Front blocked!"));
          stopMotors();
          break;
        }
        
        goStraight();
        
        if ((millis() - clearStart) % 70 == 0 && progressDots < 5) {
          Serial.print(F("."));
          progressDots++;
        }
        
        delay(20);
      }
      
      stopMotors();
      delay(300);
      Serial.println(F(" ✅"));
      Serial.println(F("✅ Body cleared! Turning right..."));
      
      // ===== TURN RIGHT =====
      unsigned long turnStart = millis();
      bool newWallFound = false;
      
      while (millis() - turnStart < 1800) {
        readSensors();
        rDiff = baselineR - sR;
        
        if ((millis() - turnStart) % 200 == 0) {
          Serial.print(F("  Turning... R="));
          Serial.print(rDiff);
          Serial.print(F(" ["));
          Serial.print(millis() - turnStart);
          Serial.println(F("ms]"));
        }
        
        if (rDiff > wallPresent && (millis() - turnStart > 600)) {
          Serial.print(F("✅ NEW WALL! R="));
          Serial.println(rDiff);
          stopMotors();
          delay(500);
          noWallCount = 0;
          gapCreated = false;  // RESET for new wall
          gapAttempts = 0;
          newWallFound = true;
          Serial.println(F("🔄 Will create gap for new wall...\n"));
          break;
        }
        
        turnRight();
        delay(50);
      }
      
      if (!newWallFound) {
        Serial.println(F("⚠️ Turn timeout"));
      }
      
      stopMotors();
      delay(200);
      noWallCount = 0;
    }
    
  } else {
    noWallCount = 0;
    
    if (rDiff > wallTooClose) {
      Serial.println(F("⬅️ Too close"));
      steerLeft();
      
    } else if (rDiff < wallSafeDistance - 15) {
      Serial.println(F("➡️ Too far"));
      steerRight();
      
    } else {
      Serial.println(F("✅ Following"));
      goStraight();
    }
  }
  
  delay(50);
}

// ========== PRINT BANNER ==========
void printBanner() {
  Serial.println(F("\n╔══════════════════════════════════════╗"));
  Serial.println(F("║  WALL FOLLOWER - 23cm Car v10       ║"));
  Serial.println(F("║  FIXED: Gap Creation Guaranteed     ║"));
  Serial.println(F("╚══════════════════════════════════════╝"));
  Serial.print(F("\n🚗 Car Length: "));
  Serial.print(CAR_LENGTH_CM);
  Serial.println(F(" cm"));
  Serial.print(F("⏱️  Clearance Time: "));
  Serial.print(CLEARANCE_TIME);
  Serial.println(F(" ms\n"));
}

// ========== CALIBRATION ==========
void calibrateSensors() {
  long sumFL = 0, sumFR = 0, sumL = 0, sumR = 0;
  int samples = 30;
  
  for (int i = 0; i < samples; i++) {
    sumFL += analogRead(IR_FRONT_LEFT);
    sumFR += analogRead(IR_FRONT_RIGHT);
    sumL += analogRead(IR_LEFT);
    sumR += analogRead(IR_RIGHT);
    
    if (i % 3 == 0) Serial.print(F("."));
    delay(100);
  }
  
  baselineFL = sumFL / samples;
  baselineFR = sumFR / samples;
  baselineL = sumL / samples;
  baselineR = sumR / samples;
  
  sFL = baselineFL;
  sFR = baselineFR;
  sL = baselineL;
  sR = baselineR;
}

// ========== SENSOR READING ==========
void readSensors() {
  sFL = analogRead(IR_FRONT_LEFT);
  sFR = analogRead(IR_FRONT_RIGHT);
  sL = analogRead(IR_LEFT);
  sR = analogRead(IR_RIGHT);
}

// ========== MOTOR FUNCTIONS ==========

void approachRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED_NORMAL + 15);
  analogWrite(ENB, SPEED_NORMAL - 10);
}

void moveLeftGentle() {
  // Gentle left turn (original)
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED_GAP - 20);
  analogWrite(ENB, SPEED_GAP + 20);
}

void moveLeftStrong() {
  // STRONGER left turn for gap creation
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED_GAP - 30);  // Left wheel SLOWER (was -20)
  analogWrite(ENB, SPEED_GAP + 35);  // Right wheel FASTER (was +20)
}

void goStraight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED_NORMAL);
  analogWrite(ENB, SPEED_NORMAL);
}

void steerLeft() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED_NORMAL - 15);
  analogWrite(ENB, SPEED_NORMAL + 15);
}

void steerRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED_NORMAL + 15);
  analogWrite(ENB, SPEED_NORMAL - 15);
}

void turnRight() {
  digitalWrite(IN1, HIGH);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, HIGH);
  analogWrite(ENA, SPEED_TURN + 30);
  analogWrite(ENB, SPEED_TURN - 30);
}

void reverse() {
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, HIGH);
  digitalWrite(IN3, HIGH);
  digitalWrite(IN4, LOW);
  analogWrite(ENA, 60);
  analogWrite(ENB, 60);
}

void stopMotors() {
  analogWrite(ENA, 0);
  analogWrite(ENB, 0);
  digitalWrite(IN1, LOW);
  digitalWrite(IN2, LOW);
  digitalWrite(IN3, LOW);
  digitalWrite(IN4, LOW);
}