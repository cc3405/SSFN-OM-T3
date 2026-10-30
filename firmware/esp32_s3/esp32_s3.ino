/*
 * OMER service robot - ESP32-S3 N16R8 bring-up firmware
 * Arduino-ESP32 core 3.x; no third-party library required.
 *
 * Implements: bounded manual drive pulses, sonar fail-safe, quadrature encoder
 * counting, and a PCA9685 3-channel hobby-servo interface for the PRODUCT
 * PICKUP arm. The separate human-powered unloading device is deliberately
 * not connected to this vehicle and is not controlled here.
 *
 * SAFETY: software stop is not an emergency stop. A physical E-stop must
 * interrupt motor-driver power. Test with wheels lifted and servo power
 * disconnected first. Servo endpoints MUST be calibrated mechanically.
 */
#include <Arduino.h>
#include <Wire.h>

namespace Pins {
constexpr uint8_t LEFT_RPWM = 4, LEFT_LPWM = 5;
constexpr uint8_t RIGHT_RPWM = 6, RIGHT_LPWM = 7;
constexpr uint8_t DRIVER_ENABLE = 21;
constexpr uint8_t LEFT_ENCODER_A = 15, LEFT_ENCODER_B = 8;
constexpr uint8_t RIGHT_ENCODER_A = 16, RIGHT_ENCODER_B = 9;
constexpr uint8_t SONAR_LEFT_TRIG = 10, SONAR_LEFT_ECHO = 11;
constexpr uint8_t SONAR_RIGHT_TRIG = 12, SONAR_RIGHT_ECHO = 13;
constexpr uint8_t LIMIT_CLOSED = 17, LIMIT_OPEN = 18;
constexpr uint8_t I2C_SDA = 1, I2C_SCL = 2;
}

namespace Pca {
constexpr uint8_t ADDRESS = 0x40;
constexpr uint8_t MODE1 = 0x00;
constexpr uint8_t PRESCALE = 0xFE;
constexpr uint8_t LED0_ON_L = 0x06;
constexpr uint8_t FULL_OFF_H = 0x09;
constexpr uint8_t SERVO_CHANNELS[3] = {0, 1, 2}; // shoulder, elbow, gripper
constexpr uint16_t SERVO_MIN_US = 600;  // conservative starting calibration only
constexpr uint16_t SERVO_MAX_US = 2400; // MUST be calibrated for the actual linkage
constexpr uint8_t SERVO_MIN_DEG = 20;
constexpr uint8_t SERVO_MAX_DEG = 160;
}

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t PWM_FREQUENCY_HZ = 16000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr uint8_t MAX_TEST_PWM = 70;
constexpr uint32_t MOTION_LIMIT_MS = 700;
constexpr uint32_t COMMAND_WATCHDOG_MS = 1000;
constexpr uint32_t SONAR_TIMEOUT_US = 18000;
constexpr float OBSTACLE_STOP_CM = 20.0f;

volatile int32_t leftEncoderTicks = 0;
volatile int32_t rightEncoderTicks = 0;
bool motorArmed = false;
bool motionActive = false;
bool pcaReady = false;
bool armEnabled = false;
uint32_t motionStartedMs = 0;
uint32_t lastCommandMs = 0;
String inputLine;

void IRAM_ATTR onLeftEncoder() {
  leftEncoderTicks += digitalRead(Pins::LEFT_ENCODER_B) ? 1 : -1;
}
void IRAM_ATTR onRightEncoder() {
  rightEncoderTicks += digitalRead(Pins::RIGHT_ENCODER_B) ? 1 : -1;
}

bool pcaWrite(uint8_t reg, uint8_t value) {
  Wire.beginTransmission(Pca::ADDRESS);
  Wire.write(reg);
  Wire.write(value);
  return Wire.endTransmission() == 0;
}

