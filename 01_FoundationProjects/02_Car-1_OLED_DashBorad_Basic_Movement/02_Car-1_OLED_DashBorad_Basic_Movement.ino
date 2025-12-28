// ============================================
//     ROBOT MOTION CONTROL WITH OLED
//     Enhanced Visual Display
// ============================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ----------- OLED Configuration -------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ----------- Motor Pins (YOUR SETUP) --------
#define Left1 7
#define Left2 8
#define Left_Speed 6

#define Right1 3
#define Right2 4
#define Right_Speed 5

// ----------- Motor Settings -----------------
#define MOTOR_SPEED 100
#define TURN_90_TIME 680

// ----------- Motion Durations ---------------
#define FORWARD_TIME 2000
#define BACKWARD_TIME 2000
#define LEFT_TIME 2000
#define RIGHT_TIME 2000
#define PAUSE_TIME 1500

void setup() {
  Serial.begin(9600);
  
  // Initialize motor pins
  pinMode(Left1, OUTPUT);
  pinMode(Left2, OUTPUT);
  pinMode(Left_Speed, OUTPUT);
  pinMode(Right1, OUTPUT);
  pinMode(Right2, OUTPUT);
  pinMode(Right_Speed, OUTPUT);

  analogWrite(Left_Speed, MOTOR_SPEED);
  analogWrite(Right_Speed, MOTOR_SPEED);

  // Initialize OLED
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("OLED init failed!"));
    while (1);
  }

  // Startup screen
  showStartup();
  delay(2000);

  Serial.println("Starting motion sequence...");
  delay(1000);

  // 1. Forward
  showAction("FORWARD", "Moving ahead", 1);
  forward();
  delay(FORWARD_TIME);
  Stop();
  showAction("STOP", "Paused", 0);
  delay(PAUSE_TIME);

  // 2. Backward
  showAction("BACKWARD", "Reversing", 2);
  backward();
  delay(BACKWARD_TIME);
  Stop();
  showAction("STOP", "Paused", 0);
  delay(PAUSE_TIME);

  // 3. Left Turn (continuous)
  showAction("LEFT", "Turning left", 3);
  turnLeft();
  delay(LEFT_TIME);
  Stop();
  showAction("STOP", "Paused", 0);
  delay(PAUSE_TIME);

  // 4. Right Turn (continuous)
  showAction("RIGHT", "Turning right", 4);
  turnRight();
  delay(RIGHT_TIME);
  Stop();
  showAction("STOP", "Paused", 0);
  delay(PAUSE_TIME);

  // Complete
  showComplete();
  Serial.println("Motion sequence complete!");
}

void loop() {}

// ============================================
//       ENHANCED DISPLAY FUNCTIONS
// ============================================

void showStartup() {
  display.clearDisplay();
  
  // Animated border
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawRect(2, 2, 124, 60, SSD1306_WHITE);
  
  // Title
  display.setTextSize(2);
  display.setTextColor(SSD1306_WHITE);
  display.setCursor(18, 12);
  display.println("ROBOT");
  
  display.setCursor(12, 32);
  display.println("MOTION");
  
  // Version info
  display.setTextSize(1);
  display.setCursor(28, 52);
  display.println("v1.0 Ready");
  
  display.display();
}

void showAction(String action, String status, int actionType) {
  display.clearDisplay();
  
  // Draw different styles based on action type
  switch(actionType) {
    case 0: // STOP - Simple display
      drawStopScreen(action, status);
      break;
    case 1: // FORWARD - Up arrow
      drawForwardScreen(action, status);
      break;
    case 2: // BACKWARD - Down arrow
      drawBackwardScreen(action, status);
      break;
    case 3: // LEFT - Left arrow
      drawLeftScreen(action, status);
      break;
    case 4: // RIGHT - Right arrow
      drawRightScreen(action, status);
      break;
    case 5: // LEFT 90 - Sharp left
      drawLeft90Screen(action, status);
      break;
    case 6: // RIGHT 90 - Sharp right
      drawRight90Screen(action, status);
      break;
  }
  
  display.display();
  
  Serial.print("Action: ");
  Serial.print(action);
  Serial.print(" | ");
  Serial.println(status);
}

