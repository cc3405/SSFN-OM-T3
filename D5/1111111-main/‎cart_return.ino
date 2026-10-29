/*
 * ============================================================
 *  cart_return.ino —— “回巢号”购物车 自行回位模块
 *  航天未来科技组 · OM 长期题《OMER 的购物服务》
 * ------------------------------------------------------------
 *  规则依据：
 *    · 拖行阶段 —— 电磁离合器分离, 购物车“随车辆行进并由车辆推进”
 *    · 释放后   —— 购物车“自行行进至购物车区域”(本题未禁止自带动力,
 *                  且 B6a 明确允许购物车包含商业生产的零件)
 *  硬件：Arduino Nano + 12V 减速电机(驱动右前轮) + 电磁离合器
 *        + 拖钩行程微动开关(检测脱钩) + 霍尔编码器(测距) + 待机LED
 *  动作时序(与《图4》完全一致):
 *    S1 拖行   —— 离合分离, 电机断电, 轮子自由滚动
 *    S2 脱钩   —— 拖钩 T 针离开锁爪 → 微动开关释放 → 触发
 *    S3 驱动   —— 离合吸合, 电机正转, 走编程距离(默认 1.8m)
 *    S4 停止   —— 到距断电, 离合保持, 毛毡阻尼 + 海绵保险杠停车
 * ============================================================
 */
/* ---------------- 可调参数 ---------------- */
const float RETURN_DIST = 1.80f;    // 回位行程 m(购物车区域距释放线约6′)
const float DRIVE_MS_PER_M = 4200;  // 标定: 电机驱动下每米毫秒数(后备计时)
const int   HALL_PPR = 24;          // 霍尔编码器每米脉冲数(现场标定)
const int   PWM_DRIVE = 200;        // 驱动占空比 0-255
const unsigned long ARM_DELAY_MS = 150;  // 脱钩消抖
/* ---------------- 引脚 ---------------- */
const int PIN_HOOK_SW = 2;     // 拖钩微动开关(挂上=闭合LOW, 脱钩=断开HIGH)
const int PIN_HALL    = 3;     // 霍尔测距(下降沿计数)
const int PIN_MOTOR   = 5;     // 电机 PWM (MOSFET)
const int PIN_CLUTCH  = 7;     // 电磁离合器(吸合=HIGH)
const int PIN_LED     = 13;    // 状态LED
const int PIN_ARM_BTN = 4;     // 人工布防按钮(调试用)
/* ---------------- 状态机 ---------------- */
enum CartState : uint8_t { CS_IDLE, CS_TOWING, CS_TRIGGERED, CS_DRIVING, CS_STOPPED };
CartState state = CS_IDLE;
volatile long hallPulses = 0;
void isrHall() { hallPulses++; }
void setup() {
  pinMode(PIN_HOOK_SW, INPUT_PULLUP);   // 挂钩闭合接地
  pinMode(PIN_HALL, INPUT_PULLUP);
  pinMode(PIN_MOTOR, OUTPUT);
  pinMode(PIN_CLUTCH, OUTPUT);
  pinMode(PIN_LED, OUTPUT);
  pinMode(PIN_ARM_BTN, INPUT_PULLUP);
  attachInterrupt(digitalPinToInterrupt(PIN_HALL), isrHall, FALLING);
  digitalWrite(PIN_MOTOR, LOW);         // 电机静止
  digitalWrite(PIN_CLUTCH, LOW);        // 离合分离(拖行自由)
  // —— 布防: 等待挂钩挂上(开关闭合) ——
  while (digitalRead(PIN_HOOK_SW) == HIGH) {
    digitalWrite(PIN_LED, !digitalRead(PIN_LED));  // 慢闪=待挂
    delay(300);
  }
  state = CS_TOWING;
  digitalWrite(PIN_LED, HIGH);          // 常亮=布防完成,进入拖行
}
void loop() {
  switch (state) {
    case CS_TOWING:                      // S1: 被车辆拖行, 全部输出关闭
      digitalWrite(PIN_MOTOR, LOW);
      digitalWrite(PIN_CLUTCH, LOW);
      // S2: 检测脱钩(开关断开, 消抖 150ms)
      if (digitalRead(PIN_HOOK_SW) == HIGH) {
        delay(ARM_DELAY_MS);
        if (digitalRead(PIN_HOOK_SW) == HIGH) {
          state = CS_TRIGGERED;
        }
      }
      break;
    case CS_TRIGGERED:                   // 脱钩确认 → 立即进入驱动
      hallPulses = 0;
      state = CS_DRIVING;
      break;
    case CS_DRIVING: {                   // S3: 离合吸合 + 电机驱动
      digitalWrite(PIN_CLUTCH, HIGH);    // 电磁离合器吸合
      delay(60);                         // 离合结合时间
      analogWrite(PIN_MOTOR, PWM_DRIVE);
      unsigned long t0 = millis();
      long targetPulse = (long)(RETURN_DIST * HALL_PPR);
      // 双保险停止条件: 编码器到距 或 超时
      while (state == CS_DRIVING) {
        if (hallPulses >= targetPulse) { state = CS_STOPPED; }
        if (millis() - t0 > (unsigned long)(RETURN_DIST * DRIVE_MS_PER_M) + 400) {
          state = CS_STOPPED;             // 霍尔失效 → 计时兜底
        }
      }
      break;
    }
    case CS_STOPPED:                     // S4: 断电 + 保持离合 → 阻尼停车
    default:
      analogWrite(PIN_MOTOR, 0);
      digitalWrite(PIN_MOTOR, LOW);
      digitalWrite(PIN_CLUTCH, HIGH);    // 保持离合=增大滚动阻尼
      digitalWrite(PIN_LED, LOW);        // 熄灯=任务完成
      while (true) { delay(1000); }      // 一次任务即结束
      break;
  }
}