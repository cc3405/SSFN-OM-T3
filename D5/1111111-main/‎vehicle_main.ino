/*
 * ============================================================
 *  vehicle_main.ino  ——  “星驰一号” 车辆主控程序
 *  航天未来科技组 · OM 长期题《OMER 的购物服务》
 * ------------------------------------------------------------
 *  硬件：Arduino Nano (ATmega328P)
 *    · 2× 直流减速电机(带编码器) + TB6612FNG 双路H桥 —— 差速驱动
 *    · MPU6050 陀螺仪(I2C) —— 航向闭环
 *    · nRF24L01 2.4G 无线 —— 双模控制(自动航点/手动接管)
 *    · SSD1306 OLED(I2C) —— 购物清单与任务状态显示
 *    · 舵机×2 —— 取货铲翻转 / 拖钩释放
 *  引脚分配见 README.md 接线表
 * ------------------------------------------------------------
 *  控制架构：
 *    上层  航点状态机(MISSION[]) —— 场地坐标以出发点为原点
 *    中层  纯跟踪转向 + 距离减速 + 原地转向(差速)
 *    底层  编码器里程计 + 陀螺仪航向融合 —— 双闭环 PID
 * ============================================================
 */
#include <SPI.h>
#include <RF24.h>
#include <Servo.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
/* ---------------- 1. 可调参数（现场标定区） ---------------- */
// —— 机械参数
const float WHEEL_DIA   = 0.080f;   // 驱动轮直径 m
const float TRACK_WIDTH = 0.420f;   // 左右轮距 m
const int   ENC_PPR     = 330;      // 编码器每圈计数(减速比×线数)
// —— 运动限幅（安全第一，可按场地实况下调）
const float V_MAX     = 0.60f;      // 最大线速度 m/s
const float W_MAX     = 2.20f;      // 最大角速度 rad/s
const float V_APPROACH = 0.18f;     // 挂钩/靠台逼近速度 m/s
const float WPT_TOL   = 0.07f;      // 航点到达判定半径 m
const float HEAD_TOL  = 0.06f;      // 转向到位判定 rad(≈3.4°)
// —— PID 增益（先比例后积分微分，逐项整定）
float KP_HEAD = 1.90f, KI_HEAD = 0.0f, KD_HEAD = 0.12f;   // 航向环
float KP_DIST = 2.60f;                                     // 距离环
// —— 无线安全
const uint32_t RADIO_TIMEOUT_MS = 300;   // 遥控失联超时 → 自动停车
// —— 舵机行程
const int SCOOP_UP_DEG   = 8;    // 取货铲 平放
const int SCOOP_DUMP_DEG = 95;   // 取货铲 翻倒
const int HOOK_LOCK_DEG  = 12;   // 拖钩 锁定
const int HOOK_FREE_DEG  = 105;  // 拖钩 释放
/* ---------------- 2. 引脚定义 ---------------- */
// TB6612FNG
const int PIN_STBY = 4;
const int PIN_PWMA = 5, PIN_PWMB = 6;          // Timer0 PWM，频率一致
const int PIN_AIN1 = A0, PIN_AIN2 = A1;       // 左电机方向
const int PIN_BIN1 = A2, PIN_BIN2 = A3;       // 右电机方向
// 编码器(仅A相，方向由指令符号判定)
const int PIN_ENC_L = 2;                       // INT0
const int PIN_ENC_R = 3;                       // INT1
// 舵机
const int PIN_SERVO_SCOOP = 7;
const int PIN_SERVO_HOOK  = 9;
// 无线
const int PIN_RF_CE = 8, PIN_RF_CSN = 10;
// I2C: A4=SDA A5=SCL (MPU6050 + OLED)
/* ---------------- 3. 无线协议 ---------------- */
// 5 字节控制帧: [魔数][lx][ly][按键][序号]
struct RadioCmd { uint8_t magic, lx, ly, btn, seq; };
const uint8_t RF_MAGIC = 0xA5;
// 按键位: bit0=模式切换  bit1=释放购物车  bit2=铲翻转  bit3=卸货完成确认
const uint8_t BTN_MODE = 0x01, BTN_RELEASE = 0x02, BTN_SCOOP = 0x04, BTN_UNLOAD_OK = 0x08;
const uint64_t RF_PIPE_V = 0xF0F0F0F0E1LL;   // 车辆地址
const uint64_t RF_PIPE_R = 0xF0F0F0F0D2LL;   // 手柄地址(回传状态)
RF24 radio(PIN_RF_CE, PIN_RF_CSN);
Adafruit_SSD1306 oled(128, 64, &Wire, -1);
Servo servoScoop, servoHook;
/* ---------------- 4. 航点任务表 ---------------- */
// 场地坐标: 原点=出发点(起始线后中点), X=前进方向(进入场地), Y=左侧, 单位 m
// 与《图1 场地布局与行驶路线》一一对应
enum Action : uint8_t { A_NONE, A_HOOK_WAIT, A_PICK3, A_WAIT_UNLOAD,
                        A_RELEASE, A_DONE };