// --- STOP Screen ---
void drawStopScreen(String action, String status) {
  // Double border for STOP
  display.drawRect(0, 0, 128, 64, SSD1306_WHITE);
  display.drawRect(3, 3, 122, 58, SSD1306_WHITE);
  
  // STOP icon (octagon)
  display.fillCircle(64, 28, 15, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(2);
  display.setCursor(48, 22);
  display.println("STOP");
  
  // Status
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(0, 54);
  display.print(">>> ");
  display.println(status);
}

// --- FORWARD Screen ---
void drawForwardScreen(String action, String status) {
  // Top banner - inverted
  display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(35, 2);
  display.println("MOVING");
  
  // Large UP arrow
  display.setTextColor(SSD1306_WHITE);
  display.fillTriangle(64, 18, 50, 38, 78, 38, SSD1306_WHITE);
  display.fillRect(58, 38, 12, 15, SSD1306_WHITE);
  
  // Action text
  display.setTextSize(2);
  display.setCursor(20, 45);
  display.println("FORWARD");
  
  // Bottom status
  display.drawLine(0, 54, 128, 54, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 57);
  display.println(status);
}

// --- BACKWARD Screen ---
void drawBackwardScreen(String action, String status) {
  // Top banner
  display.fillRect(0, 0, 128, 12, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(30, 2);
  display.println("REVERSING");
  
  // Large DOWN arrow
  display.setTextColor(SSD1306_WHITE);
  display.fillRect(58, 15, 12, 15, SSD1306_WHITE);
  display.fillTriangle(64, 45, 50, 30, 78, 30, SSD1306_WHITE);
  
  // Action text
  display.setTextSize(2);
  display.setCursor(10, 45);
  display.println("BACKWARD");
  
  // Bottom status
  display.drawLine(0, 54, 128, 54, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 57);
  display.println(status);
}

// --- LEFT Screen ---
void drawLeftScreen(String action, String status) {
  // Left side banner
  display.fillRect(0, 0, 25, 64, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(5, 15);
  display.println("T");
  display.setCursor(5, 25);
  display.println("U");
  display.setCursor(5, 35);
  display.println("R");
  display.setCursor(5, 45);
  display.println("N");
  
  // LEFT arrow
  display.setTextColor(SSD1306_WHITE);
  display.fillTriangle(35, 32, 55, 20, 55, 44, SSD1306_WHITE);
  display.fillRect(55, 28, 20, 8, SSD1306_WHITE);
  
  // Action text
  display.setTextSize(3);
  display.setCursor(80, 25);
  display.println("LEFT");
  
  // Bottom line
  display.drawLine(25, 54, 128, 54, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(30, 57);
  display.println(status);
}

// --- RIGHT Screen ---
void drawRightScreen(String action, String status) {
  // Right side banner
  display.fillRect(103, 0, 25, 64, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setTextSize(1);
  display.setCursor(110, 15);
  display.println("T");
  display.setCursor(110, 25);
  display.println("U");
  display.setCursor(110, 35);
  display.println("R");
  display.setCursor(110, 45);
  display.println("N");
  
  // RIGHT arrow
  display.setTextColor(SSD1306_WHITE);
  display.fillTriangle(93, 32, 73, 20, 73, 44, SSD1306_WHITE);
  display.fillRect(53, 28, 20, 8, SSD1306_WHITE);
  
  // Action text
  display.setTextSize(2);
  display.setCursor(5, 25);
  display.println("RIGHT");
  
  // Bottom line
  display.drawLine(0, 54, 103, 54, SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(5, 57);
  display.println(status);
}

// --- LEFT 90° Screen ---
void drawLeft90Screen(String action, String status) {
  // Thick border
  display.fillRect(0, 0, 128, 5, SSD1306_WHITE);
  display.fillRect(0, 59, 128, 5, SSD1306_WHITE);
  display.fillRect(0, 0, 5, 64, SSD1306_WHITE);
  
  // Curved arrow indicator
  display.drawCircle(40, 30, 20, SSD1306_WHITE);
  display.drawCircle(40, 30, 18, SSD1306_WHITE);
  display.fillTriangle(25, 25, 25, 35, 15, 30, SSD1306_WHITE);
  
  // 90° text
  display.setTextSize(3);
  display.setCursor(70, 15);
  display.println("90");
  display.drawCircle(115, 18, 3, SSD1306_WHITE);
  
  // LEFT text
  display.setTextSize(2);
  display.setCursor(70, 38);
  display.println("LEFT");
  
  // Status
  display.setTextSize(1);
  display.setCursor(8, 57);
  display.println(status);
}

// --- RIGHT 90° Screen ---
void drawRight90Screen(String action, String status) {
  // Thick border
  display.fillRect(0, 0, 128, 5, SSD1306_WHITE);
  display.fillRect(0, 59, 128, 5, SSD1306_WHITE);
  display.fillRect(123, 0, 5, 64, SSD1306_WHITE);
  
  // Curved arrow indicator
  display.drawCircle(88, 30, 20, SSD1306_WHITE);
  display.drawCircle(88, 30, 18, SSD1306_WHITE);
  display.fillTriangle(103, 25, 103, 35, 113, 30, SSD1306_WHITE);
  
  // 90° text
  display.setTextSize(3);
  display.setCursor(15, 15);
  display.println("90");
  display.drawCircle(60, 18, 3, SSD1306_WHITE);
  
  // RIGHT text
  display.setTextSize(2);
  display.setCursor(8, 38);
  display.println("RIGHT");
  
  // Status
  display.setTextSize(1);
  display.setCursor(8, 57);
  display.println(status);
}

// --- COMPLETE Screen ---
void showComplete() {
  display.clearDisplay();
  
  // Checkmark box
  display.fillRoundRect(10, 10, 108, 44, 5, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  
  // Big checkmark
  display.drawLine(30, 32, 45, 42, SSD1306_BLACK);
  display.drawLine(30, 33, 45, 43, SSD1306_BLACK);
  display.drawLine(45, 42, 95, 20, SSD1306_BLACK);
  display.drawLine(45, 43, 95, 21, SSD1306_BLACK);
  
  // Text
  display.setTextSize(2);
  display.setCursor(20, 15);
  display.println("SEQUENCE");
  display.setCursor(28, 35);
  display.println("COMPLETE");
  
  // Footer
  display.setTextColor(SSD1306_WHITE);
  display.setTextSize(1);
  display.setCursor(25, 57);
  display.println("All tests done!");
  
  display.display();
}

// ============================================
//          MOTOR CONTROL FUNCTIONS
// ============================================

void forward() {
  digitalWrite(Left1, HIGH);  
  digitalWrite(Left2, LOW);  
  digitalWrite(Right1, HIGH);  
  digitalWrite(Right2, LOW); 
}

void backward() {
  digitalWrite(Left1, LOW);  
  digitalWrite(Left2, HIGH);  
  digitalWrite(Right1, LOW);  
  digitalWrite(Right2, HIGH); 
}

void turnLeft() {
  // Left motor FORWARD, Right motor BACKWARD
  digitalWrite(Left1, HIGH);  
  digitalWrite(Left2, LOW);  
  digitalWrite(Right1, LOW);  
  digitalWrite(Right2, HIGH); 
}

void turnRight() {
  // Left motor BACKWARD, Right motor FORWARD
  digitalWrite(Left1, LOW);  
  digitalWrite(Left2, HIGH);  
  digitalWrite(Right1, HIGH);  
  digitalWrite(Right2, LOW); 
}

void Stop() {
  digitalWrite(Left1, LOW);  
  digitalWrite(Left2, LOW);  
  digitalWrite(Right1, LOW);  
  digitalWrite(Right2, LOW); 
}
