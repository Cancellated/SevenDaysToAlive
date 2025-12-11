# Unreal Engine 委托宏差异解释

## 委托定义差异分析

在 `SDTAPlayer.h` 文件中，三个事件的定义如下：

```cpp
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnHealthChanged, float, HealthPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnStaminaChanged, float, StaminaPercent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnDeath);
```

## 差异原因

### 1. 委托宏类型不同

- `DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam`：表示**带一个参数**的动态多播委托
- `DECLARE_DYNAMIC_MULTICAST_DELEGATE`：表示**不带参数**的动态多播委托

### 2. 使用场景不同

#### 健康值/能量值变化事件（带参数）
当健康值或能量值发生变化时，我们需要向订阅者传递**具体的变化后的值**，以便：
- 更新UI界面（如血条、能量条的显示）
- 执行基于具体数值的逻辑（如低生命值警告）
- 记录统计数据

#### 死亡事件（不带参数）
死亡事件通常表示**状态变化**，订阅者不需要额外信息就能执行相应逻辑：
- 显示死亡动画或UI
- 触发重生流程
- 播放死亡音效
- 统计死亡次数

### 3. 设计合理性

这种设计遵循了**最小信息原则**：
- 只传递必要的信息
- 减少不必要的参数传递
- 提高代码可读性和维护性

## Unreal Engine 委托宏命名规则

Unreal Engine提供了一系列委托宏，命名规则为：

```
DECLARE_[DYNAMIC_][MULTICAST_]DELEGATE[_ParamCount](DelegateName, ParamType, ParamName, ...);
```

- `DYNAMIC_`：支持蓝图绑定
- `MULTICAST_`：支持多个订阅者
- `_ParamCount`：参数数量（如`_OneParam`、`_TwoParams`等）

## 常见委托宏类型

| 宏名称 | 参数数量 | 用途 |
|--------|----------|------|
| DECLARE_DYNAMIC_MULTICAST_DELEGATE | 0 | 不带参数的动态多播委托 |
| DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam | 1 | 带一个参数的动态多播委托 |
| DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams | 2 | 带两个参数的动态多播委托 |
| DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams | 3 | 带三个参数的动态多播委托 |

## 结论

健康值和能量值变化事件需要传递具体的数值信息，因此使用带参数的委托；而死亡事件只需要通知状态变化，不需要额外参数，因此使用不带参数的委托。这种设计既符合逻辑需求，也遵循了Unreal Engine的最佳实践。