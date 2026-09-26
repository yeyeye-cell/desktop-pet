# 桌面宠物 — 设计文档

**日期:** 2026-07-07
**框架:** Qt 6 (C++17, Widgets, CMake + MSVC)
**类型:** 桌面应用

---

## 1. 项目概述

一个基于 Qt 的桌面宠物程序。宠物以透明悬浮窗形式漂浮在桌面最上层，支持拖拽、点击、跟随鼠标、随机走动和定时睡觉，并提供猫、柴犬和小蓝鸟三个内置角色。

## 2. 核心功能

- **透明悬浮窗** — 无边框、置顶、半透明区域点击穿透
- **5 种动画状态** — 待机(IDLE)、走路(WALKING)、被点击(CLICKED)、开心跟随(HAPPY)、睡觉(SLEEPING)
- **全交互行为** — 拖拽移动、点击反应、鼠标靠近跟随、闲置超时睡觉、定时随机走动
- **预设角色** — 内置 3 个预设宠物（猫、狗、鸟），每种提供 5 套动画帧
- **宠物预设** — 仅提供猫、柴犬和小蓝鸟三个内置角色
- **右键菜单** — 切换宠物、缩放调整、隐藏/退出
- **设置面板** — 预设选择和缩放滑块

## 3. 模块架构

```
src/
├── main.cpp                     # QApplication + 启动 PetController
├── PetController.h / .cpp       # 总控，组装各模块，管理生命周期
├── PetWindow.h / .cpp           # QWidget 透明置顶窗口，鼠标事件捕获
├── SpriteManager.h / .cpp       # 加载和切换内置宠物预设
├── AnimationEngine.h / .cpp     # 帧播放引擎，状态切换
├── BehaviorEngine.h / .cpp      # 行为调度 + 状态机
├── SettingsDialog.h / .cpp      # QDialog 设置面板
├── SpriteData.h                 # 纯数据模型 (struct)
└── resources/presets/           # 内置预设精灵素材
```

### 模块依赖关系

```
main.cpp
  └── PetController
        ├── PetWindow          (透明窗口 + 鼠标事件)
        ├── SpriteManager      (精灵加载/管理) → SpriteData
        ├── AnimationEngine    (帧播放) ← 依赖 SpriteData
        ├── BehaviorEngine     (行为调度 + 状态机)
        └── SettingsDialog     (设置面板) → SpriteManager
```

所有模块间通信通过 Qt 信号/槽，由 PetController 统一 `connect()`。

## 4. 各模块职责

### 4.1 SpriteData（数据模型）
纯 struct，无行为逻辑。

```cpp
// 行为状态（BehaviorEngine 使用）。注意 DRAGGED 不映射独立动画，
// 拖拽期间复用 Happy 动画帧。
enum class PetState { Idle, Walking, Clicked, Happy, Sleeping, Dragged };

struct SpriteData {
    QString name;                          // 宠物名称
    QMap<PetState, QVector<QPixmap>> frames; // 每种状态一组帧
    int frameRate = 100;                   // 逐帧间隔 (ms)
    qreal scale = 1.0;                     // 缩放比例
};
```

### 4.2 PetWindow
- 继承 `QWidget`，设置 `FramelessWindowHint | WindowStaysOnTopHint | WA_TranslucentBackground`
- 使用 `setMask()` 基于当前帧的非透明区域设置窗口掩码，透明像素点击穿透
- 重写 `mousePressEvent` / `mouseMoveEvent` / `mouseReleaseEvent` / `contextMenuEvent`
- 提供 `setPixmap(QPixmap)` 刷新显示内容
- 发出 `clicked()`, `dragged(QPoint delta)`, `rightClicked(QPoint)` 信号

### 4.3 SpriteManager
- `loadPresets()` — 从 `resources/presets/` 加载猫、柴犬和小蓝鸟
- `getCurrent()` → `const SpriteData&`
- `switchTo(const QString& name)` — 切换到指定宠物
- 不识别其他素材目录，避免额外宠物进入列表

### 4.4 AnimationEngine
- `setState(PetState state)` — 切换动画状态，重置帧索引
- `currentFrame()` → `QPixmap` — 返回当前应显示的帧
- 内部维护 `QTimer`，按 `SpriteData::frameRate` 定时推进帧索引
- 状态切换时立即切到新状态第一帧（无过渡动画）
- 所有帧循环播放

