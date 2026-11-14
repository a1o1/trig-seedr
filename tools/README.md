# tools

## Tests

    sh tools/test_seedr.sh            # all parts; PART=1..7 runs one (each under 3 minutes)
    sh tools/build_release.sh         # the firmware to flash (docs/flashing.md)

Builds core-ar2 and the seedr mod, patches the stock OS 1.74 (`build/seedr.syx`, version GR01) and drives analog-emu's
`dtboot` through the Rytm's own keys and knobs. After each stage `emucmp.py` reads the A01 BD track (trig, velocity,
flag words, µtime, sample locks, the modified mark) and compares it with `genref` (the host build of `lib/gen.c` and
`lib/groove.c` with the same settings).

| Part | Checks |
|---|---|
| 1 | build; boot empty; YES seed 1; seed 2 from the original; NO; FUNC + YES; closing keeps; a take equals a fresh generate |
| 2 | GROOVE µtime; NO; a GEN take grooved, NO undoes only the groove; velocity from a groove (needs `grooves-live.h`); regenerate rule |
| 3 | SAVE / RELOAD PATTERN; power cycle; the project's modified mark |
| 4 | GEN 3 sample locks; NO removes them; they survive SAVE + RELOAD PATTERN |
| 5 | ROTATE whole steps, one field, every track (SD moves too), NO; whole steps carry sample locks |
| 6 | a test build with no lock rows left: GEN 3 refuses and writes nothing |
| 7 | the keys: TRIG on EUCLIDEAN opens seedr, TRIG goes back to stock's TRIG page, then EUCLIDEAN, seedr again; SRC closes it |

Parts 2–5 resume from part 1's (and 5 from part 4's) emulator snapshots.

`test_groove2h.py GROOVE_DIR` writes five of Live's plain `.agr` grooves out as MIDI files (format 0 and 1) and checks
`groove2h.py` gives the same tables from both (±1: MIDI holds whole ticks and velocities).

## Helpers

| Script | What |
|---|---|
| `groove2h.py` | `.agr` / `.mid` → groove tables (`--dump FILE` shows what it read) |
| `genref.c` | host reference: `genref SEED [LEN [GROOVE TIME VEL QUANT RAND SHIFT GSEED [DENS SMP LO HI [ROT]]]]`; `GENREF_ROT="K WHAT MODE"` rotates afterwards |
| `emucmp.py` | one track of an emulator snapshot in genref's format |
| `presses.py` | menu key sequences → `dtboot --press` arguments |
| `fb2png.py` | an emulator screen (`dtpeek SNAP fb`) → PNG |
| `voice.py`, `onsets.py` | one channel of `dtboot --usb-wav` (2 = BD), and its hits |

## Firmware study

Need `pip install capstone` (and `unicorn` for `emu_steps.py`) and an elekloader checkout. Run them in a scratch
directory outside the repository: the unpacked sections are Elektron's firmware, never commit or share them.

    python3 unpack.py <elekloader checkout> <Analog-Rytm_MKII_OS1.74.syx>   # sec3.bin = the OS at 0x40000400
    python3 cfdis.py 400c0776 80      # disassemble: address, byte count (hex)
    python3 scan.py 400c0300 400c2800 # functions writing at a fixed offset from a pointer
    python3 xref.py 400c2482          # callers and references
    python3 fns.py 400c2a00 400c3e00  # function starts in a range
    python3 summ.py 400c33ae          # a function's calls and the offsets it touches
    python3 emu_steps.py sec3.bin     # runs the stock step setters against a fake track object
    sh emu_rytm_steps.sh              # analog-emu: places trigs and sets fields through the UI, prints the record

## Samples in the emulator

The emulator's card has no factory samples. `dtcard IMAGE add FILES.wav` puts WAVs in `incoming`; on the Rytm, GLOBAL >
SAMPLES > incoming > RIGHT > SELECT ALL > RIGHT > LOAD TO PROJ > YES loads them into the project's slots, and
`dtboot --power-off` keeps them on the card for the next cold boot.
