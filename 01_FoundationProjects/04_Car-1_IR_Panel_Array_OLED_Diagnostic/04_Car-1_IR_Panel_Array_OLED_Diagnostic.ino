// ============================================
//        IR PANEL DIAGNOSTIC PROGRAM
// ============================================

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ---------------- OLED CONFIG ----------------
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C

Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

// ---------------- IR PINS --------------------
#define IR_LEFT         A0
#define IR_LEFT_CENTER  A1
#define IR_RIGHT_CENTER A2
#define IR_RIGHT        A3

void setup() {
  Serial.begin(9600);
  delay(500);

  // IR pins
  pinMode(IR_LEFT, INPUT);
  pinMode(IR_LEFT_CENTER, INPUT);
  pinMode(IR_RIGHT_CENTER, INPUT);
  pinMode(IR_RIGHT, INPUT);

  // OLED init
  if (!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println("OLED init failed!");
    while (1);
  }

  display.clearDisplay();
  display.setTextSize(1);
  display.setTextColor(SSD1306_WHITE);

  // Startup screen
  display.setCursor(10, 20);
  display.setTextSize(2);
  display.println("IR PANEL");
  display.setTextSize(1);
  display.setCursor(15, 40);
  display.println("DIAGNOSTIC");
  display.display();

  Serial.println("IR Panel Diagnostic Started!");
  delay(2000);
}

void loop() {
  // Digital reads
  int L  = digitalRead(IR_LEFT);
  int LC = digitalRead(IR_LEFT_CENTER);
  int RC = digitalRead(IR_RIGHT_CENTER);
  int R  = digitalRead(IR_RIGHT);

  // Analog reads
  int aL  = analogRead(IR_LEFT);
  int aLC = analogRead(IR_LEFT_CENTER);
  int aRC = analogRead(IR_RIGHT_CENTER);
  int aR  = analogRead(IR_RIGHT);

  // ---------- SERIAL OUTPUT ----------
  Serial.println("=============================");
  Serial.print("Digital: L=");  Serial.print(L ? "BLOCK" : "CLEAR");
  Serial.print(" | LC="); Serial.print(LC ? "BLOCK" : "CLEAR");
  Serial.print(" | RC="); Serial.print(RC ? "BLOCK" : "CLEAR");
  Serial.print(" | R=");  Serial.println(R ? "BLOCK" : "CLEAR");

  Serial.print("Analog:  ");
  Serial.print(aL);  Serial.print(" | ");
  Serial.print(aLC); Serial.print(" | ");
  Serial.print(aRC); Serial.print(" | ");
  Serial.println(aR);

  // ---------- SMART OLED DISPLAY ----------
  display.clearDisplay();

  // Title bar
  display.setTextSize(1);
  display.fillRect(0, 0, 128, 10, SSD1306_WHITE);
  display.setTextColor(SSD1306_BLACK);
  display.setCursor(8, 1);
  display.println("IR PANEL STATUS");
  display.setTextColor(SSD1306_WHITE);

  // Draw sensors - adjusted Y positions
  int yPos = 14;  // Start lower to avoid title
  int xBox = 70;
  
  drawSensor("L ", L, aL, 0, yPos, xBox);
  drawSensor("LC", LC, aLC, 0, yPos + 11, xBox);
  drawSensor("RC", RC, aRC, 0, yPos + 22, xBox);
  drawSensor("R ", R, aR, 0, yPos + 33, xBox);

  // ✅ FIXED FOOTER - No overlap, clear legend
  display.drawLine(0, 55, 128, 55, SSD1306_WHITE);  // Separator line
  display.setCursor(2, 57);
  display.setTextSize(1);
  display.println("1=BLOCK | 0=CLEAR");

  display.display();
  delay(200);
}

// Sensor drawing function (unchanged)
void drawSensor(String label, int digital, int analog, int x, int y, int boxX) {
  display.setTextSize(1);
  
  // Sensor label
  display.setCursor(x, y);
  display.print(label);
  display.print(":");
  
  // Status text
  display.setCursor(x + 20, y);
  if (digital == 1) {
    display.print("BLOCK");
  } else {
    display.print("CLEAR");
  }
  
  // Visual indicator box
  if (digital == 1) {
    // BLOCKED - Filled box
    display.fillRect(boxX, y - 1, 50, 9, SSD1306_WHITE);
    display.setTextColor(SSD1306_BLACK);
    display.setCursor(boxX + 3, y);
    display.print("OBJECT");
    display.setTextColor(SSD1306_WHITE);
  } else {
    // CLEAR - Empty box
    display.drawRect(boxX, y - 1, 50, 9, SSD1306_WHITE);
    display.setCursor(boxX + 10, y);
    display.print("OK");
  }
}
