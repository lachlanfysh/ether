# EtherSynth Screen Architecture v4 — 960×320 Final
**Corrected Based on All User Feedback**  
**Date: 2025-08-30**

---

## **DISPLAY & HARDWARE**

- **Screen Size**: 960×320 pixels (landscape)
- **Hardware**: 15 top buttons + 16 bottom buttons + SmartKnob (physical, not shown on screen)
- **Visual Style**: Chrome flat design - no shadows, no depth, clean 2D

---

## **SCREEN LAYOUT (Simplified)**

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ ┌─┐ Header (28px) — Pattern A, BPM 128, I5 Armed            │ CPU │ V │ REC │ 
├─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┴─┤
│ ┌Side─┐                                                                      │
│ │Pat  │                                                                      │
│ │Inst │  MAIN CONTENT AREA (268px height)                                   │  
│ │Song │                                                                      │ 
│ │Mix  │  Pattern View / Instrument Parameters / Song Arrangement / Mixer    │
│ │[●]  │                                                                      │ ← Active tab joins content
│ └─────┘                                                                      │
├──────────────────────────────────────────────────────────────────────────────┤
│ Footer (24px) — Tool status, stamp feedback                                 │
└──────────────────────────────────────────────────────────────────────────────┘
```

**Total**: 28 + 268 + 24 = 320px height

---

## **HARDWARE BUTTON MAPPING (Physical Only)**

```
Top Row (15): [PLAY][REC][WRITE][SHIFT][INST][SOUND][PATT][COPY][CUT][ACC][RTCH][TIE][MICRO][FX][NUDGE]
```

### **Button Functions**:
- **PLAY/REC/WRITE/SHIFT**: Transport controls
- **INST**: Instrument parameters screen (replaces SHIFT+REC)
- **SOUND**: Instrument selector overlay  
- **PATT**: Pattern selector overlay
- **COPY**: Copy tool (long press = CLONE)
- **CUT**: Cut tool (long press = DELETE)
- **ACC/RATCHET/TIE/MICRO**: Stamps (affect new note entry)
- **FX**: Effects overlay
- **NUDGE**: Timing nudge tool

---

## **PATTERN SCREEN (Default)**

**1×16 Layout for 960×320 aspect ratio**

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ PATTERN A — I5 (Lead Analog) — 32 Steps — Page 1/2                          │
├──────────────────────────────────────────────────────────────────────────────┤
│ ┌─1─┐ ┌─2─┐ ┌─3─┐ ┌─4─┐ ┌─5─┐ ┌─6─┐ ┌─7─┐ ┌─8─┐ ┌─9─┐ ┌10┐ ┌11┐ ┌12┐ ┌13┐ ┌14┐ ┌15┐ ┌16┐│
│ │C4 ││   ││Em7││   ││G4+││   ││C5 ││   ││   ││F4 ││   ││A4 ││Chd││   ││B4 ││   ││
│ │●●○││   ││●●●││   ││◐○○││   ││●○○││   ││   ││●○○││   ││◐○○││●●●││   ││●○○││   ││ ← P-locks
│ │×2 ││   ││ → ││   ││ ◀ ││   ││   ││   ││   ││   ││   ││   ││×3 ││   ││ → ││   ││ ← Ratchet/tie/micro  
│ │②③││   ││①④││   ││③⑫││   ││   ││   ││   ││① ││   ││②③││①④││   ││   ││   ││ ← Other instruments
│ └───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘│
├──────────────────────────────────────────────────────────────────────────────┤
│ Length: [32] 16/48/64    ●●○○ Page Dots    Swing: 56%    Quantize: 75%      │
└──────────────────────────────────────────────────────────────────────────────┘
```

### **Pattern Features**:
- **1×16 Layout**: Better for 960×320 aspect ratio
- **Armed Instrument Focus**: Large note display (C4, Em7, Chord5)
- **Other Instruments**: Small colored circles ①②③④... (max 15)
- **P-Lock Indicators**: ●●○ (max 3 shown per step)
- **No Chain Settings**: Moved to Song screen
- **Page Navigation**: For >16 steps, show page dots

---

## **INSTRUMENT SCREEN VARIANTS**

