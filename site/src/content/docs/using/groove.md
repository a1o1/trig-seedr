---
title: TIME (grooves)
sidebar: { order: 3 }
---

TIME sets the trigs' micro timing: a groove's timing and velocity, after Ableton Live's Groove Pool, and random timing. It runs last of the layers, so a groove's velocity scales what DYNAMICS set. With SRC = TRACK it grooves the trigs already on the track, as 0.3's GROOVE page did.

![TIME](../../../assets/screens/time.png)

| Knob | Name | Range | What |
|---|---|---|---|
| A | GRV | 0…N | the groove (0: none); its name is on the bottom line while the knob turns |
| B | TIME | −100…+100 % | how much of the groove's timing is added |
| C | GVEL | −100…+100 % | how far velocities follow the groove's accents (negative inverts) |
| D | QUANT | 0–100 % | pull existing micro timing to the grid first |
| E | RAND | 0–100 % | random micro timing, up to ±23/24 of a step at 100 % (0.3's GROOVE RAND 100 is about RAND 50) |
| G | SHFT | −63…+63 | move the groove against the pattern |

A step is taken as a 1/16 whatever the track's speed. Micro timing is limited to ±23/24 of a step. RAND draws from the take's seed, so it changes with SEED like the other layers.

## Your own grooves

The repository builds in five MPC-style swing amounts (SWING 54 … SWING 71). `tools/groove2h.py` turns Live's grooves into tables built into the firmware:

```sh
python3 tools/groove2h.py "/Applications/Ableton Live 11 Suite.app/Contents/App-Resources/Core Library/Grooves/Swing/MPC" \
    MY_GROOVES/ > mods/seedr-ar2/grooves-live.h
```

- **`.agr` files:** Live's plain-XML grooves. Each base-grid line takes the displacement of the groove note nearest to it, steps in between move in proportion (a time warp, as Live describes Base), and a step on a grid line takes that note's velocity.
- **MIDI files:** any clip exported from Live. Some `.agr` files are in a protected format the tool cannot read: drag one onto a MIDI track in Live and export the clip instead. Base comes from `--base` or the file name (`8ths` → 1/8); length from `--beats` or the file, rounded up to whole bars.

`grooves-live.h` is derived from Ableton's content and is kept out of the repository.

A `grooves-live.h` made before 0.4 still builds; making it again writes the new table type.
