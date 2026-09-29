# design.md

# Nightwave — Physical + Interface Design System

## 1. Design principles

Nightwave should look and behave like a small intentional consumer device, not a development board inside a box.

Priorities:

1. controls are obvious by touch;
2. screen communicates playback state immediately;
3. speaker and headphones feel intentional;
4. enclosure is solid and serviceable;
5. internals look engineered;
6. PCB silkscreen is useful first, expressive second;
7. no visual feature may compromise battery, RF, audio, thermals, or manufacturability.

## 2. Visual direction

Working personality:

**compact offline player / late-night utility / understated retro-digital**

Avoid:
- gamer RGB;
- imitation iPod branding;
- copying PocketPlayer art;
- fake analog gauges;
- crowded cyberpunk decoration;
- unnecessary touchscreen aesthetics.

The physical object should remain recognizable even with the display off.

## 3. Product layout

Preferred portrait-ish front:

```text
┌────────────────────────────┐
│                            │
│       NIGHTWAVE            │
│ ┌────────────────────────┐ │
│ │ ♪ Track / File        │ │
│ │ ▶ 01:42     BAT 78%   │ │
│ │ VOL ███████░          │ │
│ └────────────────────────┘ │
│                            │
│   ◀      ▶/Ⅱ       ▶      │
│                            │
│       − VOL +              │
│                            │
│          speaker           │
│      · · · · · · ·         │
└────────────────────────────┘
```

This is conceptual; exact geometry comes from ergonomics and components.

## 4. Physical control hierarchy

Primary playback controls:
- Previous
- Play/Pause
- Next

Secondary:
- Volume -
- Volume +

Power:
- separate side switch/button to avoid accidental presses.

Controls should be distinguishable:
- play/pause central and largest;
- previous/next symmetric;
- volume separate enough to avoid accidental track changes.

Minimum button touch target is driven by physical switch/cap design, but should be comfortable for one-handed use.

## 5. Port layout

Preferred:
- USB-C: bottom or side, accessible while device rests on table;
- 3.5 mm jack: top edge for cable exit;
- microSD: side/service edge, not where it ejects into the user's palm;
- power: side.

Port positions MUST be frozen jointly between PCB and CAD.

## 6. Speaker design

Speaker:
- on front lower region or rear depending enclosure acoustic testing;
- grille should provide sufficient open area;
- enclosure must prevent diaphragm contact;
- speaker must have a rigid mount;
- avoid rattling surfaces;
- optional gasket if needed.

A rear speaker is acceptable only if tabletop use is not badly muffled.

## 7. Enclosure construction

Preferred two-part enclosure:
- base;
- lid/front.

Fastening:
- screws and bosses / heat-set inserts where suitable.

Internal features:
- PCB standoffs;
- battery cradle;
- speaker pocket;
- button guidance;
- display retention;
- connector clearances;
- cable channels only if wires are unavoidable.

Rounded external edges for pocket handling.

## 8. Serviceability

The enclosure SHOULD be reopenable.

Battery replacement need not be tool-less but should not require destroying the case.

SD card replacement MUST be practical.

## 9. Display typography

Monochrome OLED style:
- one readable bitmap font family or at most two sizes;
- high contrast;
- no decorative tiny fonts;
- truncate/scroll long file names predictably.

Information hierarchy:

### Level 1
Track/file title.

### Level 2
Play/pause state + elapsed time.

### Level 3
Volume, battery, output mode.

## 10. Screen inventory

### Splash
```text
NIGHTWAVE
offline audio
v0.x
```

### Initializing
```text
Mounting SD...
Starting audio...
```

### Now Playing
```text
Track Name
▶ 01:42 / 03:58
VOL 18     BAT 78%
HEADPHONES
```

### Paused
Same screen with a clear pause indicator.

### Library
```text
/Music
> Albums
  Mixes
  song.mp3
```

### No SD
```text
NO SD CARD
Insert card
```

### File error
```text
CAN'T PLAY FILE
Skip / Back
```

### Low battery
```text
LOW BATTERY
Saving state...
```

### Charging
Battery icon/state only; playback remains usable if validated.

## 11. Interaction rules

### Playback screen
- Play/Pause: immediate.
- Next/Previous: immediate.
- Volume buttons: immediate with temporary volume overlay.
- UI work must not block audio.

### Library
- Controls must have an explicitly documented navigation mapping.
- If five buttons are insufficient for intuitive library browsing, a later rotary encoder may be added, but core playback buttons remain.

### Long presses
Use only if they create real value and do not make required actions ambiguous.

## 12. Error-state behavior

Error UI must be factual.

Bad:
- "Oops! Something went wrong!"

Good:
- "SD card removed"
- "Unsupported file"
- "Decoder error"
- "Battery too low"

Where possible show one actionable response.

## 13. Motion

OLED animation must be restrained:
- short transitions;
- no continuous decorative animation during playback;
- avoid wasting CPU/power;
- no animation that can starve audio task.

## 14. Accessibility / usability

- high contrast;
- clear icons plus text where ambiguity exists;
- controls usable without color;
- playback usable with one hand;
- volume changes predictable;
- startup/error text readable;
- no essential status conveyed only by tiny icon differences.

## 15. PCB visual design

PCB should be functional and authored.

Silkscreen SHOULD include:
- NIGHTWAVE;
- board revision;
- USB/power labels;
- battery polarity;
- speaker polarity;
- button labels;
- test-point names;
- headphone label;
- small original music/night-themed art in unused space.

Silkscreen MUST NOT:
- obscure pads;
- cross keepouts;
- cover reference information needed for assembly;
- copy another project's artwork.

## 16. Color/material direction

Do not lock color until fabrication/print options are known.

Roles:
- enclosure base: neutral/dark;
- printed legends: high contrast;
- PCB soldermask may be chosen aesthetically only after fab availability/cost is confirmed.

No project decision depends on a specific color.

## 17. Renders

Required render set:
- PCB front;
- PCB rear;
- exploded assembly;
- finished front;
- finished rear/ports.

Renders must reflect actual final source files.

## 18. Build photography

Take real photos at:
- breadboard audio proof;
- schematic/PCB design screen;
- board arrival;
- bare board inspection;
- assembled PCB;
- first power;
- audio test;
- enclosure fit;
- final device;
- runtime test setup.

Photos are evidence, not decoration.

## 19. Demo video structure

Target 45–90 seconds of clear evidence.

Sequence:
- finished device beauty shot;
- SD card/music;
- boot;
- speaker playback;
- pause/play;
- next/previous;
- volume;
- headphone insertion and stereo demo explanation;
- quick internals/PCB;
- runtime result;
- closing shot.

No cinematic edit should hide whether the device actually works.

## 20. Anti-patterns

Avoid:
- giant enclosure to make CAD easy;
- exposed dev boards in final device;
- permanently dangling jumper wires;
- hot glue as core mechanical structure;
- unreadable OLED text;
- buttons with unclear behavior;
- fake album art;
- unnecessary RGB;
- menus deeper than the feature set justifies;
- excessive boot animation;
- decorative PCB routing that harms return paths/audio;
- a render-only "finished device" with no physical proof.
