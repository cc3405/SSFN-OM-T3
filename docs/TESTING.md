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

固件路径：`firmware/esp32_s3/esp32_s3.ino`。

- Arduino IDE 2.x 安装 Espressif 的 Arduino-ESP32 3.x 平台。
- 通过 Library Manager 安装 `ESP32Servo`。
- 按购买的实际开发板设置 `ESP32S3 Dev Module`、Flash/PSRAM 和 USB 上传选项。
- 编译并上传；记录板型、平台版本、ESP32Servo 版本、构建日志和串口启动信息。
- 在电机电源断开或车轮离地时，先运行 `help`、`status`；完成电平、使能和急停检查后，才可运行 `arm` 与短时运动命令。
- 首次测试必须有可触及的硬件急停，HC-SR04 Echo 需降压至不超过 3.3V，舵机必须独立供电。

官方安装文档：<https://docs.espressif.com/projects/arduino-esp32/en/latest/getting_started.html>。

## 3. 测试边界

Python 模块只验证确定性替代规则，不实现货架感知或车辆控制。ESP32 固件当前是低风险 bring-up 骨架：串口短时运动、编码器计数、超声波近距停机、舵机动作。它**尚未实现完整自动导航、自动返航、货架识别、比赛流程闭环**。

文档/源码检查和 Python 单元测试不等于 Arduino 固件编译成功，也不等于传感器、电机、机械机构或整车通过验收。所有硬件测试都要记录真实结果；未测项目不得标为通过。
