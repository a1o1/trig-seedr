---
title: Feedback
description: "How to report a hardware test, a bug or an idea, as a GitHub issue."
sidebar: { order: 2 }
---

Feedback goes to the project's GitHub issues: [github.com/a1o1/trig-seedr/issues](https://github.com/a1o1/trig-seedr/issues).
Choose **New issue** and one of three forms:

| Form | For |
|---|---|
| [Hardware report](https://github.com/a1o1/trig-seedr/issues/new?template=hardware-report.yml) | you ran a version on a unit: what worked and what did not, the first checks ([Rytm MKII](../../getting-started/flashing/#first-checks-on-the-unit), [Syntakt](../../getting-started/flashing-syntakt/#first-checks-on-the-unit)) or your own use |
| [Bug](https://github.com/a1o1/trig-seedr/issues/new?template=bug.yml) | something does not do what the manual says, or the unit misbehaves |
| [Idea](https://github.com/a1o1/trig-seedr/issues/new?template=idea.yml) | a change or a feature you would like |

A GitHub account is needed. Before opening one, look whether an issue already describes it; if so, add what you saw
there.

## What makes a report useful

- **The device and its OS:** Analog Rytm MKII with OS 1.74, or Syntakt with OS 1.42.
- **The version, from the boot screen.** While the unit starts it shows `TS08` for 0.8.0 (`T071` for 0.7.1). Say
  which release file you flashed, too (for example `trig-seedr-0.8.0-os1.74.syx` or
  `trig-seedr-0.8.0-st-os1.42.syx`).
- **The steps**, from a known state: the track, its length, the page and the knob values (a photo of the screen
  helps), RECORD on or off, playing or stopped, and the keys you pressed.
- **What happened, and what you expected.** If the manual says otherwise, link the page.
- **Whether it happens again** with the same steps.

If the unit stopped working, say so first, and see Back to stock for the [Rytm MKII](../../getting-started/flashing/#back-to-stock)
or the [Syntakt](../../getting-started/flashing-syntakt/#back-to-stock).

Please do not attach firmware files, Elektron's or ones you built: the `.syx` files contain Elektron's firmware.
