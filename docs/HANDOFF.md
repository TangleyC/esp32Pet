# DeskPet Handoff

如果换电脑或换 WiFi，先读：

```text
docs/SETUP-NEW-MACHINE.md
```

这份文档用于在家里、办公室或换电脑后快速恢复上下文。先读这里，再决定下一步做什么。

## 当前目标

打造一个桌面 AI 显示终端：

```text
Desktop Client / Window Monitor
        ↓ HTTP
ESP32-2432S028 (CYD)
        ↓
TFT 显示状态、文字、像素表情、后续动画
```

PC 负责状态判断、AI、窗口监控和指令发送。ESP32 负责接收指令、显示文字、播放状态动画。

## 当前硬件状态

设备：

```text
ESP32-2432S028 / Cheap Yellow Display
```

串口：

```text
COM3
USB-SERIAL CH340
VID_1A86 / PID_7523
```

网络：

```text
ESP32 IP: 192.168.1.120
```

如果 IP 变化，先看串口日志或路由器设备列表，再更新桌面端和脚本里的 `DeviceIp`。

## 已完成

### 环境

已安装：

```text
Node.js / npm
Rust / Cargo
Python 3.12
PlatformIO Core
Visual Studio C++ Build Tools
CH340 驱动
```

### ESP32 固件

已实现：

```text
WiFi 连接
TFT 屏幕初始化
HTTP Server
GET /ping
GET /status
POST /text
POST /state
LittleFS 图片文件系统
PNGdec 图片解码
U8g2 中文字体渲染
状态 -> PNG 图片显示
```

当前图片资源已经烧入 ESP32 LittleFS：

```text
/sprites/online.png
/sprites/thinking.png
/sprites/idea.png
/sprites/happy.png
/sprites/cool.png
/sprites/sleep.png
/sprites/working.png
/sprites/busy.png
/sprites/fishing.png
/sprites/message.png
/sprites/like.png
/sprites/sad.png
/sprites/angry.png
/sprites/confused.png
/sprites/offline.png
```

### 桌面端

已实现：

```text
设备 IP 配置
测试连接
发送文字
状态切换
```

### 窗口监控

已实现脚本：

```text
scripts/window-state-monitor.ps1
scripts/window-state-map.json
```

它会根据当前活动窗口发送状态：

```text
Codex / WebStorm / VS Code / 终端 -> working
浏览器开发相关页面 -> thinking
普通浏览器 -> fishing
微信 / 企业微信 / QQ 等 -> message
默认 -> online
```

## 常用命令

### 编译固件

```powershell
cd F:\test\esp32Pet\esp32-firmware
pio run
```

### 烧录固件

```powershell
cd F:\test\esp32Pet\esp32-firmware
pio run -t upload
```

### 上传图片文件系统

当 `esp32-firmware/data/sprites` 里的图片变化后，运行：

```powershell
cd F:\test\esp32Pet\esp32-firmware
pio run -t uploadfs
```

### 串口监视

```powershell
cd F:\test\esp32Pet\esp32-firmware
pio device monitor
```

### 启动桌面端网页开发服务

```powershell
cd F:\test\esp32Pet\desktop-client
npm run dev
```

浏览器打开：

```text
http://127.0.0.1:1420/
```

### 启动窗口监控

```powershell
cd F:\test\esp32Pet
powershell -ExecutionPolicy Bypass -File .\scripts\window-state-monitor.ps1 -DeviceIp 192.168.1.120
```

停止：

```text
Ctrl+C
```

## HTTP 测试

### Ping

```powershell
Invoke-RestMethod -Uri "http://192.168.1.120/ping"
```

### Status

```powershell
Invoke-RestMethod -Uri "http://192.168.1.120/status"
```

### Send Text

PowerShell 发中文容易有编码坑，建议用 Node：

```powershell
node -e "fetch('http://192.168.1.120/text',{method:'POST',headers:{'Content-Type':'application/json; charset=utf-8'},body:JSON.stringify({content:'主人，你回来啦'})}).then(r=>r.text()).then(console.log)"
```

### Change State

```powershell
Invoke-RestMethod -Uri "http://192.168.1.120/state" -Method Post -ContentType "application/json" -Body '{"state":"working"}'
```

## 当前素材

源图：

```text
assets/source/pet-sprite-sheet.png
```

拆分后的开发素材：

```text
assets/sprites/raw
assets/sprites/128
assets/sprites/sprites.json
assets/sprites/sprite-preview.png
```

固件文件系统素材：

```text
esp32-firmware/data/sprites
```

状态名必须和文件名一致：

```text
working -> /sprites/working.png
message -> /sprites/message.png
```

## 当前痛点

### 1. 换图片需要 uploadfs

现在图片已经存进 ESP32 LittleFS。更换图片后需要：

```powershell
pio run -t uploadfs
```

这比每次刷固件轻一点，但仍然需要 USB 烧录。

### 2. 动效还没做

当前每个状态只显示一张 PNG。目标是每个状态显示帧动画。

## 下一阶段建议

### 阶段 A：稳定当前静态图片系统

要做：

```text
确认每个状态图片能正常显示
优化图片位置和大小
确认中文状态文字不挡图片
修正窗口监控规则
```

验收：

```text
切到微信 -> message 图片
切到 Codex/WebStorm -> working 图片
切到普通浏览器 -> fishing 图片
```

### 阶段 B：增加帧序列动画

推荐格式：

```text
每帧 96x96 或 128x128
PNG
4-8 fps
每个状态 4-12 帧
```

目录结构建议：

```text
sprites_anim/
  working/
    000.png
    001.png
    002.png
  message/
    000.png
    001.png
```

固件逻辑：

```text
/state=working
↓
加载 sprites_anim/working/*.png
↓
每 150ms 显示下一帧
↓
状态变化后切换动画目录
```

### 阶段 C：考虑 SD 卡

如果状态动画很多，建议接 SD 卡。

原因：

```text
LittleFS 适合少量默认图
SD 卡适合大量动画、皮肤包、帧序列
```

推荐卡：

```text
16GB 或 32GB microSD
FAT32
Class 10
正规品牌
```

建议架构：

```text
LittleFS:
  默认图
  config.json

SD 卡:
  skins/
  sprites/
  animations/
```

### 阶段 D：资源上传管理

未来做桌面端资源管理：

```text
上传图片
上传动画帧
切换皮肤
查看 ESP32 存储空间
删除旧资源
```

这样换图不需要 PlatformIO。

## 如果出了问题先查什么

### 板子没响应

```powershell
pio device list
```

确认有没有：

```text
USB-SERIAL CH340 (COM3)
```

### HTTP 不通

```powershell
Invoke-RestMethod -Uri "http://192.168.1.120/ping"
```

如果失败，检查：

```text
WiFi 是否连接
IP 是否变化
板子是否重启
```

### 切窗口不变

看脚本日志：

```powershell
Get-Content F:\test\esp32Pet\window-state-monitor.log
```

确认活动窗口进程名是否被映射到状态。

### 图片不显示

先确认文件系统已经上传：

```powershell
cd F:\test\esp32Pet\esp32-firmware
pio run -t uploadfs
```

再确认状态名和图片文件名一致。

## 当前最推荐的下一步

按顺序做：

```text
1. 继续测试窗口切换是否能稳定换图
2. 修 window-state-map.json 的进程映射
3. 调整图片显示布局
4. 做第一个状态动画 demo，比如 working 4 帧循环
5. 再决定是否接 SD 卡
```

不要一上来重构所有东西。先让一个状态动画跑起来，再扩展到全部状态。
