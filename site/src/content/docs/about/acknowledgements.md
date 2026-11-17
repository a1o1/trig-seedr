---
title: Acknowledgements
sidebar: { order: 2 }
---

Trig Seedr builds on, and learned from, these open source projects:

| Project | License | How it was used |
|---|---|---|
| [elekloader](https://github.com/elekloader/elekloader) | GPL-2.0 | the mod loader, the Rytm core (core-ar2) the mod runs on, its build and patch tools; its notes on the OS's tracks, patterns and lock store |
| [analog-emu](https://github.com/trigcondition/analog-emu) | MIT | the whole-device emulator every test runs in (headless `dtboot`, the windowed `rytm`, `dtcard` for sample cards) |
| [sidecar-mods](https://github.com/trigcondition/sidecar-mods) | GPL-2.0 | reverse-engineering notes (`re/symbols.toml`): the parameter-lock routines and tables, sample-slot and live-record paths; `tools/presses.py` follows its `pj_emu.py` |
| [sidecar-expander](https://github.com/trigcondition/sidecar-expander), [sidecar-firmware](https://github.com/trigcondition/sidecar-firmware) | GPL-2.0 | background on extending the Rytm; this site follows sidecar-expander's Starlight setup |
| [libanalogrytm](https://github.com/bsp2/libanalogrytm) | MIT | the pattern format: step fields, flag bits, the note-length scale, trig conditions |
| [rytm-rs](https://github.com/alisomay/rytm-rs) | MIT | micro-timing values and encodings |
| [Synthack-OS](https://github.com/karigardarsson/Synthack-OS) | MIT | Syntakt firmware modding (SyntHack, Syntakt MAX), for the later Syntakt port |
| [syntakt-firmware-workbench](https://github.com/Bezronczek/syntakt-firmware-workbench) | MIT | Syntakt firmware tooling, for the later Syntakt port |
| [digikit](https://github.com/ElekLabs/digikit) | AGPL-3.0 | Digitakt II / Digitone II firmware research |
| [modwerk](https://github.com/repeat98/modwerk) | mixed (Elastic-2.0 and others) | firmware modding research |
| [Capstone](https://www.capstone-engine.org/), [Unicorn](https://www.unicorn-engine.org/) | BSD-3 / GPL-2.0 | disassembly and function-level emulation in `tools/` |
| [Mutable Instruments' Eurorack modules](https://github.com/pichenettes/eurorack): Grids by Emilie Gillet | GPL-3.0-or-later | the MAP source: its map of drum patterns is Grids' (`lib/map_data.c`, copied with its licence notice), and X / Y, DENS, CHAOS and the accents follow Grids' code; `tools/tests/23-map.sh` and `32-map-accents.sh` compare the two |
| [Astro Starlight](https://starlight.astro.build/) | MIT | this website |

Methods taken from research:

| Source | How it was used |
|---|---|
| Clarence Barlow, "Two Essays on Theory", *Computer Music Journal* 11(1), 1987 | indispensability: the rank of each step in a meter (BIAS, ROLE, SHAPE = RANK, AMOD = TOP), and the basic orders for the primes 2 to 13 |
| [hmt](https://gitlab.com/rd--/hmt) by Rohan Drape, `Music.Theory.Meter.Barlow_1987` (GPL-3.0) | the reference trig-seedr's ranks were checked against: Barlow's formula as code, and its tables of values |
| George Sioros and Carlos Guedes, their work on syncopation transformations | SYNC: anticipation, an onset moved onto the preceding weaker pulse, which leaves the stronger one silent |

Apart from Grids' map data in `lib/map_data.c`, no code or data from the projects under other licences is included;
each is credited for what it documented or made possible.
