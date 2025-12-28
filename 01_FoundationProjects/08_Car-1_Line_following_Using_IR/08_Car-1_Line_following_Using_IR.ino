// ============================================
//     LINE FOLLOWER - CORRECT FOR YOUR ROBOT
//     IR LED ON (BLACK) = Sensor reads 1
//     IR LED OFF (AIR/WHITE) = Sensor reads 0
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
#define Left1 3
#define Left2 4
#define Left_Speed 5

#define Right1 8
#define Right2 7
#define Right_Speed 6

// ============================================
// IR SENSOR PINS
// ============================================
#define L_S A1    // Left Center Sensor
#define R_S A2    // Right Center Sensor

// ============================================
// SENSOR LOGIC - CORRECT!
// YOUR SENSORS: IR LED ON = 1, IR LED OFF = 0
// ============================================
#define BLACK 1    // Sensor reads 1 when on black line (IR LED ON)
#define WHITE 0    // Sensor reads 0 when off line (IR LED OFF)

// ============================================
// SPEED SETTINGS
// ============================================
#define MOTOR_SPEED 100      // Base speed
#define TURN_SPEED 100       // Turn speed
#define SCAN_DELAY 10        // Loop delay

// Display throttling
#define DISPLAY_UPDATE_INTERVAL 200

// ============================================
// GLOBAL VARIABLES
// ============================================
int leftSensor = 0;
int rightSensor = 0;

