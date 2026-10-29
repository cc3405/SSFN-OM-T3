/*
 * OMER service robot - ESP32-S3 N16R8 MVP firmware
 *
 * Target: Arduino-ESP32 core 3.x, ESP32Servo library.
 * This is a safe bring-up scaffold, NOT a completed autonomous navigation system.
 * Physical emergency stop must interrupt motor-driver power independently of software.
 *
 * Serial commands at 115200 baud:
 *   help, status, arm, disarm, fwd, rev, left, right, stop, unload
 * Motion commands are low-speed and time-limited. Wheels must be lifted during first tests.
 */
#include <Arduino.h>
#include <ESP32Servo.h>

namespace Pins {
constexpr uint8_t LEFT_RPWM = 4;
constexpr uint8_t LEFT_LPWM = 5;
constexpr uint8_t RIGHT_RPWM = 6;
constexpr uint8_t RIGHT_LPWM = 7;
constexpr uint8_t DRIVER_ENABLE = 21; // Shared R_EN/L_EN enable; use level shifting if required
constexpr uint8_t LEFT_ENCODER_A = 15;
constexpr uint8_t RIGHT_ENCODER_A = 16;
constexpr uint8_t SONAR_LEFT_TRIG = 10;
constexpr uint8_t SONAR_LEFT_ECHO = 11;   // MUST be level-shifted to <= 3.3 V
constexpr uint8_t SONAR_RIGHT_TRIG = 12;
constexpr uint8_t SONAR_RIGHT_ECHO = 13;  // MUST be level-shifted to <= 3.3 V
constexpr uint8_t DUMP_SERVO = 14;        // Servo has a separate 5-6 V supply; common GND
constexpr uint8_t LIMIT_CLOSED = 17;
constexpr uint8_t LIMIT_OPEN = 18;
}

constexpr uint32_t SERIAL_BAUD = 115200;
constexpr uint32_t PWM_FREQUENCY_HZ = 16000;
constexpr uint8_t PWM_RESOLUTION_BITS = 8;
constexpr uint8_t MAX_TEST_PWM = 70;       // Conservative first-bring-up limit (0..255)
constexpr uint32_t MOTION_LIMIT_MS = 700; // Every manual motion pulse stops automatically
constexpr uint32_t COMMAND_WATCHDOG_MS = 1000;
constexpr uint32_t SONAR_TIMEOUT_US = 18000;
constexpr float OBSTACLE_STOP_CM = 20.0f;
constexpr int SERVO_CLOSED_DEG = 20;
constexpr int SERVO_OPEN_DEG = 95;

Servo dumpServo;
volatile uint32_t leftEncoderTicks = 0;
volatile uint32_t rightEncoderTicks = 0;
bool motorArmed = false;
bool motionActive = false;
uint32_t motionStartedMs = 0;
uint32_t lastCommandMs = 0;
String inputLine;

void IRAM_ATTR onLeftEncoder() { ++leftEncoderTicks; }
void IRAM_ATTR onRightEncoder() { ++rightEncoderTicks; }

void setMotorSide(uint8_t rpwmPin, uint8_t lpwmPin, int16_t signedPwm) {
  signedPwm = constrain(signedPwm, -MAX_TEST_PWM, MAX_TEST_PWM);
  if (signedPwm > 0) {
    analogWrite(lpwmPin, 0);
    analogWrite(rpwmPin, signedPwm);
  } else if (signedPwm < 0) {
    analogWrite(rpwmPin, 0);
    analogWrite(lpwmPin, -signedPwm);
  } else {
    analogWrite(rpwmPin, 0);
    analogWrite(lpwmPin, 0);
  }
}

void stopMotors() {
  setMotorSide(Pins::LEFT_RPWM, Pins::LEFT_LPWM, 0);
  setMotorSide(Pins::RIGHT_RPWM, Pins::RIGHT_LPWM, 0);
  digitalWrite(Pins::DRIVER_ENABLE, LOW); // Disable both drivers after PWM is zero
  motionActive = false;
}

float readDistanceCm(uint8_t trigPin, uint8_t echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(3);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  const uint32_t duration = pulseIn(echoPin, HIGH, SONAR_TIMEOUT_US);
  if (duration == 0) return -1.0f;
  return duration * 0.0343f / 2.0f;
}

bool obstacleDetected() {
  const float leftCm = readDistanceCm(Pins::SONAR_LEFT_TRIG, Pins::SONAR_LEFT_ECHO);
  const float rightCm = readDistanceCm(Pins::SONAR_RIGHT_TRIG, Pins::SONAR_RIGHT_ECHO);
  // Fail safe: unknown/no-echo readings block motion until both sensors are verified.
  // Ultrasonic sensing is only a near-field aid, never the only safety mechanism.
  if (leftCm <= 0 || rightCm <= 0) return true;
  return leftCm < OBSTACLE_STOP_CM || rightCm < OBSTACLE_STOP_CM;
}

void startMotion(int16_t leftPwm, int16_t rightPwm) {
  if (!motorArmed) {
    Serial.println("BLOCKED: run 'arm' first.");
    stopMotors();
    return;
  }
  if (obstacleDetected()) {
    stopMotors();
    Serial.println("BLOCKED: obstacle/unknown sonar reading. Check both sensors.");
    return;
  }
  digitalWrite(Pins::DRIVER_ENABLE, HIGH); // EN is raised only for an armed motion pulse
  delayMicroseconds(1000);
  setMotorSide(Pins::LEFT_RPWM, Pins::LEFT_LPWM, leftPwm);
  setMotorSide(Pins::RIGHT_RPWM, Pins::RIGHT_LPWM, rightPwm);
  motionStartedMs = millis();
  lastCommandMs = motionStartedMs;
  motionActive = true;
}

