# טופס 07 - דוח קומפילציה והעלאה

## פרטי סביבת בדיקה

| שדה | ערך |
|---|---|
| תאריך | 2026-04-29 |
| בודק תוכנה | Codex |
| Arduino IDE / CLI גרסה | arduino-cli 1.4.1 |
| ESP32 Board Package גרסה | esp32:esp32 3.3.8 |
| מערכת הפעלה | Windows |
| לוח שנבחר ב-Arduino | ESP32 Dev Module (`esp32:esp32:esp32`) |

## ספריות מותקנות

| ספרייה | גרסה | נדרש עבור | סטטוס |
|---|---|---|---|
| `TFT9341Touch` / קובץ מקומי | local raw SPI touch wrapper | UI | נמצא |
| `Adafruit_GFX` | 1.12.6 | UI | נמצא |
| `Adafruit_ILI9341` | 1.6.3 | UI | נמצא |
| `Adafruit_STMPE610` | לא בשימוש בסקיצת UI לאחר התיקון | UI | לא נדרש |
| `SoftwareSerial` / `EspSoftwareSerial` | EspSoftwareSerial 8.1.0 | UI / DFPlayer | נמצא |
| `Adafruit_VL53L0X` | __________ | Sensor | לא מולא |
| `Adafruit_VL6180X` | __________ | Sensor | לא מולא |
| `Adafruit_NeoPixel` | __________ | Sensor | לא מולא |

## קומפילציה

| סקיצה | נתיב | תוצאה | הודעת שגיאה אם קיימת |
|---|---|---|---|
| UI | `instrument_config/instrument_config.ino` | עבר | אין |
| Sensor | `SENS_FIXED/SENS_FIXED.ino` | לא הורץ | לא נדרש במשימה זו |

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
| UI | 115200 | `[TFT] begin`, `[TOUCH] OK`, `[TOUCH] diag tcs=15 tirq=35 tirqLevel=...`, `[SCREEN] name=MAIN` | לא נבדק על חומרה בתיקון זה | לא הורץ |
| Sensor | 115200 | __________ | __________ | עבר / נכשל |

## סיכום

האם שתי הסקיצות מתקמפלות בנפרד: לא נבדק במשימה זו
האם שתי הסקיצות הועלו ללוחות המתאימים: לא
האם נדרש תיקון קוד: כן

פירוט:

בוצעה קומפילציה ל-`instrument_config` בלבד לאחר החלפת מימוש המגע המקומי ל-raw SPI.
פקודה: `arduino-cli compile --fqbn esp32:esp32:esp32 instrument_config`
תוצאה: עבר.

חתימת בודק תוכנה: Codex תאריך: 2026-04-29
