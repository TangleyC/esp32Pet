# Window State Monitor

This helper watches the active Windows foreground window and sends DeskPet state updates to the ESP32.

## Run

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\window-state-monitor.ps1 -DeviceIp 192.168.1.120
```

Stop it with `Ctrl+C`.

## How It Works

The script reads the current foreground process and window title, matches it against `scripts/window-state-map.json`, then calls:

```http
POST /state
```

Example mappings:

- Code editors and terminals -> `working`
- Browser on Codex/GitHub/Jenkins pages -> `thinking`
- General browser windows -> `fishing`
- WeChat/enterprise chat apps -> `message`

## Optional Window Title Mode

You can also send the active window title as text:

```powershell
powershell -ExecutionPolicy Bypass -File .\scripts\window-state-monitor.ps1 -DeviceIp 192.168.1.120 -SendWindowTitle
```

This is noisier because every title change triggers `/text`.

## Sprite Integration

The monitor already uses state names that match the planned sprite sheet:

```text
online, thinking, idea, happy, cool, sleep, working, busy, fishing,
message, like, sad, angry, confused, offline
```

After the image sheet is split into individual sprites, the firmware can map these same states to images without changing the monitor script.
