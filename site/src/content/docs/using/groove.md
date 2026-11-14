---
title: Groove
sidebar: { order: 3 }
---

GROOVE applies a groove's micro timing and velocity to the trigs already on the track, after Ableton Live's Groove Pool.

![GROOVE](../../../assets/screens/groove.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | GRV | 1…N | the groove; its name is on the bottom line |
| B | TIME | −100…+100 % | how much of the groove's timing is added |
| C | VEL | −100…+100 % | how far velocities follow the groove's accents (negative inverts) |
| D | QUANT | 0–100 % | pull existing micro timing to the grid first |
| E | RAND | 0–100 % | random micro timing, up to half a step |
| F | SHIFT | −63…+63 | move the groove against the pattern |
| G | SEED | FREE, 0–9999 | for RAND |

A step is taken as a 1/16 whatever the track's speed. Micro timing is limited to ±23/24 of a step.

## Your own grooves

The repository builds in five MPC-style swing amounts. `tools/groove2h.py` turns Live's grooves into tables built into the firmware:

```sh
python3 tools/groove2h.py "/Applications/Ableton Live 11 Suite.app/Contents/App-Resources/Core Library/Grooves/Swing/MPC" \
    MY_GROOVES/ > mods/seedr-ar2/grooves-live.h
```

- **`.agr` files:** Live's plain-XML grooves. Each base-grid line takes the displacement of the groove note nearest to it, steps in between move in proportion (a time warp, as Live describes Base), and a step on a grid line takes that note's velocity.
- **MIDI files:** any clip exported from Live. Some `.agr` files are in a protected format the tool cannot read: drag one onto a MIDI track in Live and export the clip instead. Base comes from `--base` or the file name (`8ths` → 1/8); length from `--beats` or the file, rounded up to whole bars.

`grooves-live.h` is derived from Ableton's content and is kept out of the repository.
