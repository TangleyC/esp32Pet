# Window State Monitor

DeskPet has two ways to sync the ESP32 sprite with the active Windows foreground window.

## Recommended: Tauri Client

Run the real desktop client:

```powershell
cd F:\test\esp32Pet\desktop-client
npm run tauri dev
```

Keep the "活动窗口联动" panel enabled. The client will read the current Windows foreground app and call:

```http
POST /state
```

Important:

- `npm run dev` only starts the browser preview. It cannot read the Windows active window.
- `npm run tauri dev` starts the desktop app. This is required for active-window sync.
- The Device IP field must match the ESP32 IP, for example `192.168.1.120`.

Built-in mappings:

- Code, Codex, Cursor, WebStorm, terminals -> `working`
- Browser pages for Codex, GitHub, Jenkins, PlatformIO, Tauri, React, ESP32 -> `thinking`
- General browser pages -> `fishing`
- WeChat, WXWork, QQ, Telegram, DingTalk -> `message`
- Clash Verge -> `cool`
- No match -> `online`

## Fallback: PowerShell Script

The old script is still useful for quick testing or when the Tauri client is not running.

```powershell
cd F:\test\esp32Pet
powershell -ExecutionPolicy Bypass -File .\scripts\window-state-monitor.ps1 -DeviceIp 192.168.1.120
```

Stop it with `Ctrl+C`.

Optional title mode:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\window-state-monitor.ps1 -DeviceIp 192.168.1.120 -SendWindowTitle
```

This is noisier because every title change triggers `/text`.

## Sprite States

The monitor uses state names that match the sprite assets:

```text
online, thinking, idea, happy, cool, sleep, working, busy, fishing,
message, like, sad, angry, confused, offline
```
