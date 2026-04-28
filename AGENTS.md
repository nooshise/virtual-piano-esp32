# הנחיות לסוכני עבודה בפרויקט

מסמך זה מיועד לכל מי שעובד על המאגר `nooshise/virtual-piano-esp32`, כולל סוכני AI, בודקי תוכנה, בודקי חיווט, מורה/מנחה ובעל הפרויקט.

## מבנה המאגר

```text
.
├── README.md
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

## תפקיד הסקיצות

- `instrument_config/instrument_config.ino` היא סקיצת לוח ה-UI.
- לוח ה-UI כולל מסך מגע TFT, ספריית `TFT9341Touch`, קבצי אייקונים `.h`, תקשורת מול לוח החיישנים ושליטה ב-`DFPlayer`.
- `SENS_FIXED/SENS_FIXED.ino` היא סקיצת לוח החיישנים.
- לוח החיישנים כולל חיישני ToF כמו `VL53L0X` ו-`VL6180X`, מולטיפלקסר `TCA9548A`, תקשורת `HC-12`, וניהול נורות/חיוויים.

## כללי "לא לשנות"

- אין למזג את שתי הסקיצות לקובץ `.ino` אחד.
- אין למחוק, להחליף או ליצור מחדש את קבצי האייקונים `.h`.
- אין להחליף את `TFT9341Touch` בספריית תצוגה אחרת.
- אין לבצע שינויי לוגיקה גדולים ללא אישור מפורש.
- אין לשנות חיווט, מספרי פינים או פרוטוקול תקשורת בלי לעדכן את הטפסים הרלוונטיים.
- אין לערוך קבצי מקור כחלק ממשימות תיעוד בלבד.

## בדיקת מבנה תיקיות Arduino

לפני שינוי טכני יש לוודא:

- שם התיקייה `instrument_config` זהה לשם הסקיצה `instrument_config.ino`.
- שם התיקייה `SENS_FIXED` זהה לשם הסקיצה `SENS_FIXED.ino`.
- כל קובצי האייקונים נמצאים בתוך `instrument_config/`, ליד `instrument_config.ino`.
- אין קובץ `.ino` נוסף באותה תיקייה שמערבב בין לוח ה-UI ללוח החיישנים.
- כל סקיצה נפתחת ומתקמפלת בנפרד ב-Arduino IDE.

## אופן עבודה מומלץ

- לבצע שינויים קטנים, ממוקדים וקלים לסקירה.
- להסביר את התוכנית לפני שינוי קוד מקור.
- לציין אילו קבצים יושפעו לפני עריכת קבצי `.ino` או `.h`.
- להעדיף תיקון נקודתי על פני ארגון מחדש של הקוד.
- לא לשנות שמות קבצים, שמות תיקיות או ספריות חיצוניות ללא סיבה מתועדת.
- לאחר כל שינוי טכני, להריץ בדיקה מתאימה או להסביר מדוע לא ניתן להריץ אותה.

## עדכון טפסים לאחר שינוי טכני

לאחר כל שינוי טכני יש לעדכן לפחות אחד מהטפסים הבאים לפי הצורך:

- שינוי דרישות: `docs/forms/02_requirements_acceptance_form.md`
- שינוי רכיבים: `docs/forms/03_components_bom_form.md`
- שינוי חיווט או פינים: `docs/forms/04_wiring_connection_table.md`
- שינוי חיישנים או כיול: `docs/forms/05_sensor_calibration_log.md`
- שינוי קבצי קול או מיפוי `DFPlayer`: `docs/forms/06_dfplayer_sd_track_map.md`
- בדיקת קומפילציה/העלאה: `docs/forms/07_compile_upload_test_report.md`
- בדיקת אינטגרציה: `docs/forms/08_integration_test_checklist.md`
- תקלה שהתגלתה: `docs/forms/09_bug_issue_report_template.md`
- שינוי שדורש אישור: `docs/forms/10_change_request_approval.md`
- סיכון חדש או טיפול בסיכון: `docs/forms/11_risk_mitigation_register.md`
- הכנה להדגמה: `docs/forms/12_final_demo_checklist.md`
- אישור מסירה: `docs/forms/13_stakeholder_signoff.md`

## כלל סיום משימה

בסיום כל משימה יש לסכם:

- אילו קבצים נוצרו או שונו.
- האם קוד מקור נערך.
- אילו בדיקות בוצעו.
- אילו טפסים עודכנו.
- מה נדרש כצעד הבא.
