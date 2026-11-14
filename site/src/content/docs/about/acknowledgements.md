---
title: Acknowledgements
sidebar: { order: 1 }
---

Trig Seedr builds on, and learned from, these open source projects:

| Project | License | How it was used |
|---|---|---|
| [elekloader](https://github.com/elekloader/elekloader) | GPL-2.0 | the mod loader, the Rytm core (core-ar2) the mod runs on, its build and patch tools; its notes on the OS's tracks, patterns and lock store |
| [analog-emu](https://github.com/trigcondition/analog-emu) | MIT | the whole-device emulator every test runs in |
| [sidecar-mods](https://github.com/trigcondition/sidecar-mods) | GPL-2.0 | reverse-engineering notes: the parameter-lock routines and tables; the key-sequence helper follows its `pj_emu.py` |
| [sidecar-expander](https://github.com/trigcondition/sidecar-expander), [sidecar-firmware](https://github.com/trigcondition/sidecar-firmware) | GPL-2.0 | background on extending the Rytm; this site follows sidecar-expander's Starlight setup |
| [libanalogrytm](https://github.com/bsp2/libanalogrytm) | MIT | the pattern format: step fields, flag bits, the note-length scale, trig conditions |
| [rytm-rs](https://github.com/alisomay/rytm-rs) | MIT | micro-timing values and encodings |
| [Synthack-OS](https://github.com/karigardarsson/Synthack-OS) | MIT | Syntakt firmware modding, for the later Syntakt port |
| [syntakt-firmware-workbench](https://github.com/Bezronczek/syntakt-firmware-workbench) | MIT | Syntakt firmware tooling |
| [digikit](https://github.com/ElekLabs/digikit) | AGPL-3.0 | Digitakt II / Digitone II firmware research |
| [modwerk](https://github.com/repeat98/modwerk) | mixed | firmware modding research |
| [Capstone](https://www.capstone-engine.org/), [Unicorn](https://www.unicorn-engine.org/) | BSD-3 / GPL-2.0 | disassembly and function-level emulation in the study tools |
| [Astro Starlight](https://starlight.astro.build/) | MIT | this website |
