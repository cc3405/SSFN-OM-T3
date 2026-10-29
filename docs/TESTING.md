# 本地测试与验证口径

## 运行 Python 规则测试

需要 Python 3.10 或更新版本；本工具仅使用标准库。

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

## 测试边界

该 Python 模块只验证一个可解释、确定性的替代规则：同类别、价格不超预算、最低价优先、同价按 ID 排序。它**不**实现货架传感、摄像头识别、车辆控制、购物助理界面，也不代表比赛规则已经确认。

Arduino 固件应按每块控制板分别编译。实机测试需额外记录板卡、依赖库版本、接线、供电、传感器标定和现场结果。不要将 Python 单元测试结果描述为机器人系统测试结果。
