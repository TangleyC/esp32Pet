# DeskPet Sprites

The source sprite sheet has been split into individual 128x128 transparent PNG files.

## Output

Project asset copies:

```text
assets/sprites/128
assets/sprites/raw
assets/sprites/sprites.json
assets/sprites/sprite-preview.png
```

Firmware LittleFS-ready copies:

```text
esp32-firmware/data/sprites
```

## State Files

```text
online.png
thinking.png
idea.png
happy.png
cool.png
sleep.png
working.png
busy.png
fishing.png
message.png
like.png
sad.png
angry.png
confused.png
offline.png
```

These names match the states used by `scripts/window-state-monitor.ps1`.

## Next Step

Add PNG decoding in the firmware and map:

```text
/sprites/{state}.png
```

to the corresponding `/state` value.