struct Waypoint { float x, y; float v; Action act; };
const Waypoint MISSION[] = {
  // ① 越过起始线 → 进入购物区域
  { 1.34f,  0.00f, 0.45f, A_NONE },        // P1 (12ft,6ft)
  // ② 转向购物车区域自动挂钩
  { 2.01f, -1.07f, 0.40f, A_NONE },
  { 2.56f, -2.29f, V_APPROACH, A_HOOK_WAIT },  // 挂钩点(19.5ft,10ft)
  // ③ 回购物区域货架取货 3 件
  { 1.65f, -0.91f, 0.40f, A_NONE },
  { 1.34f,  0.30f, 0.30f, A_PICK3 },      // 货架取货点
  // ④ 驶向结账区
  { 2.10f,  1.68f, 0.40f, A_NONE },
  { 2.74f,  2.29f, V_APPROACH, A_WAIT_UNLOAD },  // 结账停靠(4.5ft,10.6ft)
  // ⑤ 结账后继续前进: 右移 → 上穿起始线平面 → 掉头
  { 1.71f,  0.91f, 0.40f, A_NONE },
  { 1.58f, -1.22f, 0.40f, A_NONE },
  { 1.34f, -2.29f, 0.35f, A_NONE },
  {-0.21f, -2.29f, 0.35f, A_NONE },       // 上穿起始线平面(购物车随行)
  { 1.10f, -2.29f, V_APPROACH, A_RELEASE },  // 掉头下穿→购物车过线瞬间释放
  // ⑥ 空载返回起始线后
  { 0.12f, -1.22f, 0.40f, A_NONE },
  { 0.00f,  0.00f, 0.30f, A_DONE }        // 回到出发点
};
const int N_WP = sizeof(MISSION) / sizeof(MISSION[0]);
/* ---------------- 5. 全局状态 ---------------- */
volatile long encL = 0, encR = 0;
float x = 0, y = 0;              // 里程计位置 m
float theta = 0;                 // 融合航向 rad
float gyroZ = 0;                 // 陀螺仪角速度 rad/s
float gyroOffset = 0;
int  wpIndex = 0;
bool autoMode = true;
bool cartReleased = false;
bool unloadConfirmed = false;
int  pickedCount = 0;
unsigned long lastRadioMs = 0;
uint8_t lastBtn = 0;
RadioCmd rxCmd;
/* ---------------- 6. 编码器中断 ---------------- */
void isrEncL() { encL += (motorDirL >= 0) ? 1 : -1; }
void isrEncR() { encR += (motorDirR >= 0) ? 1 : -1; }
/* ---------------- 7. MPU6050 最小驱动 ---------------- */
#define MPU_ADDR 0x68
void mpuInit() {
  Wire.begin();
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x6B); Wire.write(0x00);          // 唤醒
  Wire.endTransmission(true);
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x1B); Wire.write(0x00);          // ±250°/s
  Wire.endTransmission(true);
}
int16_t mpuReadGyroZ() {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x47); Wire.endTransmission(false);
  Wire.requestFrom(MPU_ADDR, 2, true);
  return (int16_t)((Wire.read() << 8) | Wire.read());
}
void gyroCalibrate() {                          // 静置 1s 采零漂
  long s = 0; const int N = 200;
  for (int i = 0; i < N; i++) { s += mpuReadGyroZ(); delay(5); }
  gyroOffset = (s / (float)N) / 131.0f;         // °/s → rad/s 前置
  gyroOffset = gyroOffset * DEG_TO_RAD;
}
/* ---------------- 8. 电机驱动 ---------------- */
int motorDirL = 0, motorDirR = 0;
void setMotors(int pwmL, int pwmR) {            // pwm ∈ [-255,255]
  pwmL = constrain(pwmL, -255, 255);
  pwmR = constrain(pwmR, -255, 255);
  motorDirL = (pwmL > 0) - (pwmL < 0) ? ((pwmL > 0) - (pwmL < 0)) : 0;
  motorDirR = (pwmR > 0) - (pwmR < 0) ? ((pwmR > 0) - (pwmR < 0)) : 0;
  motorDirL = (pwmL > 5) - (pwmL < -5);
  motorDirR = (pwmR > 5) - (pwmR < -5);
  digitalWrite(PIN_STBY, HIGH);
  digitalWrite(PIN_AIN1, pwmL >= 0); digitalWrite(PIN_AIN2, pwmL < 0);
  digitalWrite(PIN_BIN1, pwmR >= 0); digitalWrite(PIN_BIN2, pwmR < 0);
  analogWrite(PIN_PWMA, abs(pwmL));
  analogWrite(PIN_PWMB, abs(pwmR));
}
void stopMotors() {
  digitalWrite(PIN_AIN1, LOW); digitalWrite(PIN_AIN2, LOW);
  digitalWrite(PIN_BIN1, LOW); digitalWrite(PIN_BIN2, LOW);
  analogWrite(PIN_PWMA, 0);    analogWrite(PIN_PWMB, 0);
  motorDirL = motorDirR = 0;
}
/* ---------------- 9. 里程计 + 航向融合(50Hz) ---------------- */
void updateOdometry() {
  static long eL = 0, eR = 0;
  noInterrupts();
  long dL = encL - eL, dR = encR - eR;          // 本周期计数增量
  interrupts();
  eL = encL; eR = encR;
  float dPhiL = dL * TWO_PI / ENC_PPR;
  float dPhiR = dR * TWO_PI / ENC_PPR;
  float ds = (dPhiL + dPhiR) * 0.5f * WHEEL_DIA * 0.5f;   // 弧长
  float dTh = (dPhiR - dPhiL) * (WHEEL_DIA / TRACK_WIDTH) * 0.5f;
  // 航向: 陀螺仪为主(高频准), 里程计漂移校正(低频稳)
  theta += gyroZ * 0.02f;                        // 50Hz → 20ms
  theta = 0.98f * theta + 0.02f * (theta + dTh - theta); // 简易互补
  x += ds * cosf(theta);
  y += ds * sinf(theta);
}
/* ---------------- 10. 航点导航(纯跟踪转向) ---------------- */
// 返回: 是否已到达当前航点
bool navigateTo(const Waypoint &wp) {
  float dx = wp.x - x, dy = wp.y - y;
  float dist = sqrtf(dx * dx + dy * dy);
  float targetHead = atan2f(dy, dx);
  float headErr = targetHead - theta;
  while (headErr >  PI) headErr -= TWO_PI;
  while (headErr < -PI) headErr += TWO_PI;
  if (fabsf(headErr) > 0.55f) {                 // 大偏差 → 原地转向(差速优势)
    setMotors(-60 * sign(headErr), 60 * sign(headErr));
    return false;
  }
  if (dist < WPT_TOL) { stopMotors(); return true; }
  // 距离环: 越近越慢; 航向环: 比例+微分
  float vCmd = constrain(KP_DIST * dist, 0.10f, wp.v);
  float wCmd = constrain(2.2f * headErr, -W_MAX, W_MAX);
  // 差速混合: v ± w·W/2
  float vL = vCmd - wCmd * TRACK_WIDTH * 0.5f;
  float vR = vCmd + wCmd * TRACK_WIDTH * 0.5f;
  float k = 255.0f / V_MAX;
  setMotors((int)(vL * k), (int)(vR * k));
  return false;
}
/* ---------------- 11. 动作执行 ---------------- */
void actScoopCycle() {                           // 取货铲一次取放
  servoScoop.write(SCOOP_UP_DEG);  delay(400);   // 平铲推入商品托位
  servoScoop.write(SCOOP_DUMP_DEG); delay(700);  // 翻倒入车载料斗
  servoScoop.write(SCOOP_UP_DEG);  delay(400);
}
void actReleaseCart() {
  if (cartReleased) return;
  servoHook.write(HOOK_FREE_DEG);
  cartReleased = true;
  oledStatus("购物车已释放");
}
void oledStatus(const char *s) {
  oled.clearDisplay();
  oled.setTextSize(1); oled.setTextColor(SSD1306_WHITE);
  oled.setCursor(0, 0);  oled.print(F("ASTRO-MART EXPRESS"));
  oled.setCursor(0, 14); oled.print(F("MODE:")); oled.print(autoMode ? F("AUTO") : F("MANUAL"));
  oled.setCursor(0, 24); oled.print(F("WP:"));   oled.print(wpIndex + 1); oled.print(F("/")); oled.print(N_WP);
  oled.setCursor(0, 34); oled.print(F("POS:"));  oled.print(x, 2); oled.print(F(",")); oled.print(y, 2);
  oled.setCursor(0, 44); oled.print(F("HDG:"));  oled.print(theta * RAD_TO_DEG, 0); oled.print(F("deg"));
  oled.setCursor(0, 54); oled.print(s);
  oled.display();
}
/* ---------------- 12. 无线收发 ---------------- */
bool radioPoll() {
  RadioCmd c;
  if (radio.available()) {
    radio.read(&c, sizeof(c));
    if (c.magic != RF_MAGIC) return false;
    lastRadioMs = millis();
    // 按键沿检测
    if ((c.btn & BTN_MODE) && !(lastBtn & BTN_MODE)) { autoMode = !autoMode; }
    if ((c.btn & BTN_RELEASE) && !(lastBtn & BTN_RELEASE)) { actReleaseCart(); }
    if ((c.btn & BTN_UNLOAD_OK)) { unloadConfirmed = true; }
    lastBtn = c.btn;
    rxCmd = c;
    return true;
  }
  return false;
}
void manualDrive() {                             // 手动模式: 摇杆直接差速
  float jy = (rxCmd.ly - 128) / 127.0f;          // 前进
  float jx = (rxCmd.lx - 128) / 127.0f;          // 转向
  float vL = jy - 0.8f * jx, vR = jy + 0.8f * jx;
  setMotors((int)(vL * 200), (int)(vR * 200));
}
/* ---------------- 13. setup / loop ---------------- */
void setup() {
  pinMode(PIN_STBY, OUTPUT); stopMotors();
  pinMode(PIN_AIN1, OUTPUT); pinMode(PIN_AIN2, OUTPUT);
  pinMode(PIN_BIN1, OUTPUT); pinMode(PIN_BIN2, OUTPUT);
  pinMode(PIN_ENC_L, INPUT_PULLUP);
  pinMode(PIN_ENC_R, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_L), isrEncL, RISING);
  attachInterrupt(digitalPinToInterrupt(PIN_ENC_R), isrEncR, RISING);
  servoScoop.attach(PIN_SERVO_SCOOP);
  servoHook.attach(PIN_SERVO_HOOK);
  servoScoop.write(SCOOP_UP_DEG);
  servoHook.write(HOOK_LOCK_DEG);               // 出发前锁定拖钩
  oled.begin(SSD1306_SWITCHCAPVCC, 0x3C);
  oledStatus("系统自检...");
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);                // 场内近距, 低功耗防干扰
  radio.setDataRate(RF24_250KBPS);              // 慢速=远距=稳定
  radio.setChannel(108);
  radio.openReadingPipe(1, RF_PIPE_V);
  radio.openWritingPipe(RF_PIPE_R);
  radio.startListening();
  mpuInit();
  gyroCalibrate();                              // ★上电须静置1s
  oledStatus("就绪-等待开始");
}
void loop() {
  static unsigned long tNav = 0, tOled = 0;
  radioPoll();
  // —— 失联保护: 300ms 无指令 → 停车
  if (millis() - lastRadioMs > RADIO_TIMEOUT_MS && !autoMode) { stopMotors(); }
  // 手动模式最高优先级
  if (!autoMode) { manualDrive(); }
  else {
    if (millis() - tNav >= 20) {                // 50Hz 导航节拍
      tNav = millis();
      gyroZ = mpuReadGyroZ() / 131.0f * DEG_TO_RAD - gyroOffset;
      updateOdometry();
      if (wpIndex < N_WP) {
        const Waypoint &wp = MISSION[wpIndex];
        bool arrived = navigateTo(wp);
        if (arrived) {
          switch (wp.act) {
            case A_HOOK_WAIT:                    // 慢速顶进磁性导引钩窝
              oledStatus("挂钩确认中");
              delay(900);                        // 磁吸自锁行程
              break;
            case A_PICK3:                        // 逐件取货(含替代品确认)
              for (int i = 0; i < 3; i++) { actScoopCycle(); pickedCount++; }
              oledStatus("取货完成3/3");
              break;
            case A_WAIT_UNLOAD:                  // 等卸货装置完成信号
              while (!unloadConfirmed) { radioPoll(); stopMotors(); delay(20); }
              break;
            case A_RELEASE:                      // 购物车过线瞬间释放
              actReleaseCart();
              break;
            case A_DONE:
              stopMotors(); oledStatus("任务完成!");
              while (true) { radioPoll(); delay(50); }
            default: break;
          }
          wpIndex++;
        }
      } else stopMotors();
    }
  }
  if (millis() - tOled >= 300) { tOled = millis(); oledStatus("RUN"); }
}
int sign(float v) { return (v > 0) - (v < 0); }