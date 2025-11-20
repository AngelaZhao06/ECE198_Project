/*
 * MPU6050 -> JSON Serial Output (every 1s)
 * Minimal additions to your original code to print machine-parseable JSON lines.
 * Assumes same MPU6050 read / metrics logic as your original sketch.
 */

#include <Wire.h>

// MPU6050 I2C address
const int MPU_ADDR = 0x68;
int16_t accelX, accelY, accelZ;
int16_t gyroX, gyroY, gyroZ;

float activityLevel = 0;
float swayMagnitude = 0;
unsigned long lastReadTime = 0;
const int READ_INTERVAL = 100;
const int WINDOW_SIZE = 10;
float activityWindow[WINDOW_SIZE];
float swayWindow[WINDOW_SIZE];
int windowIndex = 0;

const float LOW_ACTIVITY_THRESHOLD = 500;
const float MODERATE_ACTIVITY_THRESHOLD = 2000;
const float HIGH_SWAY_THRESHOLD = 15000;
const float CRITICAL_SWAY_THRESHOLD = 25000;
const float SUDDEN_MOVEMENT_THRESHOLD = 30000;

enum RiskLevel {
  RISK_NORMAL,
  RISK_ELEVATED,
  RISK_HIGH,
  RISK_CRITICAL
};
RiskLevel currentRisk = RISK_NORMAL;

void setup() {
  Serial.begin(115200);
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1C);
  Wire.write(0x00);
  Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B);
  Wire.write(0x00);
  Wire.endTransmission(true);
  for (int i = 0; i < WINDOW_SIZE; i++) {
    activityWindow[i] = 0;
    swayWindow[i] = 0;
  }
  delay(1000);
}

void loop() {
  unsigned long currentTime = millis();
  if (currentTime - lastReadTime >= READ_INTERVAL) {
    lastReadTime = currentTime;
    readMPU6050();
    calculateMetrics();
    assessRisk();
    // Print a single JSON line every 1000ms (1s) for easier UI updating
    static unsigned long lastJson = 0;
    if (millis() - lastJson >= 1000) {
      lastJson = millis();
      printJson();
    }
  }
}

void readMPU6050() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 14, true);
  accelX = Wire.read() << 8 | Wire.read();
  accelY = Wire.read() << 8 | Wire.read();
  accelZ = Wire.read() << 8 | Wire.read();
  Wire.read(); Wire.read();
  gyroX = Wire.read() << 8 | Wire.read();
  gyroY = Wire.read() << 8 | Wire.read();
  gyroZ = Wire.read() << 8 | Wire.read();
}

void calculateMetrics() {
  float accelMagnitude = sqrt(sq(accelX) + sq(accelY) + sq(accelZ));
  float gyroMagnitude = sqrt(sq(gyroX) + sq(gyroY) + sq(gyroZ));
  activityWindow[windowIndex] = accelMagnitude;
  swayWindow[windowIndex] = gyroMagnitude;
  windowIndex = (windowIndex + 1) % WINDOW_SIZE;
  activityLevel = 0;
  swayMagnitude = 0;
  for (int i = 0; i < WINDOW_SIZE; i++) {
    activityLevel += activityWindow[i];
    swayMagnitude += swayWindow[i];
  }
  activityLevel /= WINDOW_SIZE;
  swayMagnitude /= WINDOW_SIZE;
}

void assessRisk() {
  if (swayMagnitude > CRITICAL_SWAY_THRESHOLD || activityLevel > SUDDEN_MOVEMENT_THRESHOLD) {
    currentRisk = RISK_CRITICAL;
  } else if (swayMagnitude > HIGH_SWAY_THRESHOLD) {
    currentRisk = RISK_HIGH;
  } else if (swayMagnitude > HIGH_SWAY_THRESHOLD * 0.6 || activityLevel > MODERATE_ACTIVITY_THRESHOLD) {
    currentRisk = RISK_ELEVATED;
  } else {
    currentRisk = RISK_NORMAL;
  }
}

void printJson() {
  // Example JSON:
  // {"ts":1700000000,"activity":1234.1,"sway":5678.2,"risk":2,"ax":123,"ay":456,"az":789}
  unsigned long ts = millis();
  Serial.print("{");
  Serial.print("\"ts\":"); Serial.print(ts);
  Serial.print(",\"activity\":"); Serial.print(activityLevel,1);
  Serial.print(",\"sway\":"); Serial.print(swayMagnitude,1);
  Serial.print(",\"risk\":"); Serial.print((int)currentRisk);
  Serial.print(",\"ax\":"); Serial.print(accelX);
  Serial.print(",\"ay\":"); Serial.print(accelY);
  Serial.print(",\"az\":"); Serial.print(accelZ);
  Serial.print(",\"gx\":"); Serial.print(gyroX);
  Serial.print(",\"gy\":"); Serial.print(gyroY);
  Serial.print(",\"gz\":"); Serial.print(gyroZ);
  Serial.println("}");
}
