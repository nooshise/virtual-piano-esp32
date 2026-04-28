# Wave 3 - Controlled Upload & Hardware Verification Plan

תאריך: 2026-04-28  
מאגר: `nooshise/virtual-piano-esp32`  
ענף עבודה: `codex/wave-3-upload-hardware-plan`  
סוג שינוי: תיעוד בלבד

## 1. מטרת Wave 3

מטרת Wave 3 היא להעלות בצורה מבוקרת את שתי הסקיצות שעברו קומפילציה ב-Wave 2, ואז לבצע בדיקות חומרה ראשוניות ומסודרות.

Wave 2 עבר בהצלחה:

- `instrument_config/instrument_config.ino` - לוח UI - Compile result: PASS.
- `SENS_FIXED/SENS_FIXED.ino` - לוח Sensor - Compile result: PASS.

ב-Wave 3 אין לשנות קוד. אם מתגלות בעיות בזמן העלאה או בדיקות חומרה, יש לתעד אותן ולהעביר תיקונים ל-Wave הבא.

## 2. תנאים מקדימים לפני upload

לפני העלאה ללוחות יש לוודא:

- שתי הסקיצות מתקמפלות ללא שגיאות.
- ידוע איזה ESP32 הוא לוח Sensor ואיזה ESP32 הוא לוח UI.
- כל לוח מחובר בנפרד למחשב בזמן העלאה, או שה-COM ports מזוהים בוודאות.
- Arduino CLI או Arduino IDE מותקנים ומוגדרים עם ESP32 board package.
- כבל USB תומך data ולא רק charging.
- אין חיבורי מתח מסוכנים בזמן העלאה.
- יש GND משותף רק כאשר בודקים אינטגרציה בין הלוחות.
- קבצי האייקונים `.h` נשארים בתיקיית `instrument_config/`.
- `TFT9341Touch` נשאר בשימוש ולא מוחלף.
- שתי הסקיצות נשארות נפרדות.

## 3. זיהוי ESP32 COM ports ב-Windows

דרכים מומלצות:

1. לחבר רק לוח ESP32 אחד למחשב.
2. לפתוח Device Manager.
3. להיכנס ל-Ports (COM & LPT).
4. לזהות שם כמו `USB-SERIAL CH340`, `CP210x`, או `USB Serial Device`.
5. לרשום את מספר ה-COM, לדוגמה `COM3`.
6. לנתק את הלוח ולוודא שהפורט נעלם.
7. לחבר שוב ולוודא שהפורט חוזר.
8. לסמן פיזית את הלוח כ-`Sensor` או `UI`.

אפשרות דרך PowerShell:

```powershell
[System.IO.Ports.SerialPort]::getportnames()
```

יש לתעד:

| לוח | COM port | הערות |
|---|---|---|
| Sensor board | `COM__` | __________ |
| UI board | `COM__` | __________ |

## 4. סדר העלאה

סדר העלאה מומלץ:

1. Sensor board first - להעלות קודם את `SENS_FIXED/SENS_FIXED.ino`.
2. UI board second - להעלות אחר כך את `instrument_config/instrument_config.ino`.

הסיבה: לוח החיישנים אמור לבצע boot, לזהות חיישנים, לבצע calibration, ואז לשלוח הודעות ללוח ה-UI. לאחר מכן ה-UI יכול לקבל טריגרים ולשלוח צבעים בחזרה.

## 5. Arduino CLI upload commands

יש להחליף את `COM_SENSOR` ו-`COM_UI` בפורטים האמיתיים.

דוגמה ל-Sensor board:

```powershell
arduino-cli upload --fqbn esp32:esp32:esp32 --port COM_SENSOR SENS_FIXED
```

דוגמה ל-UI board:

```powershell
arduino-cli upload --fqbn esp32:esp32:esp32 --port COM_UI instrument_config
```

אם ה-FQBN שונה לפי הלוח בפועל, יש להחליף את `esp32:esp32:esp32` בערך המתאים.

לתיעוד מומלץ לשמור:

