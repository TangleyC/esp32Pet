# 2026-06-19 SD Card Migration Log

## Summary

This session moved DeskPet sprite loading toward SD-card-first storage and verified a 16GB microSD card on the ESP32-2432S028 board.

Final verified device state:

```text
ESP32 IP: 192.168.2.20
Serial port: COM3
SD card: mounted
Card size: 14911 MB
Filesystem size: 14896 MB
Sprites directory: /sprites exists
Sprite sync: copied=0 skipped=15
```

## Firmware Changes

- Added SD card support through Arduino `SD`.
- Added separate SD SPI bus configuration:

```text
SD_CS   = GPIO5
SD_MOSI = GPIO23
SD_MISO = GPIO19
SD_SCLK = GPIO18
```

- Added startup storage initialization:
  - Mount LittleFS.
  - Mount SD card.
  - Create `/sprites` on SD if needed.
  - Copy missing `/sprites/*.png` files from LittleFS to SD.
- Updated sprite rendering to prefer SD card files and fall back to LittleFS.
- Added `GET /storage` for storage diagnostics.
- Added storage status fields to `GET /status`.

## Desktop Dev Fixes During Session

- Changed Tauri dev URL from `http://localhost:1420` to `http://127.0.0.1:1420`.
- Bound Vite dev server to `127.0.0.1`.
- Ignored `src-tauri/target/**` in Vite file watching to avoid `EBUSY` on Rust DLL build artifacts.
- Rebuilt `package-lock.json` with Node.js 24 because the previous lock metadata caused missing dependencies.

## Upload Notes

Fast upload was unstable on this board. Upload at `921600` and `115200` baud entered download mode but stopped responding mid-flash.

The stable setting was:

```ini
upload_speed = 57600
```

Successful upload required manual boot mode:

```text
1. Hold BOOT.
2. Press RST once.
3. Keep holding BOOT until "Writing at..." appears.
4. Release BOOT.
```

For this board photo, the two silver buttons at the lower-right are:

```text
Upper button: BOOT / IO0
Lower button: RST / EN
```

## Verified Logs

Serial boot log after successful flash and SD insertion:

```text
LittleFS mounted
SD mounted: card=14911MB fs=14896MB used=0MB
Sprite sync complete: copied=0 skipped=15
Connecting to WiFi SSID: B6_4
WiFi connected, IP: 192.168.2.20
DeskPet HTTP server started
```

HTTP storage verification:

```json
{
  "ok": true,
  "littlefs": {
    "mounted": true
  },
  "sd": {
    "mounted": true,
    "cardSizeMb": 14911,
    "totalMb": 14896,
    "usedMb": 0,
    "spritesDir": true
  }
}
```

HTTP status verification:

```json
{
  "ok": true,
  "device": "DeskPet ESP32",
  "mode": "normal",
  "state": "online",
  "face": "(^_^)",
  "ip": "192.168.2.20",
  "storage": {
    "sd": true,
    "littlefs": true
  }
}
```

## Next Steps

- Update the desktop client default or saved device IP to `192.168.2.20` if the network keeps this address.
- Later asset work should write to SD `/sprites` or future animation directories instead of requiring LittleFS uploads.
- Keep LittleFS sprites as a fallback until SD-based asset management is fully stable.