### **1. Chromatic Single Sample / Analog Engine**

**1×16 Layout (corrected from 2×8)**

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ INSTRUMENT I5 — Lead Analog — Engine: Analog  Preset: ▼ Warm Lead            │
├──────────────────────────────────────────────────────────────────────────────┤
│ ┌─1─┐ ┌─2─┐ ┌─3─┐ ┌─4─┐ ┌─5─┐ ┌─6─┐ ┌─7─┐ ┌─8─┐ ┌─9─┐ ┌10┐ ┌11┐ ┌12┐ ┌13┐ ┌14┐ ┌15┐ ┌16┐│
│ │Cut││Res││Drv││Atk││Dec││Sus││Rel││Tun││Gli││Str││End││Wid││Dep││Rat││Syn││Vol││
│ │78%││23%││45%││12││567││89%││234││+2c││45%││0% ││100││67%││34%││2Hz││On ││85%││
│ └───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘│
├──────────────────────────────────────────────────────────────────────────────┤
│ Press bottom button 1-16 to select • Turn SmartKnob to adjust               │
│ SHIFT + button for page 2 (params 17-32) • Poly: 8 voices                  │
└──────────────────────────────────────────────────────────────────────────────┘
```

### **2. Multi-Sample Kit Engine**

**Page 1: Sample Assignment**
```
┌──────────────────────────────────────────────────────────────────────────────┐
│ INSTRUMENT I3 — Drum Kit — Engine: Kit  Preset: ▼ 808 Classic    Page 1/2   │
├──────────────────────────────────────────────────────────────────────────────┤
│ ┌─1─┐ ┌─2─┐ ┌─3─┐ ┌─4─┐ ┌─5─┐ ┌─6─┐ ┌─7─┐ ┌─8─┐ ┌─9─┐ ┌10┐ ┌11┐ ┌12┐ ┌13┐ ┌14┐ ┌15┐ ┌16┐│
│ │Kck││Snr││Hat││OHt││Tom││Cla││Rim││Cra││Rid││Per││Shk││Cow││Fx1││Fx2││   ││   ││
│ │808││909││Cld││Opn││Low││Clp││Sht││Brk││Jzz││Blk││Mrk││Blk││Rsr││Air││   ││   ││
│ └───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘│
├──────────────────────────────────────────────────────────────────────────────┤
│ Press button 1-16 to select pad • Turn SmartKnob to choose sample file      │
│ SHIFT for Page 2 (engine parameters) • 16 kit pieces max                   │
└──────────────────────────────────────────────────────────────────────────────┘
```

**SHIFT Page 2: Engine Parameters**  
```
┌──────────────────────────────────────────────────────────────────────────────┐
│ INSTRUMENT I3 — Drum Kit — Engine Parameters                     Page 2/2   │
├──────────────────────────────────────────────────────────────────────────────┤
│ ┌─1─┐ ┌─2─┐ ┌─3─┐ ┌─4─┐ ┌─5─┐ ┌─6─┐ ┌─7─┐ ┌─8─┐ ┌─9─┐ ┌10┐ ┌11┐ ┌12┐ ┌13┐ ┌14┐ ┌15┐ ┌16┐│
│ │Cut││Res││Drv││Cmp││Vol││Pan││Snd1││Snd2││Rvrb││Dly││Hi ││Mid││Low││   ││   ││   ││
│ │65%││12%││23%││45%││85%││C  ││25%││15%││30%││18%││+2 ││-1 ││+3 ││   ││   ││   ││
│ └───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘│
├──────────────────────────────────────────────────────────────────────────────┤
│ Press button 1-16 to select parameter • Turn SmartKnob to adjust            │
│ Global engine parameters affecting entire kit instrument                    │
└──────────────────────────────────────────────────────────────────────────────┘
```

### **3. Sliced Sample Engine**

**Page 1: Slice Assignment**
```
┌──────────────────────────────────────────────────────────────────────────────┐
│ INSTRUMENT I7 — Sliced — Sample: Breakbeat_140.wav — Engine: Slice  Page 1/2 │
├──────────────────────────────────────────────────────────────────────────────┤
│ ┌─────────────────── SPECTRUM VIEW ──────────────────────────────────────────┐ │
│ │ ░░░░░██████░░░░░░██░░█████░░░░██████░░░░░░██░░░░░░██████░░░░░░░░░░░░░░░░░░ │ │ ← Waveform
│ │ ▲   ▲     ▲     ▲   ▲     ▲   ▲     ▲     ▲   ▲     ▲     ▲   ▲   ▲     │ │ ← Slice markers
│ │ 1   2     3     4   5     6   7     8     9   10    11    12  13  14    │ │ ← Slice numbers
│ └─────────────────────────────────────────────────────────────────────────────┘ │
├──────────────────────────────────────────────────────────────────────────────┤
│ SmartKnob: Scrub spectrum to position • Press buttons 1-16 to place slices  │
│ SHIFT for Page 2 (engine parameters) • Max 16 slices per sample             │
└──────────────────────────────────────────────────────────────────────────────┘
```

**SHIFT Page 2: Engine Parameters**
```
┌──────────────────────────────────────────────────────────────────────────────┐
│ INSTRUMENT I7 — Sliced — Engine Parameters                        Page 2/2   │
├──────────────────────────────────────────────────────────────────────────────┤
│ ┌─1─┐ ┌─2─┐ ┌─3─┐ ┌─4─┐ ┌─5─┐ ┌─6─┐ ┌─7─┐ ┌─8─┐ ┌─9─┐ ┌10┐ ┌11┐ ┌12┐ ┌13┐ ┌14┐ ┌15┐ ┌16┐│
│ │Cut││Res││Atk││Dec││Sus││Rel││Tune││Vol││Pan││Snd1││Snd2││Rvrb││Dly││   ││   ││   ││
│ │72%││18%││5ms││450││78%││320││-2c ││88%││L15││35%││20%││25%││12%││   ││   ││   ││
│ └───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘│
├──────────────────────────────────────────────────────────────────────────────┤
│ Press button 1-16 to select parameter • Turn SmartKnob to adjust            │
│ Global engine parameters affecting entire sliced instrument                 │
└──────────────────────────────────────────────────────────────────────────────┘
```

### **4. Chord Instrument**

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ INSTRUMENT I12 — Chord — Links: I5+I8+I11 (3/8 max) — Engine: Chord        │
├──────────────────────────────────────────────────────────────────────────────┤
│ ┌─1─┐ ┌─2─┐ ┌─3─┐ ┌─4─┐ ┌─5─┐ ┌─6─┐ ┌─7─┐ ┌─8─┐ ┌─9─┐ ┌10┐ ┌11┐ ┌12┐ ┌13┐ ┌14┐ ┌15┐ ┌16┐│
│ │I1 ││I2 ││I3 ││I4 ││I5●││I6 ││I7 ││I8●││I9 ││I10││I11││I12││I13││I14││I15││I16││ ← Link targets
│ │   ││   ││   ││   ││①Ld││   ││   ││②Bs││   ││   ││③Pd││   ││   ││   ││   ││   ││ ← Order: 1st, 2nd, 3rd
│ └───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘└───┘│
├──────────────────────────────────────────────────────────────────────────────┤
│ Press buttons 1-16 to toggle links • Order auto-assigned: 1st, 2nd, 3rd... │
│ Chord: Maj7  Inversion: 1st  Notes: C-E-G-B distributed to I5→I8→I11       │
│ SmartKnob: Chord type → Maj/Min/7th/Sus/Dim → Inversion → Velocity Spread   │ 
└──────────────────────────────────────────────────────────────────────────────┘
```