```powershell
arduino-cli upload --fqbn esp32:esp32:esp32 --port COM_SENSOR SENS_FIXED *> docs/reports/upload_logs/wave3_sensor_upload.txt
arduino-cli upload --fqbn esp32:esp32:esp32 --port COM_UI instrument_config *> docs/reports/upload_logs/wave3_ui_upload.txt
```

אין להריץ פקודות שמבצעות שינוי קוד.

## 6. אפשרות upload דרך Arduino IDE

אם CLI upload נכשל:

1. לפתוח את `SENS_FIXED/SENS_FIXED.ino` ב-Arduino IDE.
2. לבחור Board מתאים מסוג ESP32.
3. לבחור את ה-Port של לוח Sensor.
4. ללחוץ Upload.
5. לשמור screenshot או להעתיק את הודעת ההצלחה/שגיאה.
6. לחזור על הפעולה עבור `instrument_config/instrument_config.ino` עם לוח UI.

אם העלאה נכשלת גם ב-Arduino IDE, אין לשנות קוד מיד. יש לתעד את ההודעה המדויקת בטופס התקלה.

## 7. Serial Monitor settings

הגדרות כלליות:

- Baud rate: `115200`.
- Line ending: מומלץ להתחיל עם `No line ending`, אלא אם יש צורך לשלוח פקודות ידניות.
- לפתוח Serial Monitor רק ללוח אחד בכל פעם אם יש בלבול בפורטים.

פלט צפוי מלוח Sensor:

- הודעת boot בסגנון `=== SENS: TCA + MIXED-TOF + RF + CAL + NEOPIXEL ===`.
- הודעה על `TCA OK @0x70` או אזהרת חיווט אם חסר.
- תוצאות init לערוצים `CH0` עד `CH7`.
- הודעות calibration לכל ערוץ פעיל.
- הודעות trigger בסגנון `>>RF T,<key>,<mm>`.

פלט צפוי מלוח UI:

- הודעת boot בסגנון `=== VIRTUAL PIANO UI ===`.
- הודעת Touch: `Touch: OK` או `Touch: FAIL`.
- הודעת RF UART.
- הודעות `DF boot wait` ואז `DF ALL READY`.
- הודעות touch debug בעת לחיצה על המסך.
- הודעות trigger כאשר מגיעות פקודות מלוח Sensor.

## 8. Sensor board verification checklist

| בדיקה | Pass/Fail | ראיה נדרשת | הערות |
|---|---|---|---|
| Boot תקין | ___ | Serial Monitor log | __________ |
| `TCA9548A` מזוהה | ___ | `TCA OK @0x70` | __________ |
| חיישנים מזוהים בערוצים | ___ | CH0-CH7 log | __________ |
| Calibration מתחיל ומסתיים | ___ | calibration log | __________ |
| קריאות מרחק יציבות | ___ | Serial log / תצפית | __________ |
| טריגר נשלח בעת קירוב יד | ___ | `>>RF T,<key>,<mm>` | __________ |
| אין טריגרים שגויים כשהיד רחוקה | ___ | תצפית 60 שניות | __________ |
| NeoPixel נדלק בעת טריגר, אם מחובר | ___ | תמונה/וידאו | __________ |
| NeoPixel נכבה אחרי זמן קצר | ___ | תצפית | __________ |

בדיקה מיוחדת: אם חיישן מסוים מופיע כ-`NONE/FAIL`, יש לתעד ערוץ, סוג חיישן, חיווט, ותמונה של החיבור.

## 9. UI board verification checklist

| בדיקה | Pass/Fail | ראיה נדרשת | הערות |
|---|---|---|---|
| מסך נדלק לאחר boot | ___ | צילום מסך/תמונה | __________ |
| ממשק ראשי מופיע | ___ | תמונה | __________ |
| Touch מגיב ללחיצה | ___ | Serial touch log | __________ |
| אייקוני הכלים מוצגים | ___ | תמונה של מסך בחירת כלי | __________ |
| מעבר בין מסכי UI עובד | ___ | תצפית | __________ |
| `DFPlayer` מתחיל init | ___ | Serial log | __________ |
| `DF ALL READY` מופיע | ___ | Serial log | __________ |
| עוצמת שמע סבירה | ___ | תצפית שמע | __________ |
| Track mapping נכון לכלי/תו | ___ | בדיקת קובצי SD | __________ |
| תגובה ל-trigger מלוח Sensor | ___ | Serial log + שמע | __________ |