bool pcaBegin() {
  Wire.begin(Pins::I2C_SDA, Pins::I2C_SCL);
  if (!pcaWrite(Pca::MODE1, 0x10)) return false; // sleep
  if (!pcaWrite(Pca::PRESCALE, 121)) return false; // 25 MHz / (4096*50 Hz) - 1
  if (!pcaWrite(Pca::MODE1, 0x20)) return false; // auto-increment, wake
  delay(5);
  if (!pcaWrite(Pca::MODE1, 0xA0)) return false; // restart + auto-increment
  for (uint8_t ch : Pca::SERVO_CHANNELS) {
    const uint8_t base = Pca::LED0_ON_L + 4 * ch;
    pcaWrite(base, 0); pcaWrite(base + 1, 0);
    pcaWrite(base + 2, 0); pcaWrite(base + 3, 0x10); // full off
  }
  return true;
}

void pcaSetServo(uint8_t channel, uint8_t degrees) {
  if (!pcaReady || channel > 2) return;
  degrees = constrain(degrees, Pca::SERVO_MIN_DEG, Pca::SERVO_MAX_DEG);
  const uint16_t pulseUs = Pca::SERVO_MIN_US +
      (static_cast<uint32_t>(degrees) * (Pca::SERVO_MAX_US - Pca::SERVO_MIN_US)) / 180;
  const uint16_t ticks = (static_cast<uint32_t>(pulseUs) * 50 * 4096) / 1000000;
  const uint8_t base = Pca::LED0_ON_L + 4 * channel;
  Wire.beginTransmission(Pca::ADDRESS);
  Wire.write(base);
  Wire.write(0); Wire.write(0);
  Wire.write(static_cast<uint8_t>(ticks & 0xFF));
  Wire.write(static_cast<uint8_t>(ticks >> 8));
  Wire.endTransmission();
}

void pcaAllOff() {
  if (!pcaReady) return;
  for (uint8_t ch : Pca::SERVO_CHANNELS) {
    const uint8_t base = Pca::LED0_ON_L + 4 * ch;
    Wire.beginTransmission(Pca::ADDRESS);
    Wire.write(base); Wire.write(0); Wire.write(0); Wire.write(0); Wire.write(0x10);
    Wire.endTransmission();
  }
  armEnabled = false;
}

int8_t servoIndex(const String &name) {
  if (name == "shoulder") return 0;
  if (name == "elbow") return 1;
  if (name == "grip" || name == "gripper") return 2;
  return -1;
}

void setMotorSide(uint8_t rpwmPin, uint8_t lpwmPin, int16_t signedPwm) {
  signedPwm = constrain(signedPwm, -MAX_TEST_PWM, MAX_TEST_PWM);
  if (signedPwm > 0) {
    analogWrite(lpwmPin, 0); analogWrite(rpwmPin, signedPwm);
  } else if (signedPwm < 0) {
    analogWrite(rpwmPin, 0); analogWrite(lpwmPin, -signedPwm);
  } else {
    analogWrite(rpwmPin, 0); analogWrite(lpwmPin, 0);
  }
}

void stopMotors() {
  setMotorSide(Pins::LEFT_RPWM, Pins::LEFT_LPWM, 0);
  setMotorSide(Pins::RIGHT_RPWM, Pins::RIGHT_LPWM, 0);
  digitalWrite(Pins::DRIVER_ENABLE, LOW);
  motionActive = false;
}

float readDistanceCm(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW); delayMicroseconds(3);
  digitalWrite(trigPin, HIGH); delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  const uint32_t duration = pulseIn(echoPin, HIGH, SONAR_TIMEOUT_US);
  if (duration == 0) return -1.0f;
  return duration * 0.0343f / 2.0f;
}

bool obstacleDetected() {
  const float leftCm = readDistanceCm(Pins::SONAR_LEFT_TRIG, Pins::SONAR_LEFT_ECHO);
  const float rightCm = readDistanceCm(Pins::SONAR_RIGHT_TRIG, Pins::SONAR_RIGHT_ECHO);
  if (leftCm <= 0 || rightCm <= 0) return true; // fail closed on unknown readings
  return leftCm < OBSTACLE_STOP_CM || rightCm < OBSTACLE_STOP_CM;
}

