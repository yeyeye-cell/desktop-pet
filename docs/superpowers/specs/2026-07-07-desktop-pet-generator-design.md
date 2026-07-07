# 桌面宠物生成器 — 设计文档

**日期:** 2026-07-07
**框架:** Qt 6 (C++17, Widgets, CMake + MSVC)
**类型:** 桌面应用

---

## 1. 项目概述

一个基于 Qt 的桌面宠物程序。宠物以透明悬浮窗形式漂浮在桌面最上层，支持全交互（拖拽、点击、跟随鼠标、随机走动、定时睡觉），内置预设角色并允许用户导入自定义精灵素材。

## 2. 核心功能

- **透明悬浮窗** — 无边框、置顶、半透明区域点击穿透
- **5 种动画状态** — 待机(IDLE)、走路(WALKING)、被点击(CLICKED)、开心跟随(HAPPY)、睡觉(SLEEPING)
- **全交互行为** — 拖拽移动、点击反应、鼠标靠近跟随、闲置超时睡觉、定时随机走动
- **预设角色** — 内置 3 个预设宠物（猫、狗、鸟），每种提供 5 套动画帧
- **自定义导入** — 支持 GIF/APNG 单文件导入和 PNG 序列帧文件夹导入，逐状态映射
- **右键菜单** — 切换宠物、导入素材、缩放调整、隐藏/退出
- **设置面板** — 预设选择网格、动画状态映射表、缩放滑块

## 3. 模块架构

```
src/
├── main.cpp                     # QApplication + 启动 PetController
├── PetController.h / .cpp       # 总控，组装各模块，管理生命周期
├── PetWindow.h / .cpp           # QWidget 透明置顶窗口，鼠标事件捕获
├── SpriteManager.h / .cpp       # 精灵集管理，加载预设/导入自定义
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
- `loadPresets()` — 扫描 `resources/presets/` 目录，加载内置精灵
- `importCustom(const QString& path)` — 导入外部素材文件/文件夹
- `getCurrent()` → `const SpriteData&`
- `switchTo(const QString& name)` — 切换到指定宠物
- 导入时自动检测格式：单文件 GIF/APNG 解析为帧序列，文件夹中 PNG 按文件名排序作为帧序列
- 容错：文件缺失/损坏时返回空帧列表，调用方回退到预设

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
- 导入区：5 行映射表（idle/walk/clicked/happy/sleep），每行一个文件选择按钮
- 缩放滑块：20%~200%
- 确定/取消按钮
- 发出 `petChanged(QString name)`, `spriteImported(SpriteData data)` 信号

### 4.7 PetController
- `init()` — 实例化所有模块，建立信号/槽连接，显示 PetWindow
- `switchPet(name)` — 通知 SpriteManager 切换，更新 AnimationEngine 的 SpriteData 引用
- `shutdown()` — 清理资源，保存配置
- 信号转发：
  - `BehaviorEngine::stateChanged` → `AnimationEngine::setState`
  - `AnimationEngine::frameChanged(QPixmap)` → `PetWindow::setPixmap` (QTimer 每帧触发信号)
  - `PetWindow::clicked/dragged/rightClicked` → `BehaviorEngine` 处理
  - `SettingsDialog::petChanged` → `switchPet()`

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
    │ signal: petChanged / spriteImported
    ▼
PetController → SpriteManager 切换/导入 → 更新 AnimationEngine 引用 → 刷新
```

## 7. 错误处理

| 场景 | 处理方式 |
|---|---|
| 导入文件格式不支持 | `QMessageBox::warning()` 提示，不崩溃 |
| GIF/PNG 损坏无法解析 | 跳过该状态，回退使用 idle 帧 |
| 已导入文件被用户删除 | 启动时检测路径有效性，缺失则回退默认预设 |
| 只设置了部分动画状态 | 未设置的状态全部回退使用 idle 动画 |
| 素材文件过大 (>10MB) | 警告但允许，帧加载时限制最大 128x128 |
| 配置文件损坏 | 删除损坏配置，重建默认 |
| 动画帧加载失败 | 跳过坏帧继续播放，不中断动画循环 |
| 窗口句柄异常 | 记录日志，尝试重建 PetWindow |

## 8. 测试策略

- **SpriteManager** (QTest) — 加载预设成功、导入合法文件、导入非法文件回退、缺失文件回退
- **AnimationEngine** (QTest) — 状态切换帧重置、帧索引推进、循环播放、空帧列表不崩溃
- **BehaviorEngine** (QTest) — 状态转移规则、优先级覆盖、定时器触发
- **手动验收** — 拖拽流畅度、右键菜单响应、设置面板完整流程、缩放效果、多状态动画切换

测试框架：Qt 自带 `QTest`，无额外依赖。

## 9. 技术约束

- **平台:** Windows（与桌面系统集成）
- **构建:** CMake + MSVC
- **Qt 版本:** Qt 6.x
- **依赖:** 仅 Qt 核心模块（`Qt::Widgets`, `Qt::Core`, `Qt::Gui`），如需 GIF 解码可能引入 `Qt::ImageFormats` 插件
- **GIF 解码:** Qt6 内置 GIF 支持已移除，引入 `Qt::ImageFormats` 插件即可恢复 `QMovie` 对 GIF 的解码能力。不作为可选方案——直接依赖此插件，保证 GIF 导入可用。PNG 序列帧作为主要导入方式同样支持
- **配置文件:** 使用 `QSettings` (.ini 格式)，存储当前宠物名、缩放比例、自定义导入路径
