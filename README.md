# SQUALCH-32 — Step-LFO Ladder Synth (VST3)

סינת' VST3 לצלילים פסיכדליים בסגנון Zenon: סקוולץ', ביצתי, זז כל הזמן.
מונו/לגאטו עם Glide, פילטר Ladder בסגנון Moog Subsequent, וסיקוונסר של 32 צעדים שבו **כל צעד הוא LFO עצמאי**.

## איך מקבלים את קובץ ה-VST3

### אפשרות א' — בלי להתקין כלום (GitHub Actions)
1. פותחים ריפו חדש ב-GitHub ומעלים אליו את כל התיקייה הזו (כולל `.github`).
2. הבנייה מתחילה לבד. בלשונית **Actions** → הריצה האחרונה → **Artifacts** → מורידים `Squalch32-Windows`.
3. מעתיקים את `Squalch32.vst3` ל-`C:\Program Files\Common Files\VST3\`.
4. ב-Cubase: Studio → VST Plug-in Manager → Rescan. הפלאגין מופיע תחת **Instrument → Synth → PsyLab**.

### אפשרות ב' — בנייה במחשב (Windows)
דרוש: Visual Studio 2022 (עם "Desktop development with C++"), CMake 3.22 ומעלה, Git.
```
cmake -B build -G "Visual Studio 17 2022" -DSQ_FORMATS="VST3;Standalone"
cmake --build build --config Release --target Squalch32_VST3
```
הקובץ ייווצר ב-`build/Squalch32_artefacts/Release/VST3/Squalch32.vst3`. JUCE 8 יורד אוטומטית בבנייה הראשונה.

## מבנה הסאונד
```
OSC1 (morph Tri>Saw>Square>Pulse) ┐
OSC2 (morph, Hard Sync, FM 1>2)   ├ MIXER ─ MULTIDRIVE ─ BITE(pre) ─ LADDER 6/12/18/24 LP/BP/HP ─ BITE(post) ─ VCA
SUB (-1/-2) · NOISE · FEEDBACK    ┘
─ TALK (פורמנטים A-E-I-O-U) ─ CRUSH ─ PHASER ─ PING-PONG DELAY ─ REVERB
```
הקול רץ ב-2x oversampling (פחות אליאסינג ברזוננס גבוה ובדיסטורשן).

## סיקוונסר Step-LFO (הלב של הפלאגין)
לכל אחד מ-32 הצעדים יש:
- **CURVE (offset)**: גובה קבוע של הקאטוף בצעד (כמו סיקוונסר של Moog). גוררים לציור על פני כמה צעדים.
- **DEPTH**: כמה חזק ה-LFO של הצעד (אפשר שלילי = הפוך).
- **SHAPE**: Sine, Tri, Ramp Up/Down, Square, S&H, Smooth Random, **Squelch** (פלאק מעריכי, הכי "אסידי"), **Chaos** (מפה לוגיסטית). קליק = הבא, Shift+קליק = הקודם, קליק ימני = תפריט / "החל על כל הצעדים".
- **RATE**: כמה מחזורי LFO בתוך הצעד: OFF (צעד סטטי), 1/8 ... 16. גוררים למעלה/למטה או גלגלת.

בקרות גלובליות: STEPS (1-32), STEP LEN (מ-1/64 עד בר שלם, כולל טריולות ומנוקדים), >CUTOFF (עומק באוקטבות), SMOOTH, SPEED (מכפיל לכל ה-LFOs, לא מסונכרן = פסיכדלי), WANDER (סטייה אקראית בכל צעד), SWING, CHAOS, ושליחות נוספות ל-RESO / PITCH / DRIVE / TALK.
מסונכרן למיקום של Cubase כשה-transport מתנגן; כשהוא עצור רץ לפי ה-BPM ומתאפס בכל נוטה (RETRIG).

כפתורים: **RANDOM** (פטרן חדש עם משקלים מוזיקליים), **MUTATE** (שינוי עדין של הקיים, כדי שהסאונד יתפתח), **CLEAR**.
Mod Wheel מגביר את עומק הסיקוונסר בזמן אמת.

## טיפים לסאונד ביצתי / Zenon
- רזוננס 0.7–0.95, Multidrive 0.3–0.6, Cutoff נמוך (200–400Hz), ו-Seq>Cutoff 2.5–3.5 אוקטבות.
- לערבב בין צעדי OFF (סטטיים) לצעדי Squelch ב-RATE 2–4: זה נותן את ה"דיבור" הבועתי.
- TALK + Seq>Talk = צליל שנשמע כמו מילים, צפרדעים, מים.
- FEEDBACK במיקסר + Slope 18dB = בוץ עבה ומלוכלך.
- Phaser איטי (0.05–0.15Hz) + Delay 1/8D בפינג-פונג = תנועה מתמדת בסטריאו.

## פריסטים
Bog Walker · Swamp Talk · Squalch Rider · Mycelium Drift · Zenon Groove Bass · Spore Rain · Deep Mud · Alien Frog · Sludge Tentacle · Fractal Bubbles (מכוונים ל-125–138 BPM).

## קבצים
- `Source/dsp/SquelchEngine.h` — מנוע ה-DSP (C++17, לא תלוי ב-JUCE)
- `Source/dsp/ParamTable.h` — כל הפרמטרים במקום אחד (207 פרמטרים, כולם ניתנים לאוטומציה ב-Cubase)
- `Source/dsp/Presets.h` — פריסטים
- `Source/PluginProcessor.*`, `Source/PluginEditor.*` — עטיפת JUCE וממשק
- `tools/render_demo.cpp` — מרנדר את כל הפריסטים ל-WAV בלי DAW
- `tests/HostTest.cpp` — בדיקה אוטומטית (`-DSQ_BUILD_TESTS=ON`)