void startMotion(int16_t leftPwm, int16_t rightPwm) {
  if (!motorArmed) {
    Serial.println("BLOCKED: run 'arm' first."); stopMotors(); return;
  }
  if (obstacleDetected()) {
    stopMotors(); Serial.println("BLOCKED: obstacle/unknown sonar reading."); return;
  }
  digitalWrite(Pins::DRIVER_ENABLE, HIGH);
  delayMicroseconds(1000);
  setMotorSide(Pins::LEFT_RPWM, Pins::LEFT_LPWM, leftPwm);
  setMotorSide(Pins::RIGHT_RPWM, Pins::RIGHT_LPWM, rightPwm);
  motionStartedMs = millis(); lastCommandMs = motionStartedMs; motionActive = true;
}

void printStatus() {
  noInterrupts();
  const int32_t left = leftEncoderTicks, right = rightEncoderTicks;
  interrupts();
  Serial.printf("motorArmed=%s motion=%s encL=%ld encR=%ld limitClosed=%d limitOpen=%d\n",
      motorArmed ? "yes" : "no", motionActive ? "yes" : "no",
      static_cast<long>(left), static_cast<long>(right),
      digitalRead(Pins::LIMIT_CLOSED), digitalRead(Pins::LIMIT_OPEN));
  Serial.printf("pca9685=%s arm=%s (channels 0/1/2 = shoulder/elbow/grip)\n",
      pcaReady ? "ready" : "not-found", armEnabled ? "enabled" : "off");
}

void printHelp() {
  Serial.println("Drive: help status arm disarm fwd rev left right stop encreset");
  Serial.println("Pickup arm: armenable armhome armoff servo shoulder|elbow|grip 20..160");
  Serial.println("No vehicle unloading command exists; unloading device is separate/manual.");
}

void handleCommand(String command) {
  command.trim(); command.toLowerCase();
  if (command == "help") printHelp();
  else if (command == "status") printStatus();
  else if (command == "arm") {
    motorArmed = true; Serial.println("Drive armed. Physical E-stop must remain accessible.");
  } else if (command == "disarm" || command == "stop") {
    stopMotors(); motorArmed = false; Serial.println("Drive stopped and disarmed.");
  } else if (command == "fwd") startMotion(MAX_TEST_PWM, MAX_TEST_PWM);
  else if (command == "rev") startMotion(-MAX_TEST_PWM, -MAX_TEST_PWM);
  else if (command == "left") startMotion(-MAX_TEST_PWM, MAX_TEST_PWM);
  else if (command == "right") startMotion(MAX_TEST_PWM, -MAX_TEST_PWM);
  else if (command == "encreset") {
    noInterrupts(); leftEncoderTicks = 0; rightEncoderTicks = 0; interrupts();
    Serial.println("Encoder counts reset.");
  } else if (command == "armenable" || command == "armhome") {
    if (!pcaReady) Serial.println("ERROR: PCA9685 not detected at I2C 0x40.");
    else {
      armEnabled = true;
      pcaSetServo(0, 90); pcaSetServo(1, 90); pcaSetServo(2, 90);
      Serial.println("Pickup arm set to neutral 90/90/90. Check linkage clearance.");
    }
  } else if (command == "armoff") {
    pcaAllOff(); Serial.println("PCA9685 servo outputs disabled.");
  } else if (command.startsWith("servo ")) {
    const int split = command.indexOf(' ', 6);
    if (split < 0 || !armEnabled) {
      Serial.println("Usage: servo shoulder|elbow|grip 20..160 (run armenable first)");
    } else {
      const String axis = command.substring(6, split);
      const String value = command.substring(split + 1);
      const int index = servoIndex(axis);
      char *end = nullptr;
      const long degrees = strtol(value.c_str(), &end, 10);
      if (index < 0 || end == value.c_str() || *end != '\0' ||
          degrees < Pca::SERVO_MIN_DEG || degrees > Pca::SERVO_MAX_DEG) {
        Serial.println("ERROR: valid axis and angle 20..160 required.");
      } else {
        pcaSetServo(static_cast<uint8_t>(index), static_cast<uint8_t>(degrees));
        Serial.printf("Servo %s -> %ld deg\n", axis.c_str(), degrees);
      }
    }
  } else if (command.length() > 0) Serial.println("Unknown command; type 'help'.");
}

