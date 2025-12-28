#include <Servo.h>

// ===== PIN DEFINITIONS =====
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

// ===== CONFIGURATION =====
#define SPEED_NORMAL 140        // Increased speed for better performance
#define SPEED_TURN 160          // Turn speed
#define OBSTACLE_DISTANCE 40    // Stop distance (cm)
#define SCAN_DISTANCE 50        // Start scanning distance (cm)
#define TURN_TIME 400           // Time to turn (ms)
#define BACKUP_TIME 300         // Time to reverse (ms)

// ===== GLOBAL VARIABLES =====
Servo scanner;
int rightDistance = 0;
int leftDistance = 0;
int middleDistance = 0;

// ===== ULTRASONIC SENSOR FUNCTION =====
int getDistance() {
  digitalWrite(TRIG_PIN, LOW);
  delayMicroseconds(2);
  digitalWrite(TRIG_PIN, HIGH);
  delayMicroseconds(10);
  digitalWrite(TRIG_PIN, LOW);
  
  long duration = pulseIn(ECHO_PIN, HIGH, 30000); // 30ms timeout
  
  // Validate reading
  if (duration == 0) {
    return 999; // No echo received - return max distance
  }
  
  int distance = duration * 0.034 / 2;
  
  // Filter out invalid readings
  if (distance > 400 || distance < 2) {
    return 999; // Invalid reading
  }
  
  return distance;
}

// ===== MOTOR CONTROL FUNCTIONS =====
void setMotorSpeed(int leftSpeed, int rightSpeed) {
  analogWrite(LEFT_SPEED, leftSpeed);
  analogWrite(RIGHT_SPEED, rightSpeed);
}

void forward() {
  digitalWrite(LEFT1, HIGH);
  digitalWrite(LEFT2, LOW);
  digitalWrite(RIGHT1, LOW);
  digitalWrite(RIGHT2, HIGH);
  setMotorSpeed(SPEED_NORMAL, SPEED_NORMAL);
}

void backward() {
  digitalWrite(LEFT1, LOW);
  digitalWrite(LEFT2, HIGH);
  digitalWrite(RIGHT1, HIGH);
  digitalWrite(RIGHT2, LOW);
  setMotorSpeed(SPEED_NORMAL, SPEED_NORMAL);
}

void turnRight() {
  // FIXED: Left motor forward, Right motor backward
  digitalWrite(LEFT1, HIGH);
  digitalWrite(LEFT2, LOW);
  digitalWrite(RIGHT1, HIGH);   // FIXED: Was LOW
  digitalWrite(RIGHT2, LOW);    // FIXED: Was HIGH
  setMotorSpeed(SPEED_TURN, SPEED_TURN);
}

void turnLeft() {
  // Left motor backward, Right motor forward
  digitalWrite(LEFT1, LOW);     // FIXED: Was HIGH
  digitalWrite(LEFT2, HIGH);    // FIXED: Was LOW
  digitalWrite(RIGHT1, LOW);
  digitalWrite(RIGHT2, HIGH);
  setMotorSpeed(SPEED_TURN, SPEED_TURN);
}

void stopMotors() {
  digitalWrite(LEFT1, LOW);
  digitalWrite(LEFT2, LOW);
  digitalWrite(RIGHT1, LOW);
  digitalWrite(RIGHT2, LOW);
  setMotorSpeed(0, 0);
}

// ===== SCANNING FUNCTION =====
void scanAndDecide() {
  Serial.println("\n🛑 OBSTACLE DETECTED! Scanning...");
  
  stopMotors();
  delay(200);
  
  // Backup a bit
  Serial.println("⏪ Backing up...");
  backward();
  delay(BACKUP_TIME);
  stopMotors();
  delay(200);
  
  // Scan Right (45°)
  Serial.print("👉 Scanning Right... ");
  scanner.write(45);
  delay(400); // Wait for servo to reach position
  rightDistance = getDistance();
  Serial.print(rightDistance);
  Serial.println(" cm");
  
  // Return to Center
  scanner.write(90);
  delay(300);
  
  // Scan Left (135°)
  Serial.print("👈 Scanning Left... ");
  scanner.write(135);
  delay(400); // Wait for servo to reach position
  leftDistance = getDistance();
  Serial.print(leftDistance);
  Serial.println(" cm");
  
  // Return to Center
  scanner.write(90);
  delay(300);
  
  // Decision Making
  Serial.println("\n📊 Analysis:");
  Serial.print("   Right: "); Serial.print(rightDistance); Serial.println(" cm");
  Serial.print("   Left:  "); Serial.print(leftDistance); Serial.println(" cm");
  
  // Both paths blocked
  if (rightDistance < OBSTACLE_DISTANCE && leftDistance < OBSTACLE_DISTANCE) {
    Serial.println("⚠️ Both paths blocked! Turning around...");
    turnRight();
    delay(TURN_TIME * 2); // 180° turn
    stopMotors();
  }
  // Right is clearer
  else if (rightDistance > leftDistance) {
    Serial.println("✅ Turning RIGHT (clearer path)");
    turnRight();
    delay(TURN_TIME);
    stopMotors();
  }
  // Left is clearer
  else {
    Serial.println("✅ Turning LEFT (clearer path)");
    turnLeft();
    delay(TURN_TIME);
    stopMotors();
  }
  
  delay(200);
  Serial.println("🚀 Resuming forward movement\n");
}

// ===== SETUP =====
void setup() {
  // Motor pins
  pinMode(LEFT1, OUTPUT);
  pinMode(LEFT2, OUTPUT);
  pinMode(LEFT_SPEED, OUTPUT);
  pinMode(RIGHT1, OUTPUT);
  pinMode(RIGHT2, OUTPUT);
  pinMode(RIGHT_SPEED, OUTPUT);
  
  // Ultrasonic sensor
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);
  
  // Servo
  scanner.attach(SERVO_PIN);
  scanner.write(90); // Center position
  
  Serial.begin(9600);
  
  Serial.println("\n╔════════════════════════════════════╗");
  Serial.println("║  🤖 OBSTACLE AVOIDANCE ROBOT     ║");
  Serial.println("║  Ultrasonic + Servo Scanner      ║");
  Serial.println("╚════════════════════════════════════╝\n");
  
  Serial.println("⚙️ Configuration:");
  Serial.print("   Speed: "); Serial.println(SPEED_NORMAL);
  Serial.print("   Obstacle Distance: "); Serial.print(OBSTACLE_DISTANCE); Serial.println(" cm");
  Serial.print("   Scan Distance: "); Serial.print(SCAN_DISTANCE); Serial.println(" cm");
  
  Serial.println("\n🚦 Starting in 3 seconds...\n");
  delay(3000);
}

// ===== MAIN LOOP =====
void loop() {
  // Get distance reading
  middleDistance = getDistance();
  
  // Print status
  Serial.print("Distance: ");
  Serial.print(middleDistance);
  Serial.print(" cm | Status: ");
  
  // Check for obstacle
  if (middleDistance <= SCAN_DISTANCE && middleDistance > 0) {
    
    if (middleDistance <= OBSTACLE_DISTANCE) {
      Serial.println("🚨 TOO CLOSE!");
      scanAndDecide();
    }
    else {
      Serial.println("⚠️ Obstacle ahead - slowing down");
      // Slow down when obstacle is near
      setMotorSpeed(SPEED_NORMAL - 40, SPEED_NORMAL - 40);
      forward();
      delay(50);
    }
    
  } else {
    Serial.println("✅ Clear - moving forward");
    forward();
    delay(100); // Small delay for stability
  }
}