### 4.5 BehaviorEngine
- 维护当前 `PetState`
- 通过 `QTimer` 周期性检查：鼠标距离 (<200px → HAPPY)、闲置计时 (>30s → SLEEPING)
- 随机行走定时器 (5~15s 随机间隔，移动 50~200px 随机距离)
- 接收 PetWindow 的鼠标信号 → 判断点击/拖拽
- 发出 `stateChanged(PetState)` 信号
- 内部状态转移优先级：DRAGGED > CLICKED > HAPPY > SLEEPING > WALKING > IDLE

### 4.6 SettingsDialog
- 继承 `QDialog`，模态显示
- 预设选择区：横向排列的按钮组，显示宠物名称和预览
- 缩放滑块：20%~200%
- 确定/取消按钮
- 发出 `petChanged(int index)` 和 `scaleChanged(qreal factor)` 信号

### 4.7 PetController
- `init()` — 实例化所有模块，建立信号/槽连接，显示 PetWindow
- `switchPet(name)` — 通知 SpriteManager 切换，更新 AnimationEngine 的 SpriteData 引用
- `shutdown()` — 清理资源，保存配置
- 信号转发：
  - `BehaviorEngine::stateChanged` → `AnimationEngine::setState`
  - `AnimationEngine::frameChanged(QPixmap)` → `PetWindow::setPixmap` (QTimer 每帧触发信号)
  - `PetWindow::clicked/dragged/rightClicked` → `BehaviorEngine` 处理
  - `SettingsDialog::petChanged` → 切换当前内置宠物

## 5. 状态机

```
                    ┌──────────┐
         拖动宠物   │ DRAGGED  │  松开
       ┌──────────►│(跟随鼠标) │──────────┐
       │           └──────────┘          │
       │                                 ▼
  ┌────┴─────┐                    ┌──────────┐
  │   IDLE   │◄───────────────────│  HAPPY   │
  │  待机     │   鼠标离开/超时    │ 跟随鼠标  │
  └──┬──┬──┬─┘                    └──────────┘
     │  │  │                           ▲
     │  │  │  鼠标靠近 (<200px)         │
     │  │  └───────────────────────────┘
     │  │
     │  │  闲置 30 秒
     │  └──────────► ┌──────────┐
     │               │ SLEEPING │
     │   点击/靠近   │  睡觉     │
     │   唤醒        └────┬─────┘
     │       ◄───────────┘
     │
     │  点击宠物
     └──────────► ┌──────────┐
       动画结束   │ CLICKED  │
      ◄──────────│ 点击反应  │
                  └──────────┘
```

状态转移优先级：DRAGGED > CLICKED > HAPPY > SLEEPING > WALKING > IDLE

## 6. 运行时数据流

```
鼠标事件 (PetWindow)
    │ signals: clicked / dragged / rightClicked / mouseNear
    ▼
BehaviorEngine (状态机判断)
    │ signal: stateChanged(PetState)
    ▼
AnimationEngine (查 SpriteData.frames[state], QTimer 逐帧)
    │ QPixmap 通过定时或回调
    ▼
PetWindow.setPixmap() → update() → 用户看到
```

设置变更路径：
```
右键菜单 → SettingsDialog
    │ signal: petChanged / scaleChanged
    ▼
PetController → SpriteManager 切换内置宠物 → 更新 AnimationEngine → 刷新
```

## 7. 错误处理

| 场景 | 处理方式 |
|---|---|
| 内置预设缺失或损坏 | 跳过不可用预设，保留其他可用预设 |
| 动画状态缺少帧 | 动画引擎回退使用 idle 帧 |
| 配置文件损坏 | 删除损坏配置，重建默认 |
| 动画帧加载失败 | 跳过坏帧继续播放，不中断动画循环 |
| 窗口句柄异常 | 记录日志，尝试重建 PetWindow |

## 8. 测试策略

- **SpriteManager** (QTest) — 加载内置预设、忽略额外目录、切换宠物
- **AnimationEngine** (QTest) — 状态切换帧重置、帧索引推进、循环播放、空帧列表不崩溃
- **BehaviorEngine** (QTest) — 状态转移规则、优先级覆盖、定时器触发
- **手动验收** — 拖拽流畅度、右键菜单响应、设置面板完整流程、缩放效果、多状态动画切换

测试框架：Qt 自带 `QTest`，无额外依赖。

## 9. 技术约束

- **平台:** Windows（与桌面系统集成）
- **构建:** CMake + MSVC
- **Qt 版本:** Qt 6.x
- **依赖:** Qt `Core`、`Gui` 和 `Widgets`
- **配置文件:** 使用 `QSettings` 存储当前宠物名和缩放比例