אם `Touch: FAIL` מופיע, יש לבדוק חיווט `TFT9341Touch`, touch CS, TIRQ, מתח ו-GND. אין להחליף ספריית תצוגה בשלב זה.

## 10. HC-12 integration checklist

| בדיקה | Pass/Fail | הערות |
|---|---|---|
| GND משותף בין שני הלוחות | ___ | __________ |
| TX של Sensor מחובר ל-RX של HC-12 המתאים | ___ | __________ |
| RX של Sensor מחובר ל-TX של HC-12 המתאים | ___ | __________ |
| TX/RX מוצלבים נכון בצד UI | ___ | __________ |
| שני הצדדים עובדים ב-`9600` baud | ___ | __________ |
| לוח Sensor שולח `BOOT,SENS` | ___ | __________ |
| לוח UI שולח `BOOT,UI` | ___ | __________ |
| הודעות `T,<key>,<mm>` מגיעות ל-UI | ___ | __________ |
| הודעות `C,<key>,<colorIdx>` מגיעות ל-Sensor | ___ | __________ |

אם אין תקשורת, יש לבדוק קודם חיווט, GND, מתח, baud rate, וזיהוי COM ports. שינוי קוד הוא רק Wave 4 ומעלה.

## 11. ראיות שהאדם צריך לאסוף

יש לשמור:

- screenshot או העתק טקסט של upload success לכל לוח.
- Serial Monitor log של Sensor boot + calibration.
- Serial Monitor log של UI boot + `DFPlayer` init.
- תמונות ברורות של חיווט שני הלוחות.
- תמונה של מסך UI עם אייקונים.
- צילום או וידאו קצר של NeoPixel בזמן trigger, אם מחובר.
- הערות על כל כשל, כולל תאריך, שעה, COM port, לוח, ומה היה מחובר.
- אם יש שגיאה, להעתיק את ההודעה המדויקת לטופס `docs/forms/09_bug_issue_report_template.md`.

## 12. קריטריוני Pass/Fail

Wave 3 עובר אם:

- שתי ההעלאות מצליחות.
- Sensor board עולה ומדפיס boot log.
- Sensor board מזהה לפחות את החיישנים המחוברים בפועל.
- Sensor board מבצע calibration ללא קריסה.
- UI board עולה ומציג את המסך הראשי.
- Touch מגיב או מתועד כבעיה חומרתית ברורה.
- `DFPlayer` מגיע לשלב init, ואם חומרת השמע מחוברת, נשמע צליל מתאים.
- הודעות trigger מה-Sensor מגיעות ל-UI דרך `HC-12` או חיבור Serial תואם.
- כל בעיה מתועדת עם ראיות.

Wave 3 נכשל אם:

- אחת ההעלאות נכשלת ולא ניתן להשלים אותה ב-CLI או Arduino IDE.
- אחד הלוחות לא עולה כלל לאחר upload.
- יש קצר, התחממות, או בעיית מתח.
- אין אפשרות לזהות איזה COM port שייך לאיזה לוח.
- אין מספיק ראיות כדי להבין מה נכשל.

## 13. Wave הבא מומלץ

Wave 4 ייפתח רק אם בדיקות upload או hardware reveal runtime problems.

דוגמאות ל-Wave 4:

- תיקון בעיית touch runtime.
- תיקון בעיית תקשורת `HC-12`.
- תיקון בעיית `DFPlayer` או מיפוי Tracks.
- התאמת כיול חיישנים לאחר מדידה אמיתית.
- שיפור לוגים או הודעות debug, רק אם יש צורך ברור.

## סיכום בטיחות שינוי

Wave 3 הוא תיעוד בלבד. לא נערכו קבצי `.ino`, לא נערכו קבצי `.h`, לא נמחקו אייקונים, לא הוחלפה `TFT9341Touch`, ולא מוזגו שתי סקיצות Arduino.
