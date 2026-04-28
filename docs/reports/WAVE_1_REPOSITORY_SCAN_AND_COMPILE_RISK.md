# Wave 1 - Repository Scan & Compile Risk Analysis

תאריך: 2026-04-28  
מאגר: `nooshise/virtual-piano-esp32`  
ענף עבודה: `codex/wave-1-repository-scan`  
סוג שינוי: תיעוד בלבד

## 1. סיכום מבנה המאגר

מבנה המאגר הנוכחי:

```text
.
├── AGENTS.md
├── README.md
├── docs/
│   ├── ARCHITECTURE_OVERVIEW.md
│   ├── PROJECT_STATUS.md
│   ├── forms/
│   │   ├── README.md
│   │   ├── 01_project_summary_form.md
│   │   ├── 02_requirements_acceptance_form.md
│   │   ├── 03_components_bom_form.md
│   │   ├── 04_wiring_connection_table.md
│   │   ├── 05_sensor_calibration_log.md
│   │   ├── 06_dfplayer_sd_track_map.md
│   │   ├── 07_compile_upload_test_report.md
│   │   ├── 08_integration_test_checklist.md
│   │   ├── 09_bug_issue_report_template.md
│   │   ├── 10_change_request_approval.md
│   │   ├── 11_risk_mitigation_register.md
│   │   ├── 12_final_demo_checklist.md
│   │   └── 13_stakeholder_signoff.md
│   └── reports/
│       └── WAVE_1_REPOSITORY_SCAN_AND_COMPILE_RISK.md
├── instrument_config/
│   ├── instrument_config.ino
│   ├── TFT9341Touch.h
│   ├── IconConfig.h
│   ├── icon_accordion.h
│   ├── icon_drums.h
│   ├── icon_fan_fluit.h
│   ├── icon_guitar.h
│   ├── icon_oud.h
│   ├── icon_piano.h
│   ├── icon_trumpet.h
│   └── icon_violin.h
└── SENS_FIXED/
    └── SENS_FIXED.ino
```

המאגר בנוי בצורה תקינה עבור Arduino IDE: כל סקיצה נמצאת בתיקייה ששמה זהה לשם קובץ ה-`.ino` שלה.

## 2. אישור הפרדת שתי הסקיצות

נבדק ואושר:

- סקיצת לוח ה-UI נמצאת ב-`instrument_config/instrument_config.ino`.
- סקיצת לוח החיישנים נמצאת ב-`SENS_FIXED/SENS_FIXED.ino`.
- אין מיזוג בין שתי הסקיצות.
- אין קובץ `.ino` נוסף בתיקיית `instrument_config/`.
- אין קובץ `.ino` נוסף בתיקיית `SENS_FIXED/`.

יש להמשיך לפתוח, לקמפל ולהעלות כל סקיצה בנפרד.

## 3. ניתוח סיכוני קומפילציה - לוח UI

סקיצה: `instrument_config/instrument_config.ino`

### Arduino board/core נדרש

- נדרש ESP32 Arduino core.
- בחירת לוח ב-Arduino IDE צריכה להתאים ללוח הפיזי בפועל, למשל ESP32 Dev Module או דגם ESP32 אחר.
- סביבת הבדיקה צריכה לתמוך ב-`HardwareSerial`, `SoftwareSerial`, `PROGMEM`, `SPI`, ו-GPIO של ESP32.

### ספריות נדרשות

הסקיצה וקובץ `TFT9341Touch.h` דורשים:

| ספרייה / include | מקור | סיכון |
|---|---|---|
| `Arduino.h` | ESP32 core | נמוך |
| `SoftwareSerial.h` | ספרייה חיצונית או תאימות ESP32 | גבוה |
| `SPI.h` | ESP32 core | נמוך |
| `Adafruit_GFX.h` | Adafruit GFX Library | בינוני |
| `Adafruit_ILI9341.h` | Adafruit ILI9341 | בינוני |
| `Adafruit_STMPE610.h` | Adafruit STMPE610 | בינוני |
| `pgmspace.h` | ESP32 core | נמוך-בינוני |

### headers מקומיים

הקבצים המקומיים המשמשים את סקיצת ה-UI:

- `instrument_config/TFT9341Touch.h`
- `instrument_config/icon_piano.h`
- `instrument_config/icon_guitar.h`
- `instrument_config/icon_drums.h`
- `instrument_config/icon_trumpet.h`
- `instrument_config/icon_violin.h`
- `instrument_config/icon_accordion.h`
- `instrument_config/icon_oud.h`
- `instrument_config/icon_fan_fluit.h`

הקובץ `instrument_config/IconConfig.h` קיים אך לא מזוהה כרגע כקובץ שנכלל ישירות בסקיצה.

