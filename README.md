# DeskPet MVP

DeskPet is a desktop AI pet prototype: the desktop app sends state and text commands over HTTP, and the ESP32 CYD renders the pet face and message on its TFT screen.

## Project Structure

```text
deskpet/
├── desktop-client/
│   ├── src/
│   │   ├── components/
│   │   ├── services/
│   │   ├── App.tsx
│   │   ├── main.tsx
│   │   └── styles.css
│   └── src-tauri/
└── esp32-firmware/
    ├── src/main.cpp
    └── platformio.ini
```

## Desktop Client

```powershell
cd desktop-client
npm install
npm run dev
```

Run as a Tauri desktop app:

```powershell
npm run tauri dev
```

## ESP32 Firmware

1. Open `esp32-firmware` with PlatformIO.
2. Copy `include/deskpet_config.example.h` to `include/deskpet_config.h`.
3. Set your WiFi SSID and password.
4. Upload to the ESP32-2432S028 board.

The firmware exposes:

- `GET /ping`
- `POST /text`
- `POST /state`

Supported states:

- `idle`
- `sleep`
- `coding`
- `error`
- `success`
- `happy`

## Notes

The default TFT_eSPI built-in font may not render every Unicode glyph or Chinese character on all setups. For production-quality Chinese text and expressive pet faces, add a UTF-8 capable font asset in a later stage.