---

## **SONG SCREEN (All Global Settings)**

```
┌──────────────────────────────────────────────────────────────────────────────┐
│ SONG — "My Track" — 142 bars                                                │
├──────────────────────────────────────────────────────────────────────────────┤
│ ┌─Sections─┐  ┌─Timeline───────────────────────────────────────────────────┐ │
│ │ Intro  A4│  │ A │ A │ A │ A │ B │ B │ C │ C │ B │ B │ D │ D │ A │ ...     │ │
│ │ Verse  B8│  │   │   │   │   │   │   │   │   │   │   │   │   │   │         │ │
│ │ Chorus C4│  └───▲───────────────────────────────────────────────────────┘ │
│ │ Bridge D4│      Current: Bar 5                                            │
│ │ [Edit]   │                                                                │
│ └──────────┘                                                                │
├──────────────────────────────────────────────────────────────────────────────┤
│ Master BPM: [128] Tap   Key: [C] Scale: [Major] ▼                          │
│ Chain Mode: ●Auto ○Manual ○OneShot     Master Volume: 85%                   │
│ Global Swing: [56%]   Global Quantize: [75%]   Song Length: 142 bars       │
└──────────────────────────────────────────────────────────────────────────────┘
```

### **Song Features**:
- **All Global Settings**: BPM, key, scale, global swing, global quantize
- **Pattern Chaining**: Chain mode and song structure
- **No Separate Global Screen**: Everything in Song screen