### קבצי אייקונים

כל קבצי האייקונים נמצאים בתיקיית `instrument_config/`, ליד הסקיצה. זה מיקום נכון עבור include מקומי ב-Arduino IDE.

| קובץ | סמל bitmap צפוי | מצב |
|---|---|---|
| `icon_piano.h` | `icon_piano_bitmap` | קיים |
| `icon_guitar.h` | `icon_guitar_bitmap` | קיים |
| `icon_drums.h` | `icon_drums_bitmap` | קיים |
| `icon_trumpet.h` | `icon_trumpet_bitmap` | קיים |
| `icon_violin.h` | `icon_violin_bitmap` | קיים |
| `icon_accordion.h` | `icon_accordion_bitmap` | קיים |
| `icon_oud.h` | `icon_oud_bitmap` | קיים |
| `icon_fan_fluit.h` | `icon_fan_fluit_bitmap` | קיים |

אין למחוק או להחליף את קבצי האייקונים.

### בעיות include אפשריות

- `SoftwareSerial.h` הוא הסיכון הגבוה ביותר בסקיצת ה-UI. ב-ESP32 בדרך כלל נדרש `EspSoftwareSerial` או ספרייה תואמת שמספקת header בשם `SoftwareSerial.h`.
- אם ספריות Adafruit אינן מותקנות, הקומפילציה תיכשל כבר ב-`TFT9341Touch.h`.
- קבצי האייקונים כוללים `<pgmspace.h>` ישירות. זה בדרך כלל מתאים ל-ESP32, אך בסביבות מסוימות ייתכן שתידרש תאימות include שונה. אין לשנות זאת לפני שמתקבלת שגיאת קומפילציה אמיתית.
- `TFT9341Touch.h` הוא header מקומי והוא נמצא במקום הנכון. אין להחליף אותו בספריית מסך אחרת.

### סיכוני תאימות ESP32

- `RF_RX_PIN` מוגדר כ-GPIO34. זה פין input-only, והוא מתאים ל-RX אך לא מתאים ל-TX.
- `DF_RX_DUMMY` מוגדר כ-GPIO39. זה פין input-only, ולכן מתאים כ-RX dummy בלבד.
- `TIRQ` מוגדר כ-GPIO35 דרך `tft9341touch lcd(5, 4, 15, 35)`. GPIO35 הוא input-only, ולכן מתאים ל-IRQ, אך הקוד מניח pull-up חיצוני/על הלוח.
- שימוש ב-8 מופעי `SoftwareSerial` במקביל על ESP32 הוא סיכון קומפילציה/תאימות וסיכון ריצה. גם אם הקומפילציה עוברת, יש לבדוק בפועל שהספרייה תומכת בשימוש הזה.
- הפינים `21` ו-`22` משמשים ב-UI כ-TX ל-DFPlayer. בלוחות ESP32 רבים הם גם פיני I2C ברירת מחדל, אך בסקיצת ה-UI לא זוהה שימוש ב-I2C. יש לוודא שאין חיווט חיצוני שמתנגש.

### סיכוני DFPlayer / SoftwareSerial

- השליטה ב-`DFPlayer` היא TX-only, ללא feedback מהמודול.
- אם `SoftwareSerial` ב-ESP32 לא תומך במבנה constructor שבו משתמש הקוד, תהיה שגיאת קומפילציה.
- אם הספרייה כן מתקמפלת, עדיין נדרש לבדוק בפועל השמעה דרך כל אחד מ-8 הפינים.
- יש לוודא שכרטיס ה-SD מכיל קבצים `001.mp3` עד `064.mp3` לפי המיפוי.
- שינוי במיפוי קבצי SD צריך לעדכן את `docs/forms/06_dfplayer_sd_track_map.md`.

## 4. ניתוח סיכוני קומפילציה - לוח חיישנים

סקיצה: `SENS_FIXED/SENS_FIXED.ino`

### Arduino board/core נדרש

- נדרש ESP32 Arduino core.
- נדרש לוח ESP32 התומך ב-`Wire.begin(SDA, SCL)`, `HardwareSerial`, ו-NeoPixel timing.
- בחירת board ב-Arduino IDE צריכה להתאים ללוח הפיזי.

### ספריות נדרשות

| ספרייה / include | מקור | סיכון |
|---|---|---|
| `Wire.h` | ESP32 core | נמוך |
| `Adafruit_VL53L0X.h` | Adafruit VL53L0X | בינוני-גבוה |
| `Adafruit_VL6180X.h` | Adafruit VL6180X | בינוני |
| `Adafruit_NeoPixel.h` | Adafruit NeoPixel | בינוני |

### סיכוני ספריות ToF

