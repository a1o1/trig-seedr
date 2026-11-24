---
title: Devices
description: "The Elektron machines Trig Seedr runs on, their OS, their status and what differs between them."
sidebar: { order: 1 }
---

Trig Seedr's engine and pages are the same on every device; each device has its own build, made from your own copy
of its stock OS, and its own page for what differs.

| Device | OS | Status | Hardware passes | Build | Flash |
|---|---|---|---|---|---|
| [Analog Rytm MKII](../analog-rytm-mk2/) | 1.74 | beta | 0.5.1, 0.6.0, 0.7.1, 0.9.0 ([list](../../getting-started/flashing/#tested-on-hardware)) | `sh tools/build_release.sh` → `trig-seedr-<version>-os1.74.syx` | [Flashing a Rytm MKII](../../getting-started/flashing/) |
| [Syntakt](../syntakt/) | 1.42 | ready for testing (beta) | 0.7.1, 0.9.0 on one unit ([list](../../getting-started/flashing-syntakt/#tested-on-hardware)) | `sh tools/build_release_st.sh` → `trig-seedr-<version>-st-os1.42.syx` | [Flashing a Syntakt](../../getting-started/flashing-syntakt/) |

Each build runs on that device and that OS only: a file is refused for any other stock file. Every release is
tested in full in an emulator on each device; the hardware passes are listed on each device's flashing page.

## What differs

| | Analog Rytm MKII | Syntakt |
|---|---|---|
| Tracks Trig Seedr writes | 12 voice tracks (not the FX track) | tracks 1–12 (not the FX track) |
| Fields a take writes | trigs, notes, velocity, length, micro timing, conditions, retrigs, the accent flag, parameter locks | the same without retrigs and the accent flag; chord notes are kept |
| LOCK's parameters | one list for every track | the track's machine's SYN controls, then the shared pages |
| Lock limit | 72 lock rows per pattern (NO LOCK ROWS LEFT) | no pattern-wide pool: each step has its own lock slots (no NO LOCK ROWS LEFT) |
| Opening | TRIG, TRIG, TRIG (a third TRIG page), or TRIG held + PAGE | the same |
| TURING (0.9) | the TURG source and the live page | the same |

## Planned

The Analog Four MKII and the Digitone II are the next candidates. How a device is added:
[Adding a device](../../developers/adding-a-device/).
