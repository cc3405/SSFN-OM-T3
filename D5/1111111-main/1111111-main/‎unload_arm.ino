/*
 * ============================================================
 *  unload_arm.ino —— “星臂”三工位回转卸货装置
 *  航天未来科技组 · OM 长期题《OMER 的购物服务》
 * ------------------------------------------------------------
 *  规则依据：
 *    · 装置独立于车辆(独立底座+独立电池), 由队员持按钮盒操作
 *    · 仅按按钮(非人力搬运) → D6b 创造性得分高
 *  硬件：Arduino Nano + 12V 蜗轮蜗杆减速电机(回转臂)
 *        + 舵机(翻料托盘) + 三个工位限位开关 + 双按钮控制盒
 *  工位布置(俯视, 120° 均分):
 *    · 取料位(购物车侧) → 过渡位 → 放料位(结账台)
 *  一个完整循环 = 从购物车取 1 件 → 回转 → 翻落到结账台 → 复位
 *  三件商品 = 按 3 次 CYCLE 键
 * ============================================================
 */
#include <Servo.h>
/* ---------------- 可调参数 ---------------- */
const int ARM_PWM    = 180;      // 回转电机占空比(蜗轮自锁, 断电即停)
const int TRAY_GRIP  = 20;       // 托盘: 夹持位(度)
const int TRAY_DUMP  = 110;      // 托盘: 翻落位(度)
const int SWEEP_MS   = 2600;     // 单段 120° 回转时间(标定)
const int GRIP_TIME  = 600;      // 夹持稳定 ms
const int DUMP_TIME  = 800;      // 翻料稳定 ms
/* ---------------- 引脚 ---------------- */
const int PIN_MOTOR_FWD = 5;      // H桥: 正转(顺时针)
const int PIN_MOTOR_REV = 6;      // H桥: 反转(逆时针)
const int PIN_SERVO_TRAY = 9;     // 翻料托盘舵机
const int PIN_BTN_CYCLE = 2;      // 按钮: 执行一个卸料循环
const int PIN_BTN_RESET = 3;      // 按钮: 手动复位到取料位
const int PIN_LIM_PICK  = A0;     // 限位: 取料位(LOW=到位)
const int PIN_LIM_DROP  = A1;     // 限位: 放料位(LOW=到位)
const int PIN_LED       = 13;
Servo servoTray;
int itemsDone = 0;
/* ---------------- 基础动作 ---------------- */
void armStop() {
  digitalWrite(PIN_MOTOR_FWD, LOW);
  digitalWrite(PIN_MOTOR_REV, LOW);
}
void armCw() { armStop(); digitalWrite(PIN_MOTOR_FWD, HIGH); }   // 顺时针
void armCcw() { armStop(); digitalWrite(PIN_MOTOR_REV, HIGH); }  // 逆时针
// 顺时针转到指定限位开关触发, 带超时保护
void armTurnUntil(bool pickSide, unsigned long timeoutMs) {
  unsigned long t0 = millis();
  armCw();
  while (true) {
    bool hit = pickSide ? (digitalRead(PIN_LIM_PICK) == LOW)
                        : (digitalRead(PIN_LIM_DROP) == LOW);
    if (hit) break;
    if (millis() - t0 > timeoutMs) break;      // 超时保护, 防卡死
  }
  armStop();
}
/* ---------------- 一个完整卸料循环 ---------------- */
void oneUnloadCycle() {
  // 1) 确保臂在取料位(购物车侧)
  armTurnUntil(true, SWEEP_MS * 2);
  delay(200);
  // 2) 托盘夹取一件商品
  servoTray.write(TRAY_GRIP);
  delay(GRIP_TIME);
  // 3) 顺时针回转 120°×? 至放料位(结账台)
  armTurnUntil(false, SWEEP_MS * 2);
  delay(200);
  // 4) 托盘翻落, 商品落入结账台
  servoTray.write(TRAY_DUMP);
  delay(DUMP_TIME);
  servoTray.write(TRAY_GRIP);
  delay(300);
  itemsDone++;
  blinkLed(itemsDone);                        // 蜂鸣/LED 指示第几件
}
void blinkLed(int n) {
  for (int i = 0; i < n; i++) {
    digitalWrite(PIN_LED, HIGH); delay(120);
    digitalWrite(PIN_LED, LOW);  delay(120);
  }
}
/* ---------------- 主程序 ---------------- */
void setup() {
  pinMode(PIN_MOTOR_FWD, OUTPUT);
  pinMode(PIN_MOTOR_REV, OUTPUT);
  pinMode(PIN_BTN_CYCLE, INPUT_PULLUP);
  pinMode(PIN_BTN_RESET, INPUT_PULLUP);
  pinMode(PIN_LIM_PICK, INPUT_PULLUP);        // 限位开关常闭接地
  pinMode(PIN_LIM_DROP, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  armStop();
  servoTray.attach(PIN_SERVO_TRAY);
  servoTray.write(TRAY_GRIP);
  // 上电自检: 回转到取料位待命
  armTurnUntil(true, SWEEP_MS * 2);
}
void loop() {
  // CYCLE: 卸一件(共按3次)
  if (digitalRead(PIN_BTN_CYCLE) == LOW) {
    delay(30);                                 // 消抖
    if (digitalRead(PIN_BTN_CYCLE) == LOW) {
      while (digitalRead(PIN_BTN_CYCLE) == LOW) delay(10);  // 等松手
      oneUnloadCycle();
    }
  }
  // RESET: 手动回到取料位
  if (digitalRead(PIN_BTN_RESET) == LOW) {
    delay(30);
    if (digitalRead(PIN_BTN_RESET) == LOW) {
      armTurnUntil(true, SWEEP_MS * 2);
    }
  }
}