- הסקיצה משתמשת גם ב-`VL53L0X` וגם ב-`VL6180X`.
- כל החיישנים צפויים להיות בכתובת I2C דומה/זהה מאחורי `TCA9548A`, ולכן בחירת ערוץ תקינה היא קריטית.
- קריאת `vl53[ch].begin(0x29,false,&Wire)` תלויה בחתימת API של ספריית `Adafruit_VL53L0X` המותקנת. גרסה ישנה מדי של הספרייה עלולה לא לתמוך בכל הפרמטרים.
- השימוש ב-`stopRangeContinuous()` וב-`setMeasurementTimingBudgetMicroSeconds()` תלוי בתמיכת הספרייה בפונקציות אלה.
- הטיפוס `VL53L0X_RangingMeasurementData_t` מגיע מספריית Adafruit/ST API. אם הספרייה לא מותקנת או לא תואמת, זו תהיה נקודת כשל מוקדמת.
- עבור `VL6180X`, הפונקציות `begin()`, `readRange()` ו-`readRangeStatus()` צריכות להתאים לגרסת הספרייה.

### סיכוני HC-12 Serial

- הסקיצה משתמשת ב-`HardwareSerial RF(1)`.
- הפינים הם `RF_RX_PIN 34` ו-`RF_TX_PIN 13`.
- GPIO34 מתאים ל-RX בלבד כי הוא input-only.
- יש לוודא חיווט מוצלב בין TX/RX של שני מודולי `HC-12` או בין הלוחות.
- יש לוודא GND משותף וקצב `9600`.
- הסיכון העיקרי הוא חיווט/תצורה, לא בהכרח קומפילציה.

### סיכוני NeoPixel

- הסקיצה כוללת `Adafruit_NeoPixel`.
- מוגדר `NEO_PIN 25` ו-`NEO_COUNT 8`.
- קומפילציה דורשת ספריית Adafruit NeoPixel שתומכת ב-ESP32.
- בזמן ריצה יש לבדוק אספקת מתח מספקת, GND משותף וכיוון DATA.
- שימוש ב-NeoPixel עשוי להיות רגיש לתזמון, במיוחד אם נוספות בעתיד פעולות חוסמות או תקשורת נוספת.

## 5. בדיקת קומפילציה בפועל

Compile was not executed because Arduino CLI is not available in this environment.

### בדיקת זמינות שבוצעה

פקודה:

```powershell
Get-Command arduino-cli -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
```

תוצאה: לא נמצא נתיב ל-`arduino-cli`.

פקודה נוספת:

```powershell
arduino-cli version
```

שגיאה מדויקת:

```text
arduino-cli:
Line |
   2 |  arduino-cli version
     |  ~~~~~~~~~~~
     | The term 'arduino-cli' is not recognized as a name of a cmdlet, function, script file, or executable program.
     | Check the spelling of the name, or if a path was included, verify that the path is correct and try again.
```

### פקודות קומפילציה שלא הורצו

הפקודות הבאות לא הורצו כי `arduino-cli` אינו זמין. הן מוצעות כדוגמה בלבד לאחר התקנת Arduino CLI ו-ESP32 core:

```powershell
arduino-cli compile --fqbn esp32:esp32:esp32 instrument_config
arduino-cli compile --fqbn esp32:esp32:esp32 SENS_FIXED
```

יש להחליף את ה-`fqbn` אם הלוח הפיזי אינו `esp32:esp32:esp32`.

## 6. רשימת compile blockers סבירים לפי עדיפות

1. `arduino-cli` או Arduino IDE/ESP32 core לא זמינים בסביבת הבדיקה.
2. `SoftwareSerial.h` חסר או לא תואם ESP32 בסקיצת ה-UI.
3. ספריות Adafruit חסרות: `Adafruit_GFX`, `Adafruit_ILI9341`, `Adafruit_STMPE610`, `Adafruit_VL53L0X`, `Adafruit_VL6180X`, `Adafruit_NeoPixel`.
4. גרסת `Adafruit_VL53L0X` לא תומכת בחתימות או בפונקציות שבהן משתמשת סקיצת `SENS_FIXED`.
5. בעיית include של `<pgmspace.h>` בקבצי האייקונים בסביבת קומפילציה מסוימת.
6. חתימת constructor של `SoftwareSerial` שונה מהמצופה עבור `new SoftwareSerial(DF_RX_DUMMY, DF_TX_PINS[i])`.
7. בחירת board/FQBN לא נכונה ב-Arduino IDE או Arduino CLI.

## 7. משימות מומלצות ל-Wave 2, מהבטוח למסוכן