String currentAction = "Starting...";
unsigned long loopCounter = 0;
unsigned long lastDisplayUpdate = 0;

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
  
  // Header
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(5, 0);
  display.println(F("BLACK LINE FOLLOWER"));
  display.drawLine(0, 10, 128, 10, SSD1306_WHITE);
  
  // Sensor status with LED indicator
  display.setCursor(0, 14);
  display.print(F("L(A1): "));
  display.print(leftSensor == BLACK ? "BLACK(LED)" : "off");
  
  display.setCursor(0, 24);
  display.print(F("R(A2): "));
  display.print(rightSensor == BLACK ? "BLACK(LED)" : "off");
  
  // Visual bars - FILLED when BLACK (LED ON)
  int y = 36;
  int barWidth = 50;
  int spacing = 64;
  
  // Left bar - FILLED when on BLACK (LED ON)
  if (leftSensor == BLACK) {
    display.fillRect(5, y, barWidth, 12, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRect(5, y, barWidth, 12, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(12, y+2);
  display.print("LEFT");
  display.setTextColor(SSD1306_WHITE);
  
  // Right bar - FILLED when on BLACK (LED ON)
  if (rightSensor == BLACK) {
    display.fillRect(5 + spacing, y, barWidth, 12, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
  } else {
    display.drawRect(5 + spacing, y, barWidth, 12, SSD1306_WHITE);
    display.setTextColor(SSD1306_WHITE);
  }
  display.setCursor(12 + spacing, y+2);
  display.print("RIGHT");
  display.setTextColor(SSD1306_WHITE);
  
  // Action
  display.drawLine(0, 52, 128, 52, SSD1306_WHITE);
  display.setCursor(0, 56);
  display.println(currentAction);
  
  display.display();
}

// ============================================
// SENSOR READING
// ============================================

void readSensors() {
  leftSensor = digitalRead(L_S);    // A1
  rightSensor = digitalRead(R_S);   // A2
}

// ============================================
// MOTOR CONTROL
// ============================================

void forward() {
  digitalWrite(Left1, HIGH);
  digitalWrite(Left2, LOW);
  digitalWrite(Right1, HIGH);
  digitalWrite(Right2, LOW);
  analogWrite(Left_Speed, MOTOR_SPEED);
  analogWrite(Right_Speed, MOTOR_SPEED);
  currentAction = ">>> FORWARD >>>";
}

void turnLeft() {
  digitalWrite(Left1, LOW);
  digitalWrite(Left2, HIGH);
  digitalWrite(Right1, HIGH);
  digitalWrite(Right2, LOW);
  analogWrite(Left_Speed, TURN_SPEED);
  analogWrite(Right_Speed, TURN_SPEED);
  currentAction = "<<< TURN LEFT";
}

void turnRight() {
  digitalWrite(Left1, HIGH);
  digitalWrite(Left2, LOW);
  digitalWrite(Right1, LOW);
  digitalWrite(Right2, HIGH);
  analogWrite(Left_Speed, TURN_SPEED);
  analogWrite(Right_Speed, TURN_SPEED);
  currentAction = "TURN RIGHT >>>";
}

void stop() {
  digitalWrite(Left1, LOW);
  digitalWrite(Left2, LOW);
  digitalWrite(Right1, LOW);
  digitalWrite(Right2, LOW);
  analogWrite(Left_Speed, 0);
  analogWrite(Right_Speed, 0);
  currentAction = "STOP - OFF LINE";
}

// ============================================
// LINE FOLLOWING LOGIC - FINALLY CORRECT! ✅
// ============================================

void followLine() {
  readSensors();
  
  // Print sensor values for debugging
  Serial.print("L:");
  Serial.print(leftSensor);
  Serial.print(leftSensor == BLACK ? "(LED_ON)" : "(LED_OFF)");
  Serial.print(" R:");
  Serial.print(rightSensor);
  Serial.print(rightSensor == BLACK ? "(LED_ON)" : "(LED_OFF)");
  Serial.print(" | ");
  
  // ═══════════════════════════════════════════
  // CORRECT LOGIC: IR LED ON = 1 = BLACK
  // ═══════════════════════════════════════════
  
  // CASE 1: BOTH = 1 (Both IR LEDs ON) - Robot on black line
  // Action: GO FORWARD ✅
  if (leftSensor == BLACK && rightSensor == BLACK) {
    forward();
    Serial.println("FORWARD - Both on line!");
    loopCounter++;
    return;
  }
  
  // CASE 2: LEFT = 1, RIGHT = 0 (Left LED ON, Right OFF)
  // Line is to the LEFT
  // Action: TURN LEFT ✅
  if (leftSensor == BLACK && rightSensor == WHITE) {
    turnLeft();
    Serial.println("TURN LEFT - Line to left");
    loopCounter++;
    return;
  }
  
  // CASE 3: LEFT = 0, RIGHT = 1 (Right LED ON, Left OFF)
  // Line is to the RIGHT
  // Action: TURN RIGHT ✅
  if (leftSensor == WHITE && rightSensor == BLACK) {
    turnRight();
    Serial.println("TURN RIGHT - Line to right");
    loopCounter++;
    return;
  }
  
  // CASE 4: BOTH = 0 (Both IR LEDs OFF) - Lost line
  // Action: STOP ⚠️
  if (leftSensor == WHITE && rightSensor == WHITE) {
    stop();
    Serial.println("STOP - Line lost!");
    loopCounter++;
    return;
  }
  
  loopCounter++;
}

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(9600);
  delay(500);
  
  Serial.println(F("\n╔════════════════════════════════════════╗"));
  Serial.println(F("║   BLACK LINE FOLLOWER - FINAL FIX      ║"));
  Serial.println(F("╚════════════════════════════════════════╝"));
  Serial.println();
  Serial.println(F("YOUR SENSOR LOGIC:"));
  Serial.println(F("  • On BLACK line → IR LED ON → Reads 1"));
  Serial.println(F("  • In AIR/WHITE → IR LED OFF → Reads 0"));
  Serial.println();
  Serial.println(F("BEHAVIOR:"));
  Serial.println(F("  • Both = 1 (LEDs ON) → FORWARD"));
  Serial.println(F("  • Left = 1, Right = 0 → TURN LEFT"));
  Serial.println(F("  • Left = 0, Right = 1 → TURN RIGHT"));
  Serial.println(F("  • Both = 0 (LEDs OFF) → STOP"));
  Serial.println();
  
  // Motor pins
  pinMode(Left1, OUTPUT);
  pinMode(Left2, OUTPUT);
  pinMode(Left_Speed, OUTPUT);
  pinMode(Right1, OUTPUT);
  pinMode(Right2, OUTPUT);
  pinMode(Right_Speed, OUTPUT);
  
  // Sensor pins
  pinMode(L_S, INPUT);
  pinMode(R_S, INPUT);
  
  // Initialize OLED
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("❌ OLED init failed"));
  } else {
    Serial.println(F("✅ OLED ready"));
  }
  
  // Startup screen
  display.clearDisplay();
  display.setTextSize(1);
  display.setCursor(10, 10);
  display.println(F("LINE FOLLOWER"));
  display.setCursor(5, 25);
  display.println(F("LED ON = BLACK"));
  display.setCursor(10, 40);
  display.println(F("Starting..."));
  display.display();
  delay(2000);
  
  // Test sensors
  Serial.println(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
  Serial.println(F("SENSOR TEST:"));
  delay(1000);
  
  int testL = digitalRead(L_S);
  int testR = digitalRead(R_S);
  
  Serial.print(F("  L_S(A1): "));
  Serial.print(testL);
  Serial.print(F(" | R_S(A2): "));
  Serial.println(testR);
  Serial.println();
  
  Serial.println(F("Place robot on BLACK LINE to start!"));
  Serial.println(F("(Both sensors should read 1 when on line)"));
  Serial.println(F("━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━━"));
  Serial.println();
  
  loopCounter = 0;
  lastDisplayUpdate = millis();
}

// ============================================
// MAIN LOOP
// ============================================

void loop() {
  followLine();
  updateDisplay();
  delay(SCAN_DELAY);
}