void printStatus() {
  noInterrupts();
  const uint32_t left = leftEncoderTicks;
  const uint32_t right = rightEncoderTicks;
  interrupts();
  Serial.printf("armed=%s motion=%s encL=%lu encR=%lu limitClosed=%d limitOpen=%d\n",
                motorArmed ? "yes" : "no", motionActive ? "yes" : "no",
                static_cast<unsigned long>(left), static_cast<unsigned long>(right),
                digitalRead(Pins::LIMIT_CLOSED), digitalRead(Pins::LIMIT_OPEN));
}

void handleCommand(String command) {
  command.trim();
  command.toLowerCase();
  if (command == "help") {
    Serial.println("Commands: help, status, arm, disarm, fwd, rev, left, right, stop, unload");
  } else if (command == "status") {
    printStatus();
  } else if (command == "arm") {
    motorArmed = true;
    Serial.println("MOTOR ARMED: physical emergency stop must remain accessible.");
  } else if (command == "disarm" || command == "stop") {
    stopMotors();
    motorArmed = false;
    Serial.println("Motors stopped; drive disarmed.");
  } else if (command == "fwd") {
    startMotion(MAX_TEST_PWM, MAX_TEST_PWM);
  } else if (command == "rev") {
    startMotion(-MAX_TEST_PWM, -MAX_TEST_PWM);
  } else if (command == "left") {
    startMotion(-MAX_TEST_PWM, MAX_TEST_PWM);
  } else if (command == "right") {
    startMotion(MAX_TEST_PWM, -MAX_TEST_PWM);
  } else if (command == "unload") {
    stopMotors();
    dumpServo.write(SERVO_OPEN_DEG);
    delay(500);
    dumpServo.write(SERVO_CLOSED_DEG);
    Serial.println("Unload servo cycle issued; verify limit switches and mechanism physically.");
  } else if (command.length() > 0) {
    Serial.println("Unknown command; type 'help'.");
  }
}

void setup() {
  const uint8_t outputs[] = {
    Pins::LEFT_RPWM, Pins::LEFT_LPWM, Pins::RIGHT_RPWM, Pins::RIGHT_LPWM,
    Pins::DRIVER_ENABLE, Pins::SONAR_LEFT_TRIG, Pins::SONAR_RIGHT_TRIG
  };
  for (uint8_t pin : outputs) {
    pinMode(pin, OUTPUT);
    digitalWrite(pin, LOW);
  }
  pinMode(Pins::SONAR_LEFT_ECHO, INPUT);
  pinMode(Pins::SONAR_RIGHT_ECHO, INPUT);
  pinMode(Pins::LEFT_ENCODER_A, INPUT_PULLUP);
  pinMode(Pins::RIGHT_ENCODER_A, INPUT_PULLUP);
  pinMode(Pins::LIMIT_CLOSED, INPUT_PULLUP);
  pinMode(Pins::LIMIT_OPEN, INPUT_PULLUP);

  analogWriteResolution(Pins::LEFT_RPWM, PWM_RESOLUTION_BITS);
  analogWriteResolution(Pins::LEFT_LPWM, PWM_RESOLUTION_BITS);
  analogWriteResolution(Pins::RIGHT_RPWM, PWM_RESOLUTION_BITS);
  analogWriteResolution(Pins::RIGHT_LPWM, PWM_RESOLUTION_BITS);
  analogWriteFrequency(Pins::LEFT_RPWM, PWM_FREQUENCY_HZ);
  analogWriteFrequency(Pins::LEFT_LPWM, PWM_FREQUENCY_HZ);
  analogWriteFrequency(Pins::RIGHT_RPWM, PWM_FREQUENCY_HZ);
  analogWriteFrequency(Pins::RIGHT_LPWM, PWM_FREQUENCY_HZ);
  stopMotors();

  attachInterrupt(digitalPinToInterrupt(Pins::LEFT_ENCODER_A), onLeftEncoder, RISING);
  attachInterrupt(digitalPinToInterrupt(Pins::RIGHT_ENCODER_A), onRightEncoder, RISING);

  dumpServo.setPeriodHertz(50);
  dumpServo.attach(Pins::DUMP_SERVO, 500, 2400);
  dumpServo.write(SERVO_CLOSED_DEG);

  Serial.begin(SERIAL_BAUD);
  delay(300);
  Serial.println("\nOMER ESP32-S3 MVP firmware ready. Motors DISARMED.");
  Serial.println("Verify level shifters, motor-driver enable wiring and hardware E-stop before 'arm'.");
  Serial.println("Type 'help' for commands.");
}

void loop() {
  while (Serial.available() > 0) {
    const char c = static_cast<char>(Serial.read());
    if (c == '\n' || c == '\r') {
      if (inputLine.length() > 0) {
        handleCommand(inputLine);
        inputLine = "";
      }
    } else if (inputLine.length() < 48) {
      inputLine += c;
    } else {
      inputLine = "";
      Serial.println("Input too long; discarded.");
    }
  }

  if (motionActive) {
    const uint32_t elapsed = millis() - motionStartedMs;
    if (elapsed >= MOTION_LIMIT_MS || millis() - lastCommandMs >= COMMAND_WATCHDOG_MS) {
      stopMotors();
      Serial.println("Motion pulse complete; motors stopped.");
    } else if (obstacleDetected()) {
      stopMotors();
      Serial.println("STOP: obstacle or unknown sonar reading.");
    }
  }
}
