# סקירת ארכיטקטורה

הפרויקט בנוי משני לוחות ESP32 נפרדים. כל לוח מריץ סקיצת Arduino עצמאית, עם אחריות שונה וברורה.

## עקרון מרכזי

אין למזג את שתי הסקיצות. ההפרדה בין לוח ה-UI ללוח החיישנים היא חלק מהארכיטקטורה של הפרויקט.

## לוח UI

סקיצה: `instrument_config/instrument_config.ino`

אחריות:

- הצגת ממשק משתמש במסך TFT מגע.
- שימוש בספריית `TFT9341Touch`.
- טעינת אייקוני כלי נגינה מקבצי `.h`.
- בחירת כלי, תו וצבע לכל מקש.
- קבלת טריגרים מלוח החיישנים.
- שליחת צבעים/הגדרות ללוח החיישנים.
- הפעלת `DFPlayer` להשמעת קבצי קול.

קבצים קשורים:

- `instrument_config/TFT9341Touch.h`
- `instrument_config/IconConfig.h`
- `instrument_config/icon_*.h`

## לוח חיישנים

סקיצה: `SENS_FIXED/SENS_FIXED.ino`

אחריות:

- קריאת חיישני ToF.
- תמיכה בחיישני `VL53L0X` ו-`VL6180X`.
- בחירת ערוץ דרך `TCA9548A`.
- כיול מרחקי בסיס וספי טריגר.
- שליחת אירועי נגינה ללוח ה-UI.
- קבלת צבעים מלוח ה-UI.
- הפעלת תאורת LED/NeoPixel לפי טריגרים.

## תקשורת בין הלוחות

התקשורת מתבצעת דרך מודול `HC-12` או קישור Serial תואם.

פקודות עיקריות:

| כיוון | פורמט | משמעות |
|---|---|---|
| Sensor -> UI | `T,<key>,<mm>` | טריגר ממקש 1-8 עם מרחק במילימטרים |
| UI -> Sensor | `C,<key>,<colorIdx>` | צבע LED עבור מקש 1-8 |
| Sensor -> UI | `BOOT,SENS` | לוח החיישנים הופעל |
| UI -> Sensor | `BOOT,UI` | לוח ה-UI הופעל |

## מיפוי כללי

| תחום | לוח אחראי | קובץ מרכזי |
|---|---|---|
| מסך מגע | UI | `instrument_config/instrument_config.ino` |
| אייקונים | UI | `instrument_config/icon_*.h` |
| נגינה דרך `DFPlayer` | UI | `instrument_config/instrument_config.ino` |
| חיישני ToF | Sensor | `SENS_FIXED/SENS_FIXED.ino` |
| כיול חיישנים | Sensor | `SENS_FIXED/SENS_FIXED.ino` |
| LED/NeoPixel | Sensor | `SENS_FIXED/SENS_FIXED.ino` |
| תקשורת `HC-12` | שני הלוחות | שתי הסקיצות |

## גבולות שינוי

- שינויי UI צריכים להישאר בתיקיית `instrument_config/`.
- שינויי חיישנים צריכים להישאר בתיקיית `SENS_FIXED/`.
- שינויי תקשורת צריכים להיבדק בשני הלוחות.
- שינוי במיפוי `DFPlayer` חייב לעדכן את טופס מיפוי כרטיס ה-SD.
- שינוי חיווט חייב לעדכן את טבלת החיבורים.
