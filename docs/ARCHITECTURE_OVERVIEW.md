# Architecture Overview

The project uses two separate ESP32 boards and two separate Arduino sketches. The sketches must stay separate because they run on different hardware.

## UI / Sound Board

Sketch folder: `instrument_config/`

Responsibilities:

- Draw the TFT UI.
- Read STMPE610 touch through the installed `tft_touch_ESP_UNO _V3` library.
- Configure 8 virtual keys: instrument, note, and color.
- Receive `T,<key>,<mm>` trigger messages from the sensor board.
- Calculate tracks with `track = instrumentId * 8 + noteIndex + 1`.
- Play the matching DFPlayer Mini module.
- Send `C,<key>,<colorIdx>` LED color updates to the sensor board.

Current module map:

| File | Responsibility |
|---|---|
| `instrument_config.ino` | Arduino `setup()` / `loop()` entry point |
| `ProjectConfig.h` | UI board pins, counts, colors, RF and DF constants |
| `UIState.h` | screen enum and key configuration state |
| `Instruments.h` | instrument, note, and color names |
| `Icons.h` | references to the existing icon bitmap headers |
| `TrackMap.h` | SD root track formula |
| `DFPlayers.cpp/.h` | DFPlayer serial initialization and playback command frames |
| `RFProtocol.cpp/.h` | HC-12 receive/send protocol handling |
| `UIDraw.cpp/.h` | main screen, key config, instrument picker, color picker |
| `UITouch.cpp/.h` | simple touch read flow and navigation |
| `icon_*.h` | existing bitmap data, not edited by the refactor |

Touch wiring:

- TFT CS: `GPIO5`
- TFT DC: `GPIO4`
- Touch CS: `GPIO15`
- Touch IRQ: `GPIO35`

Touch calibration:

```cpp
lcd.setTouch(3780, 372, 489, 3811);
```

## Sensor Board

Sketch folder: `SENS_FIXED/`

Responsibilities:

- Read up to 8 ToF sensors through the TCA9548A I2C mux.
- Support VL53L0X and VL6180X detection.
- Calibrate baseline distance and trigger thresholds.
- Send one-shot trigger messages while preventing repeat triggers while a finger is held.
- Re-arm only after far exit.
- Receive LED color commands from the UI board.
- Drive NeoPixel feedback on `GPIO25`.

Current module map:

| File | Responsibility |
|---|---|
| `SENS_FIXED.ino` | Arduino `setup()` / `loop()` entry point |
| `SensorConfig.h` | sensor board pins, thresholds, timing constants |
| `TcaMux.cpp/.h` | I2C setup, TCA selection, ping, register read, bus recovery |
| `ToFSensors.cpp/.h` | sensor init, detection, calibration, reads, recovery |
| `TriggerState.cpp/.h` | near/far one-shot trigger state machine |
| `LedStrip.cpp/.h` | NeoPixel colors, trigger flash, calibration animation |
| `RFProtocol.cpp/.h` | HC-12 `T` send and `C` receive handling |
| `Diagnostics.cpp/.h` | readable sensor type names for Serial logs |

Sensor wiring:

- I2C SDA: `GPIO21`
- I2C SCL: `GPIO22`
- TCA9548A address: `0x70`
- NeoPixel strip: `GPIO25`
- HC-12 UART: RX `GPIO34`, TX `GPIO13`, baud `9600`

## RF Protocol

| Direction | Format | Meaning |
|---|---|---|
| Sensor -> UI | `T,<1..8>,<mm>` | Sensor/key trigger with distance |
| UI -> Sensor | `C,<1..8>,<colorIdx>` | Set LED color index for one key |
| Sensor -> UI | `BOOT,SENS` | Sensor board booted |
| UI -> Sensor | `BOOT,UI` | UI board booted |

## SD Track Layout

Files remain in the SD card root:

```text
001.mp3
002.mp3
...
064.mp3
```

Track ranges:

| Instrument | Tracks |
|---|---|
| Piano | 001-008 |
| Guitar | 009-016 |
| Drums | 017-024 |
| Trumpet | 025-032 |
| Violin | 033-040 |
| Accordion | 041-048 |
| Oud | 049-056 |
| Fan Flute | 057-064 |

## Compile Commands

```powershell
arduino-cli compile --fqbn esp32:esp32:uPesy_wroom --libraries "C:\Users\shash\OneDrive\מסמכים\Arduino\libraries" instrument_config
arduino-cli compile --fqbn esp32:esp32:uPesy_wroom --libraries "C:\Users\shash\OneDrive\מסמכים\Arduino\libraries" SENS_FIXED
```

## Current Known Issues

- The modular refactor has compile verification only. Physical TFT, touch, RF, DFPlayer, sensor, and LED tests still need to be run on the two ESP32 boards.
- The UI intentionally relies on the installed `TFT9341Touch.h`; a local header with the same name must not be reintroduced inside `instrument_config/`.
