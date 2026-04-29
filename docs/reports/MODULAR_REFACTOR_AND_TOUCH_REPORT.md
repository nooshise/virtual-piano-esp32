# Modular Refactor and Touch Report

Date: 2026-04-29

## Stage 1 - Inventory

Repo root used for the PR work: `C:\Users\shash\Documents\VirtualPiano_ESP32_Project`

Current sketch owners:

- UI board: `instrument_config/`
- Sensor board: `SENS_FIXED/`
- Project reference docs: `docs/`

Compile risks found:

- `instrument_config/TFT9341Touch.h` existed in the sketch folder and shadowed the known-working installed Arduino library `tft_touch_ESP_UNO _V3`.
- The UI sketch depended on helper APIs from that local header, including `touchOk()`, `drainFifo()`, `xRaw`, and `clearButtons()`. These do not exist in the external working library.
- Both sketches were large single-file sketches, making RF, DFPlayer, touch, sensor, and LED behavior hard to isolate.

## Stage 2 - Structural Refactor

UI board modules:

- `ProjectConfig.h` - pins, counts, color constants, DF/RF settings
- `UIState.h` - screen enum and key configuration state
- `Instruments.h` - instrument, note, and color names
- `Icons.h` - icon bitmap array references only; icon data files unchanged
- `TrackMap.h` - `track = instrumentId * 8 + noteIndex + 1`
- `DFPlayers.*` - DFPlayer serial init and play commands
- `RFProtocol.*` - HC-12 RF parse/send logic
- `UIDraw.*` - main/config/instrument/color drawing and hit testing
- `UITouch.*` - simple touch read and screen navigation
- `instrument_config.ino` - Arduino setup/loop entry point

Sensor board modules:

- `SensorConfig.h` - pins, thresholds, timing constants
- `TcaMux.*` - I2C/TCA select, ping, register read, bus recovery
- `ToFSensors.*` - sensor detection, init, calibration, reads, recovery
- `TriggerState.*` - one-shot near/far trigger state machine
- `LedStrip.*` - NeoPixel palette, trigger flash, calibration animation
- `RFProtocol.*` - HC-12 `T` send and `C` receive handling
- `Diagnostics.*` - sensor type names for logs
- `SENS_FIXED.ino` - Arduino setup/loop entry point

## Stage 3 - Compile Results

Commands run:

```powershell
arduino-cli compile --fqbn esp32:esp32:uPesy_wroom --libraries "C:\Users\shash\OneDrive\מסמכים\Arduino\libraries" instrument_config
arduino-cli compile --fqbn esp32:esp32:uPesy_wroom --libraries "C:\Users\shash\OneDrive\מסמכים\Arduino\libraries" SENS_FIXED
```

Results:

- UI: passed. Sketch uses 401440 bytes program storage and 24716 bytes dynamic memory.
- Sensor: passed. Sketch uses 357373 bytes program storage and 29196 bytes dynamic memory.

## Stage 4 - Touch Integration

The local shadow header was moved to:

`docs/reference/TFT9341Touch_local_shadow_removed.h`

The UI sketch now includes the library with:

```cpp
#include <TFT9341Touch.h>
```

The setup keeps the known-working construction and calibration:

```cpp
tft9341touch lcd(5, 4, 15, 35);
lcd.begin();
lcd.setTouch(3780, 372, 489, 3811);
```

Touch read behavior is simple and matches the working examples:

```cpp
if (lcd.touched()) {
  delay(30);
  lcd.readTouch();
  x = lcd.xTouch;
  y = lcd.yTouch;
}
```

The UI no longer depends on local-only helper methods such as `touchOk()`, `drainFifo()`, or `xRaw`.

## Behavior Intentionally Preserved

- Two separate sketches remain separate.
- RF protocol remains `T,<1..8>,<mm>`, `C,<1..8>,<colorIdx>`, and `BOOT,SENS`.
- Track formula remains `instrumentId * 8 + noteIndex + 1`.
- SD files remain expected in the root as `001.mp3` through `064.mp3`.
- DFPlayer command logic remains root FAT-index playback command `0x03`.
- Sensor thresholds, TCA address, I2C pins, NeoPixel pin, and HC-12 pins remain unchanged.
- Icon bitmap data files were not edited.

## Hardware Tests Still Required

- Upload both sketches to their separate ESP32 boards.
- Verify TFT boot and touch coordinates on the real UI board.
- Verify screen navigation: main, config, instrument picker, color picker, back, note +/-
- Verify each DFPlayer module receives the expected track number.
- Verify sensor board detects all installed ToF sensors and calibrates with hands clear.
- Verify trigger one-shot behavior and far-exit re-arm.
- Verify HC-12 traffic and color sync in both directions.
