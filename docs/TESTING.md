# 本地测试与验证口径

## 1. Python 替代规则测试

需要 Python 3.10 或更新版本；仅使用标准库。

```bash
python -m unittest discover -s tests -v
```

运行一次替代品决策示例：

```bash
python -m tools.substitution <<'JSON'
{
  "item": "milk",
  "category": "dairy",
  "out_of_stock": true,
  "budget_remaining": 4,
  "candidates": [
    {"id": "soy_milk", "category": "dairy", "price": 2.5},
    {"id": "juice", "category": "drink", "price": 1.2}
  ]
}
JSON
```

预期为 `substitute_selected`，候选 `soy_milk`。价格和候选数据仅为示例。

## 2. ESP32-S3 固件构建

固件：`firmware/esp32_s3/esp32_s3.ino`。

- Arduino IDE 2.x 安装 Espressif Arduino-ESP32 3.x 平台。
- 按实际开发板设置 ESP32S3 Dev Module、Flash/PSRAM 与 USB 上传选项。
- 编译、上传并记录完整构建日志、板型和平台版本。
- PCA9685 使用 Arduino core 自带 `Wire`，I²C 默认 GPIO1/2、地址 0x40、50Hz。
- 车轮离地时先运行 `help`、`status`；完成电平、使能和硬件急停检查后才可执行底盘 `arm`。
- 抓取臂先断开舵机电源或卸下连杆，确认 PCA9685 输出和舵机行程后再供电；执行 `armenable`、`servo shoulder 90`、`armoff` 等命令。
- `armoff` 仅关闭 PWM 信号，不是舵机电源隔离开关。

## 3. 新增固件接口测试步骤

1. 上电后 `status` 应显示底盘 disarmed；PCA9685 若存在应显示 ready，舵机输出应为 OFF。
2. PCA9685 缺失时运行 `armenable`，应明确报错，不应假装动作成功。
3. 检测到 PCA9685 后运行 `armenable`，三通道应输出 90° 中位命令；先无连杆/无负载测量脉宽。
4. 运行 `servo shoulder 170` 应拒绝；`servo shoulder 90` 在 arm enabled 后才允许。
5. 运行 `armoff` 后三个 PWM 通道应为 full-off；注意舵机电源仍需独立断电。
6. 手动转动编码器，确认左右计数变化方向与车轮定义一致；标定计数/转和 x2 计数口径。
7. 底盘命令仍受短时脉冲、超声波有效回波和驱动使能约束；无回波会阻止运动。

## 4. 测试边界

Python 模块只验证确定性替代规则，不实现货架感知或车辆控制。固件新增了编码器方向计数和 PCA9685 商品抓取臂接口，但仍**没有**自动导航、路径规划、自动返航、货架识别、缺货感知、购物助理批准流程、购物车自动释放闭环或比赛流程控制。

源码检查和 Python 单元测试不等于 Arduino 编译成功，也不等于传感器、电机、舵机、机械机构或整车通过验收。所有硬件测试必须记录真实结果；未测项目不得标记为通过。
