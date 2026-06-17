# Setup On A New Computer Or Network

这份文档用于换电脑、换办公室、换 WiFi 后恢复 DeskPet 项目。

## 先判断你要做哪种恢复

### 只运行桌面端和窗口监控

需要：

```text
Node.js
项目代码
ESP32 当前 IP
```

不一定需要：

```text
PlatformIO
Rust
CH340 驱动
USB 连接
```

### 需要重新烧录 ESP32

需要：

```text
Node.js
Python
PlatformIO
Rust / Cargo
Visual Studio C++ Build Tools
CH340 驱动
USB-C 数据线
```

## 1. 拉取项目

```powershell
git clone https://github.com/TangleyC/esp32Pet.git
cd esp32Pet
```

如果已经有项目：

```powershell
cd F:\test\esp32Pet
git pull
```

## 2. 安装基础工具

### Node.js

安装 Node.js 22 或更新版本。

验证：

```powershell
node --version
npm --version
```

### Python

安装 Python 3.12 或更新版本。

验证：

```powershell
python --version
pip --version
```

### PlatformIO

```powershell
python -m pip install --upgrade platformio
```

验证：

```powershell
pio --version
platformio --version
```

### Rust / Cargo

安装 Rustup。

验证：

```powershell
cargo --version
rustc --version
```

### Visual Studio C++ Build Tools

Windows 上 Tauri/Rust 需要 MSVC linker。

安装：

```powershell
winget install --id Microsoft.VisualStudio.2022.BuildTools -e --accept-package-agreements --accept-source-agreements --silent --override "--wait --quiet --norestart --add Microsoft.VisualStudio.Workload.VCTools --includeRecommended"
```

验证：

```powershell
cargo check
```

运行目录：

```powershell
cd desktop-client\src-tauri
cargo check
```

## 3. 安装桌面端依赖

```powershell
cd desktop-client
npm install
```

验证：

```powershell
npm run build
```

启动开发服务：

```powershell
npm run dev
```

浏览器打开：

```text
http://127.0.0.1:1420/
```

## 4. 安装 ESP32 驱动

这块板子使用 CH340。

设备管理器里应该看到：

```text
USB-SERIAL CH340 (COMx)
```

如果看到：

```text
USB Serial
代码28
```

说明缺 CH340/CH341 驱动，需要安装：

```text
CH341SER
```

验证：

```powershell
pio device list
```

应该看到类似：

```text
COM3
----
USB-SERIAL CH340
```

如果 COM 口不是 `COM3`，修改：

```text
esp32-firmware/platformio.ini
```

里面的：

```ini
upload_port = COM3
monitor_port = COM3
```

改成实际端口。

## 5. 换 WiFi 后配置 ESP32

固件已经支持 AP 配网页面，不再需要把 WiFi 密码写进代码。

如果 ESP32 连接已保存 WiFi 失败超过 15 秒，会自动进入配网模式：

```text
AP 名称：DeskPet-Setup
AP IP：192.168.4.1
```

操作：

```text
1. 用电脑或手机连接 WiFi：DeskPet-Setup
2. 浏览器打开：http://192.168.4.1
3. 输入 ssid、password、deviceName
4. 点击保存
5. ESP32 写入 Preferences/NVS 后自动重启
```

注意：

```text
ESP32 通常只支持 2.4GHz WiFi
```

清除 WiFi 配置：

```text
http://当前ESP32_IP/reset-wifi
```

清除后设备会重启并重新进入配网流程。

## 6. 编译和烧录 ESP32

```powershell
cd esp32-firmware
pio run
```

烧录固件：

```powershell
pio run -t upload
```

上传图片文件系统：

```powershell
pio run -t uploadfs
```

如果换了图片资源，只需要：

```powershell
pio run -t uploadfs
```

如果换了 C++ 固件代码，需要：

```powershell
pio run -t upload
```

如果两者都换了，两个都执行。

## 7. 查看 ESP32 新 IP

烧录后打开串口监视：

```powershell
pio device monitor
```

找到：

```text
WiFi connected, IP: 192.168.x.x
DeskPet HTTP server started
```

记下这个 IP。

## 8. 测试 ESP32 HTTP

假设 IP 是：

```text
192.168.1.120
```

测试：

```powershell
Invoke-RestMethod -Uri "http://192.168.1.120/ping"
Invoke-RestMethod -Uri "http://192.168.1.120/status"
```

发送状态：

```powershell
Invoke-RestMethod -Uri "http://192.168.1.120/state" -Method Post -ContentType "application/json" -Body '{"state":"working"}'
```

发送中文建议用 Node，避免 PowerShell 编码问题：

```powershell
node -e "fetch('http://192.168.1.120/text',{method:'POST',headers:{'Content-Type':'application/json; charset=utf-8'},body:JSON.stringify({content:'主人，你回来啦'})}).then(r=>r.text()).then(console.log)"
```

## 9. 启动窗口监控

把 `DeviceIp` 换成当前 ESP32 IP：

```powershell
cd F:\test\esp32Pet
powershell -ExecutionPolicy Bypass -File .\scripts\window-state-monitor.ps1 -DeviceIp 192.168.1.120
```

如果换电脑后项目路径不同，先进入项目目录再运行。

日志：

```text
window-state-monitor.log
```

查看：

```powershell
Get-Content .\window-state-monitor.log
```

## 10. 新网络下最容易忘的事

### ESP32 IP 会变

每次换 WiFi，ESP32 IP 很可能变化。

处理：

```text
看串口 monitor
更新桌面端 Device IP
更新 window-state-monitor.ps1 启动参数
```

### WiFi 密码文件不会从 Git 下来

因为：

```text
esp32-firmware/include/deskpet_config.h
```

被 `.gitignore` 忽略。

所以新电脑上必须自己创建。

### 图片资源在 Git 里，板子里不一定有

电脑项目里有：

```text
esp32-firmware/data/sprites
```

但新板子或擦除后，需要：

```powershell
pio run -t uploadfs
```

### PowerShell 脚本权限

如果不能运行脚本，用：

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\window-state-monitor.ps1 -DeviceIp 192.168.1.120
```

## 11. 最短恢复流程

如果只想最快跑起来：

```powershell
git pull
cd desktop-client
npm install
npm run dev
```

打开：

```text
http://127.0.0.1:1420/
```

填 ESP32 IP，测试连接。

窗口监控：

```powershell
cd ..
powershell -ExecutionPolicy Bypass -File .\scripts\window-state-monitor.ps1 -DeviceIp 当前ESP32_IP
```

如果 ESP32 换 WiFi：

```text
连接 DeskPet-Setup
打开 192.168.4.1
保存新 WiFi
重启后查看新 IP
```

## 12. 未来要优化

为了减少换网络/换电脑麻烦，后续建议做：

```text
ESP32 配网页面
桌面端自动发现设备
资源 HTTP 上传
窗口监控集成进 Tauri
SD 卡动画资源读取

```

DeskPet Roadmap v0.1

[x] ESP32驱动
[x] 联网
[x] 网页控制
[x] 活动窗口识别
[x] 小人状态切换

[ ] 配网页面
[ ] 模式系统
[ ] 动画系统
[ ] SD卡资源包
[ ] Tauri客户端
[ ] 天气插件
[ ] 待办插件