---

## **SIDE MENU (Chrome Tab Joining)**

```
┌─────────┐┌─────────────────────────────────────────────────────────┐
│ Pattern ├┤ Pattern content (joined - no border between)          │
├─────────┤│                                                        │
│ Instrmt ││ Content area                                           │
│ Song    ││                                                        │ 
│ Mix     ││                                                        │
└─────────┘└─────────────────────────────────────────────────────────┘
```

**Screens**: Pattern, Instrument, Song, Mix (no separate Global)

---

## **OVERLAYS (Single Active)**

### **Sound Overlay**
```
┌───────────────────────────┐
│ Select Instrument I1–I16  │
│ ┌I1─┐┌I2─┐┌I3─┐┌I4─┐      │
│ │Kit││Bs ││Ld●││Pad│      │ ← I3 armed
│ └───┘└───┘└───┘└───┘      │
│ [Continue I5-I16...]      │
│ Engine: [Analog] for I3   │ ← Only for unassigned
│        [×] Close          │
└───────────────────────────┘
```

### **Pattern Overlay**
```
┌───────────────────────────┐
│ Select Pattern A–P        │
│ ┌A──┐┌B──┐┌C──┐┌D──┐      │
│ │16●││32 ││16 ││24 │      │ ← A active  
│ └───┘└───┘└───┘└───┘      │
│ [Continue E-P...]         │
│        [×] Close          │
└───────────────────────────┘
```

---

## **VISUAL DESIGN (True Chrome Style)**

### **Flat 2D Design**:
- **No shadows, no depth effects**
- **Clean lines and flat surfaces**
- **Rounded corners**: 6px radius
- **Color palette**: Light greys (#F8F9FA, #E8EAED), mid greys (#DADCE0), dark text (#3C4043)
- **Color pops**: Instrument-specific colors from pastel palette
- **Active highlights**: Chrome blue (#1A73E8)

### **Tab Joining Effect**:
Active side menu tab **connects seamlessly** to content area with no border between them.

---

## **INTERACTION MODEL**

### **Hardware (Physical Buttons)**:
- **Top 16**: Transport + tools + stamps  
- **Bottom 16**: Step entry + parameter selection
- **SmartKnob**: Value adjustment + timeline scrubbing
- **Long press any step**: DELETE

### **Touch**:
- **Side Menu**: Screen navigation
- **Overlays**: Selection dialogs
- **Settings**: All adjustments not handled by hardware

### **Key Workflows**:
1. **PATT button** → Select pattern → Pattern screen updates
2. **SOUND button** → Select instrument → Pattern screen shows new armed instrument
3. **Side menu → Instrmt** → Press button 1-16 → Adjust with SmartKnob
4. **SHIFT + button**: Access parameter page 2 or special functions per engine type

---

## **CORRECTED FEATURES SUMMARY**

✅ **CUT button restored** - long press step = DELETE  
✅ **No on-screen hardware** - physical buttons only  
✅ **1×16 layouts** - correct for 960×320 aspect ratio  
✅ **Chain moved to Song** - removed from Pattern screen  
✅ **Engine-specific views** - Kit, Slice, Chord variants  
✅ **No Global screen** - settings in Song screen  
✅ **True Chrome flat design** - no shadows or depth  
✅ **Simplified interaction** - hardware + touch hybrid  

---

**This architecture provides the clean, hardware-first interface optimized for 960×320 with specialized views for different instrument types and proper Chrome flat styling.**