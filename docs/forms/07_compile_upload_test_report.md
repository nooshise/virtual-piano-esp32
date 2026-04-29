# טופס 07 - דוח קומפילציה והעלאה

## פרטי סביבת בדיקה

| שדה | ערך |
|---|---|
| תאריך | 2026-04-29 |
| בודק תוכנה | Codex |
| Arduino IDE / CLI גרסה | arduino-cli 1.4.1 |
| ESP32 Board Package גרסה | esp32:esp32 3.3.8 |
| מערכת הפעלה | Windows / PowerShell |
| לוח שנבחר ב-Arduino | esp32:esp32:uPesy_wroom |

## ספריות מותקנות

| ספרייה | גרסה | נדרש עבור | סטטוס |
|---|---|---|---|
| `TFT9341Touch` / קובץ מקומי | installed library `tft_touch_ESP_UNO _V3`; local shadow removed from sketch folder | UI | עבר קומפילציה |
| `Adafruit_GFX` | __________ | UI | לא מולא |
| `Adafruit_ILI9341` | __________ | UI | לא מולא |
| `Adafruit_STMPE610` | __________ | UI | לא מולא |
| `SoftwareSerial` / `EspSoftwareSerial` | installed via library path | UI / DFPlayer | עבר קומפילציה |
| `Adafruit_VL53L0X` | installed via library path | Sensor | עבר קומפילציה |
| `Adafruit_VL6180X` | installed via library path | Sensor | עבר קומפילציה |
| `Adafruit_NeoPixel` | installed via library path | Sensor | עבר קומפילציה |

## קומפילציה

| סקיצה | נתיב | תוצאה | הודעת שגיאה אם קיימת |
|---|---|---|---|
| UI | `instrument_config/instrument_config.ino` | עבר | `401440 bytes` program, `24716 bytes` RAM |
| Sensor | `SENS_FIXED/SENS_FIXED.ino` | עבר | `357373 bytes` program, `29196 bytes` RAM |

## העלאה ללוחות

| סקיצה | Port | Baud | תוצאה | הערות |
|---|---|---:|---|---|
| UI | __________ | ___ | עבר / נכשל | __________ |
| Sensor | __________ | ___ | עבר / נכשל | __________ |

## Wave 3 - תיעוד upload מבוקר

Wave 2 כבר אישר קומפילציה תקינה לשתי הסקיצות. בסעיף זה יש לתעד העלאה לחומרה בלבד.

| לוח | סקיצה | COM port | כלי העלאה | תוצאה | קובץ לוג / צילום מסך |
|---|---|---|---|---|---|
| Sensor | `SENS_FIXED/SENS_FIXED.ino` | `COM__` | Arduino CLI / Arduino IDE | עבר / נכשל | __________ |
| UI | `instrument_config/instrument_config.ino` | `COM__` | Arduino CLI / Arduino IDE | עבר / נכשל | __________ |

פקודות CLI מתוכננות:

```powershell
arduino-cli upload --fqbn esp32:esp32:esp32 --port COM_SENSOR SENS_FIXED
arduino-cli upload --fqbn esp32:esp32:esp32 --port COM_UI instrument_config
```

הערות upload:

__________________________________________________________________

## Serial Monitor

| לוח | Baud | הודעת התחלה צפויה | נצפה בפועל | סטטוס |
|---|---:|---|---|---|
| UI | 115200 | __________ | __________ | עבר / נכשל |
| Sensor | 115200 | __________ | __________ | עבר / נכשל |

## סיכום

האם שתי הסקיצות מתקמפלות בנפרד: כן
האם שתי הסקיצות הועלו ללוחות המתאימים: לא, לא בוצעה בדיקת חומרה בסשן זה
האם נדרש תיקון קוד: כן, בוצע פיצול מודולרי והסרת shadow מקומי של `TFT9341Touch.h`

פירוט:

קומפילציה בוצעה עם:

```powershell
arduino-cli compile --fqbn esp32:esp32:uPesy_wroom --libraries "C:\Users\shash\OneDrive\מסמכים\Arduino\libraries" instrument_config
arduino-cli compile --fqbn esp32:esp32:uPesy_wroom --libraries "C:\Users\shash\OneDrive\מסמכים\Arduino\libraries" SENS_FIXED
```

חתימת בודק תוכנה: __________ תאריך: __________
