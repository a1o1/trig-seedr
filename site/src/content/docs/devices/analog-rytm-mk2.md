---
title: Analog Rytm MKII
description: "Trig Seedr on the Analog Rytm MKII (OS 1.74): the beta, hardware-tested."
sidebar: { order: 2 }
---

The Analog Rytm MKII with OS 1.74 is the device the manual describes: every page, key and message in the
[manual](../../manual/concepts/) is the Rytm's. Trig Seedr is a beta on the Rytm ([what that means](../../beta/beta/)).

| | |
|---|---|
| OS | 1.74 only |
| Build | `sh tools/build_release.sh` → `trig-seedr-<version>-os1.74.syx` ([Building](../../getting-started/build/)) |
| Flash, the way back to stock, the first checks | [Flashing a Rytm MKII](../../getting-started/flashing/) |
| Tested | every release in full in analog-emu; on a Rytm MKII with OS 1.74: 0.5.1, 0.6.0 and 0.7.1 ([Tested on hardware](../../getting-started/flashing/#tested-on-hardware)) |
| Tracks | the 12 voice tracks; the FX track is refused (NOT ON THE FX TRACK) |
| LOCK | two lanes over one list of parameters for every track; lane 1 starts on the sample slot |
| Lock limit | 72 lock rows per pattern: a take that needs more is refused (NO LOCK ROWS LEFT) |

Known quirks: YES + SCENE does not save while Trig Seedr is open
([Generate and apply](../../manual/concepts/#generate-and-apply)).

Other devices, and what differs on each: [Devices](../overview/).
