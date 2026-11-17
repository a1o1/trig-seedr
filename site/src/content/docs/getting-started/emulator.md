---
title: Trying it in the emulator
description: "Run the patched OS in analog-emu's window and try Trig Seedr before flashing a unit."
sidebar: { order: 4 }
---

analog-emu runs the Rytm's OS in a window with its panel, so you can try Trig Seedr before going near a unit. Build
it first ([Building](../build/)); the build leaves the patched OS in `build/emu/seedr.syx`.

```sh
cd ../analog-emu
./target/release/dtcard play.img format
./target/release/rytm ../trig-seedr/build/emu/seedr.syx --card play.img --snapshot play.snap
```

- A new card first shows **FILE SYSTEM OK, PRESS Y/N TO CLOSE**; every other key is ignored until you close it with
  NO.
- Click a key to press it; **Shift + click** latches it down (for FUNC + YES, or TRIG held + PAGE: Shift + click
  TRIG, click PAGE, Shift + click TRIG). Click **TRIG** three times to open Trig Seedr. Drag or scroll on a knob to
  turn it; the arrow keys nudge the last knob touched.
- `--snapshot` saves the machine when you close the window (or press F5). Delete the `.snap` after rebuilding the
  firmware: it holds the old mod's memory.

Then follow the [Quick start](../quick-start/) from step 2.

## Samples

An emulator card has no factory samples. Add WAVs with `dtcard play.img add *.wav`, then on the Rytm: GLOBAL →
SAMPLES → incoming → RIGHT → SELECT ALL → RIGHT → LOAD TO PROJ → YES. They land in the project's sample slots, ready
for a LOCK lane on SMP.
