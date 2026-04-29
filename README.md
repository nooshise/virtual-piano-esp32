# Virtual Piano ESP32 Project

Two separate ESP32 boards run two separate Arduino sketches. Do not merge them.

## Sketches

- `instrument_config/instrument_config.ino` - UI, TFT touch, key configuration, RF trigger handling, and 8 DFPlayer Mini outputs.
- `SENS_FIXED/SENS_FIXED.ino` - TCA9548A mux, up to 8 mixed ToF sensors, one-shot trigger state, HC-12 RF, and NeoPixel feedback.

## Compile

Use the same external Arduino libraries folder used by the working touch examples:

```powershell
arduino-cli compile --fqbn esp32:esp32:uPesy_wroom --libraries "C:\Users\shash\OneDrive\מסמכים\Arduino\libraries" instrument_config
arduino-cli compile --fqbn esp32:esp32:uPesy_wroom --libraries "C:\Users\shash\OneDrive\מסמכים\Arduino\libraries" SENS_FIXED
```

Fallback board target if `uPesy_wroom` is unavailable:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 instrument_config
arduino-cli compile --fqbn esp32:esp32:esp32 SENS_FIXED
```

## Upload

Replace the COM ports with the two actual boards:

```powershell
arduino-cli upload --fqbn esp32:esp32:uPesy_wroom --port COM_UI instrument_config
arduino-cli upload --fqbn esp32:esp32:uPesy_wroom --port COM_SENSOR SENS_FIXED
```

## Required Libraries

- `tft_touch_ESP_UNO _V3` for `TFT9341Touch.h`
- `SoftwareSerial` / `EspSoftwareSerial`
- `Adafruit_VL53L0X`
- `Adafruit_VL6180X`
- `Adafruit_NeoPixel`

The UI sketch intentionally does not keep a local `instrument_config/TFT9341Touch.h`, because that shadows the known-working installed library.

## RF Protocol

- Sensor to UI: `T,<1..8>,<mm>`
- UI to Sensor: `C,<1..8>,<colorIdx>`
- Sensor boot: `BOOT,SENS`
- UI boot: `BOOT,UI`

## Track Map

`track = instrumentId * 8 + noteIndex + 1`

Files belong in the SD card root as `001.mp3` through `064.mp3`.

## Wiring Summary

- UI TFT: CS `GPIO5`, DC `GPIO4`
- UI touch: CS `GPIO15`, IRQ `GPIO35`
- Sensor I2C: SDA `GPIO21`, SCL `GPIO22`, TCA9548A `0x70`
- Sensor NeoPixel: `GPIO25`
- HC-12 UART on both sketches: RX `GPIO34`, TX `GPIO13`, `9600`

## Known Hardware Tests Still Required

- Confirm TFT display and touch coordinates on the UI board.
- Confirm all 8 DFPlayer Mini modules play the expected root SD tracks.
- Confirm sensor detection, calibration, one-shot trigger, and re-arm behavior on the sensor board.
- Confirm HC-12 traffic in both directions and LED color sync.