void setup() {
  const uint8_t outputs[] = {Pins::LEFT_RPWM, Pins::LEFT_LPWM, Pins::RIGHT_RPWM,
      Pins::RIGHT_LPWM, Pins::DRIVER_ENABLE, Pins::SONAR_LEFT_TRIG, Pins::SONAR_RIGHT_TRIG};
  for (uint8_t pin : outputs) { pinMode(pin, OUTPUT); digitalWrite(pin, LOW); }
  pinMode(Pins::SONAR_LEFT_ECHO, INPUT); pinMode(Pins::SONAR_RIGHT_ECHO, INPUT);
  pinMode(Pins::LEFT_ENCODER_A, INPUT_PULLUP); pinMode(Pins::LEFT_ENCODER_B, INPUT_PULLUP);
  pinMode(Pins::RIGHT_ENCODER_A, INPUT_PULLUP); pinMode(Pins::RIGHT_ENCODER_B, INPUT_PULLUP);
  pinMode(Pins::LIMIT_CLOSED, INPUT_PULLUP); pinMode(Pins::LIMIT_OPEN, INPUT_PULLUP);

  analogWriteResolution(Pins::LEFT_RPWM, PWM_RESOLUTION_BITS);
  analogWriteResolution(Pins::LEFT_LPWM, PWM_RESOLUTION_BITS);
  analogWriteResolution(Pins::RIGHT_RPWM, PWM_RESOLUTION_BITS);
  analogWriteResolution(Pins::RIGHT_LPWM, PWM_RESOLUTION_BITS);
  analogWriteFrequency(Pins::LEFT_RPWM, PWM_FREQUENCY_HZ);
  analogWriteFrequency(Pins::LEFT_LPWM, PWM_FREQUENCY_HZ);
  analogWriteFrequency(Pins::RIGHT_RPWM, PWM_FREQUENCY_HZ);
  analogWriteFrequency(Pins::RIGHT_LPWM, PWM_FREQUENCY_HZ);
  stopMotors();

  attachInterrupt(digitalPinToInterrupt(Pins::LEFT_ENCODER_A), onLeftEncoder, CHANGE);
  attachInterrupt(digitalPinToInterrupt(Pins::RIGHT_ENCODER_A), onRightEncoder, CHANGE);
  Serial.begin(SERIAL_BAUD);
  pcaReady = pcaBegin();
  delay(300);
  Serial.println("\nOMER ESP32-S3 firmware ready. Drive DISARMED; servo outputs OFF.");
  Serial.printf("PCA9685: %s at 0x40. Verify servo power and calibrated travel before use.\n",
      pcaReady ? "detected" : "not detected");
  Serial.println("Verify level shifters, driver enable and hardware E-stop before drive 'arm'.");
  printHelp();
}

void loop() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) { handleCommand(inputLine); inputLine = ""; }
    } else if (inputLine.length() < 48) inputLine += c;
    else { inputLine = ""; Serial.println("Input too long; discarded."); }
  }

  if (motionActive) {
    const uint32_t elapsed = millis() - motionStartedMs;
    if (elapsed >= MOTION_LIMIT_MS || millis() - lastCommandMs >= COMMAND_WATCHDOG_MS) {
      stopMotors(); Serial.println("Motion pulse complete; motors stopped.");
    } else if (obstacleDetected()) {
      stopMotors(); Serial.println("STOP: obstacle or unknown sonar reading.");
    }
  }
}
