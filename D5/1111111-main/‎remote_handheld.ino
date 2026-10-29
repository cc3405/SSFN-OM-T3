/*
 * ============================================================
 *  remote_handheld.ino —— 遥控手柄程序
 *  航天未来科技组 · “星驰一号” 配套 2.4G 双摇杆手柄
 * ------------------------------------------------------------
 *  硬件：Arduino Nano + nRF24L01 + 双轴摇杆×1(或×2) + 按键×4 + 状态LED
 *  功能：
 *    · 左摇杆 = 前进/转向(手动模式)
 *    · 按键A = 自动/手动模式切换
 *    · 按键B = 释放购物车(应急人工触发,与自动释放并联)
 *    · 按键C = 取货铲翻转(手动辅助)
 *    · 按键D = 卸货完成确认(通知车辆进入送回阶段)
 *    · 0.1s 心跳, 车辆 300ms 收不到即自动停车(安全联锁)
 * ============================================================
 */
#include <SPI.h>
#include <RF24.h>
const int PIN_RF_CE = 8, PIN_RF_CSN = 10;
const int PIN_JOY_X = A0, PIN_JOY_Y = A1;
const int PIN_BTN_A = 2, PIN_BTN_B = 3, PIN_BTN_C = 4, PIN_BTN_D = 5;
const int PIN_LED   = 6;
const uint8_t RF_MAGIC = 0xA5;
struct RadioCmd { uint8_t magic, lx, ly, btn, seq; };
const uint64_t RF_PIPE_V = 0xF0F0F0F0E1LL;   // 发往车辆
const uint64_t RF_PIPE_R = 0xF0F0F0F0D2LL;   // 接收车辆回传
RF24 radio(PIN_RF_CE, PIN_RF_CSN);
uint8_t seq = 0;
void setup() {
  pinMode(PIN_BTN_A, INPUT_PULLUP);
  pinMode(PIN_BTN_B, INPUT_PULLUP);
  pinMode(PIN_BTN_C, INPUT_PULLUP);
  pinMode(PIN_BTN_D, INPUT_PULLUP);
  pinMode(PIN_LED, OUTPUT);
  radio.begin();
  radio.setPALevel(RF24_PA_LOW);
  radio.setDataRate(RF24_250KBPS);
  radio.setChannel(108);                      // 与车辆一致
  radio.openWritingPipe(RF_PIPE_V);
  radio.openReadingPipe(1, RF_PIPE_R);
  radio.stopListening();
  // 摇杆中位自校准(上电保持摇杆松开 1s)
  delay(1000);
}
void loop() {
  RadioCmd c;
  c.magic = RF_MAGIC;
  // —— 摇杆采样 + 死区处理 ——
  int jx = analogRead(PIN_JOY_X) >> 2;        // 0..255
  int jy = analogRead(PIN_JOY_Y) >> 2;
  if (abs(jx - 128) < 12) jx = 128;           // 死区, 防漂移
  if (abs(jy - 128) < 12) jy = 128;
  c.lx = (uint8_t)constrain(jx, 0, 255);
  c.ly = (uint8_t)constrain(jy, 0, 255);
  // —— 按键(低有效) ——
  uint8_t btn = 0;
  if (digitalRead(PIN_BTN_A) == LOW) btn |= 0x01;   // 模式切换
  if (digitalRead(PIN_BTN_B) == LOW) btn |= 0x02;   // 释放购物车
  if (digitalRead(PIN_BTN_C) == LOW) btn |= 0x04;   // 铲翻转
  if (digitalRead(PIN_BTN_D) == LOW) btn |= 0x08;   // 卸货完成
  c.btn = btn;
  c.seq = seq++;
  // —— 100ms 心跳发送 ——
  radio.stopListening();
  bool ok = radio.write(&c, sizeof(c));
  radio.startListening();
  digitalWrite(PIN_LED, ok ? HIGH : LOW);     // LED=链路正常
  delay(100);
}