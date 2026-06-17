# WiFi Setup Mode

DeskPet uses ESP32 Preferences/NVS for WiFi configuration.

## Boot Behavior

```text
1. Read saved ssid/password/deviceName from NVS
2. Try to connect for 15 seconds
3. If connected, start normal DeskPet mode
4. If not connected, start AP setup mode
```

## Setup AP

```text
SSID: DeskPet-Setup
IP: 192.168.4.1
```

The TFT screen shows:

```text
Setup Mode
WiFi: DeskPet-Setup
Open: 192.168.4.1
```

## Setup Page

Connect to `DeskPet-Setup`, then open:

```text
http://192.168.4.1
```

Fields:

```text
ssid
password
deviceName
```

Saving writes values to Preferences/NVS and restarts the ESP32.

## Reset WiFi

When DeskPet is online:

```text
GET /reset-wifi
POST /reset-wifi
```

This clears the NVS namespace and restarts the ESP32.

## API Compatibility

Normal mode keeps:

```text
GET /ping
GET /status
POST /text
POST /message
POST /state
```
