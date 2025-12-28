
#define IR_LEFT         A0  // Analog pin, can read 0-1023
#define IR_LEFT_CENTER  A1  // Analog pin, can read 0-1023
#define IR_RIGHT_CENTER A2  // Analog pin, can read 0-1023
#define IR_RIGHT        A3  // Analog pin, can read 0-1023

void setup() {
  pinMode(IR_LEFT_CENTER, INPUT);  // Test A1 only
  Serial.begin(9600);
}

void loop() {
  // Test ONLY Left Sensor (A0)
  Serial.print("A1 Digital: "); Serial.print(digitalRead(IR_LEFT_CENTER));
  Serial.print(" | A1 Analog: "); Serial.println(analogRead(IR_LEFT_CENTER));
  
  delay(300);
}