1. להתקין/לאמת Arduino IDE או Arduino CLI, ESP32 Arduino core, ולתעד גרסאות.
2. לבצע compile בלבד ל-`instrument_config/instrument_config.ino` ללא שינוי קוד ולשמור פלט מלא.
3. לבצע compile בלבד ל-`SENS_FIXED/SENS_FIXED.ino` ללא שינוי קוד ולשמור פלט מלא.
4. אם חסרות ספריות, להתקין את הספריות הנדרשות ולתעד ב-`docs/forms/07_compile_upload_test_report.md`.
5. אם יש כשל `SoftwareSerial.h`, להחליט האם מדובר בהתקנת ספרייה חסרה או בתיקון include מינימלי. אין להחליף ארכיטקטורת `DFPlayer` לפני אישור.
6. אם יש כשל בספריות ToF, לבדוק גרסאות ספרייה לפני שינוי קוד.
7. רק לאחר שיש פלט קומפילציה מדויק, לבצע תיקוני קוד קטנים ומבודדים.
8. לדחות שינויי לוגיקה, שינויי חיווט או שינויי פרוטוקול תקשורת עד לאחר קומפילציה נקייה.

## 8. קריטריוני קבלה ל-Wave 2

Wave 2 ייחשב מוכן כאשר:

- שתי הסקיצות נשארות נפרדות.
- אף קובץ אייקון `.h` לא נמחק ולא הוחלף.
- `TFT9341Touch` לא הוחלף.
- יש פלט קומפילציה מלא עבור סקיצת ה-UI.
- יש פלט קומפילציה מלא עבור סקיצת ה-Sensor.
- כל תיקון קוד, אם יידרש, יהיה קטן, ממוקד ומתועד.
- כל שינוי טכני יעדכן את הטופס המתאים תחת `docs/forms/`.
- אם קומפילציה עדיין נכשלת, קיימת רשימת שגיאות מדויקת וסדר טיפול מומלץ.

## 9. מה האדם צריך לבדוק ב-Arduino IDE

יש לבצע ידנית:

1. לפתוח את `instrument_config/instrument_config.ino` כסקיצה נפרדת.
2. לבחור לוח ESP32 מתאים.
3. לוודא שהספריות של מסך TFT, מגע ו-`SoftwareSerial` מותקנות.
4. להריץ Verify בלבד ולשמור את פלט השגיאות/אזהרות.
5. לפתוח את `SENS_FIXED/SENS_FIXED.ino` כסקיצה נפרדת.
6. לוודא שהספריות של `VL53L0X`, `VL6180X` ו-`Adafruit_NeoPixel` מותקנות.
7. להריץ Verify בלבד ולשמור את פלט השגיאות/אזהרות.
8. לאחר קומפילציה נקייה, להעלות כל סקיצה ללוח המתאים בלבד.
9. לבדוק Serial Monitor במהירות `115200`.
10. לבדוק שהתקשורת בין הלוחות עובדת רק לאחר ששני הלוחות עולים בנפרד.

## 10. טפסים שיש לעדכן לאחר בדיקות קומפילציה

לאחר בדיקות קומפילציה יש לעדכן:

- `docs/forms/07_compile_upload_test_report.md` - תוצאות compile/upload וסביבת בדיקה.
- `docs/forms/03_components_bom_form.md` - אם התגלתה ספרייה/רכיב חסר או גרסה מחייבת.
- `docs/forms/04_wiring_connection_table.md` - אם מתברר שחיווט פינים שונה מהקוד.
- `docs/forms/05_sensor_calibration_log.md` - לאחר בדיקות חיישנים בפועל.
- `docs/forms/06_dfplayer_sd_track_map.md` - לאחר בדיקת קבצי SD ו-`DFPlayer`.
- `docs/forms/08_integration_test_checklist.md` - לאחר בדיקה משולבת בין הלוחות.
- `docs/forms/09_bug_issue_report_template.md` - עבור כל שגיאת קומפילציה או תקלה שלא נפתרה מיד.
- `docs/forms/10_change_request_approval.md` - עבור שינוי ספרייה, חיווט, פרוטוקול או לוגיקה.
- `docs/forms/11_risk_mitigation_register.md` - אם מתגלה סיכון חדש.

## סיכום

Wave 1 הוא ניתוח בלבד. לא בוצעו תיקוני קוד, לא נערכו קבצי `.ino`, לא נערכו קבצי `.h`, לא נמחקו אייקונים, לא הוחלפה `TFT9341Touch`, ולא מוזגו הסקיצות.

הסיכון המרכזי לקומפילציה כרגע הוא זמינות סביבת Arduino מלאה, ובעיקר `SoftwareSerial.h` בסביבת ESP32. הסיכון המשני הוא גרסאות ספריות Adafruit, במיוחד עבור חיישני ToF.
