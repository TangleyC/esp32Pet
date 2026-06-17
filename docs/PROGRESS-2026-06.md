# ESP32PET 项目进度记录（2026-06）

## 项目目标

打造一个桌面 AI 显示终端（DeskPet）。

最终架构：

```text
Desktop Client (Tauri + React)
        ↓
HTTP / WebSocket
        ↓
ESP32-2432S028
        ↓
TFT屏幕显示
```

ESP32 仅负责：

- 接收指令
- 显示文字
- 播放动画
- 展示状态

AI 逻辑全部运行在 PC 端。

## 当前硬件

设备：

```text
ESP32-2432S028
```

俗称：

```text
Cheap Yellow Display (CYD)
```

配置：

- ESP32
- 2.8 寸 TFT LCD
- 320x240
- 触摸屏
- WiFi
- USB-C
- CH340 USB 转串口

## 已完成

### 1. 板子到货

已收到硬件。

屏幕正常点亮。

### 2. 出厂程序验证

开机后显示 LVGL Demo。

功能正常：

- 屏幕显示正常
- 触摸正常
- WiFi 扫描正常

成功扫描到：

- DAZHICHANG-s5
- DAZHICHANG-s9

结论：

硬件无故障。

### 3. USB 驱动排查

初始状态：

```text
其他设备
└─ USB Serial
```

错误：

```text
代码28
未安装驱动
```

### 4. 确认 USB 芯片

硬件 ID：

```text
VID_1A86
PID_7523
```

确认型号：

```text
CH340
```

### 5. 安装驱动

安装：

```text
CH341SER
```

安装成功。

### 6. 设备识别成功

当前状态：

```text
端口(COM和LPT)

USB-SERIAL CH340 (COM3)
```

说明：

- 驱动正常
- USB 正常
- ESP32 连接正常
- 可进行烧录

当前串口：

```text
COM3
```

## 软件结构

当前工程：

```text
ESP32PET
│
├─ desktop-client
│  ├─ src
│  ├─ src-tauri
│  └─ React + Tauri
│
└─ esp32-firmware
   ├─ platformio.ini
   ├─ src
   └─ .pio
```

## 已确认内容

从目录观察：

```text
.pio/build/esp32-2432S028
```

已经存在。

说明：

PlatformIO 至少完成过一次 Build。

生成文件：

```text
firmware.bin
firmware.elf
bootloader.bin
partitions.bin
```

## MVP 目标

第一阶段：

不接入 AI。

不接入语音。

不做复杂动画。

仅实现：

```text
电脑
 ↓
发送文本
 ↓
ESP32
 ↓
显示文本
```

示例：

```json
{
  "text": "Hello DeskPet"
}
```

ESP32 显示：

```text
Hello DeskPet
```

## 第二阶段

增加状态同步：

状态：

```text
idle
thinking
coding
done
error
```

显示：

```text
[Thinking...]

Codex正在思考
```

## 第三阶段

增加简单角色动画。

例如：

```text
(•_•)

在线
```

```text
(-_-)

休眠
```

```text
(⊙_⊙)

收到消息
```

## 最终目标

打造一个桌面 AI 终端：

功能包括：

- 状态显示
- 消息显示
- 动画显示
- 本地联网
- 与 Codex 联动
- 与 Tauri 桌面客户端联动

作为个人 AI 助手实体化载体。

## 当前项目状态

```text
硬件：90%
驱动：100%
USB通信：100%
ESP32识别：100%
COM端口：COM3
Build：已成功
Upload：已成功
屏幕自定义程序：已烧录
桌面客户端：开发中
```

下一步：

```text
使用桌面客户端连接 ESP32 IP 并验证文本/状态控制。
```

## 2026-06-17 烧录验证

PlatformIO Upload 已验证成功。

当前结果：

```text
串口：COM3
ESP32 IP：192.168.1.120
HTTP 服务：已启动
/ping：成功
/text：成功
/state：成功
```
