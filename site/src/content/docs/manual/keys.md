---
title: Keys and messages
description: "Every key Trig Seedr uses, and every message it shows on the strip."
sidebar: { order: 14 }
---

## Keys

| Key | Does |
|---|---|
| TRIG on the EUCLIDEAN page | opens Trig Seedr |
| TRIG held + PAGE | opens Trig Seedr from any page |
| TRIG on Trig Seedr | closes it (stock's TRIG page); a pending take is not applied |
| a parameter page key (SRC, SMPL, FLTR, AMP, LFO, FX) or a menu key | closes it; a pending take is not applied |
| LEFT / RIGHT | the previous / next page |
| UP / DOWN | LOCK: lane 1 or lane 2; on other pages nothing |
| FUNC + UP / FUNC + DOWN | TURING's live page: remember the loop / bring it back |
| knobs A–H | the page's settings |
| FUNC + knob | a range's low end (VEL, LEN, NOTE, RNGE, CVAL, RATE); NOTE's SCALE knob: ROOT; SEED in steps of 100; other knobs with more than 20 values in steps of 10 |
| YES | generate: write a take to hear it (needs RECORD on); on TOOLS, run the tool; on TURING, start the live run (again: rewrite every step from the loop) |
| FUNC + YES | apply: keep the take (the only way a take stays) |
| NO | with a take pending: back to the original; otherwise: close Trig Seedr |
| PAGE, the TRIG keys, the transport | stock's: PAGE still picks which 16 steps the TRIG keys show |
| YES + SCENE (stock's save pattern) | does not save while Trig Seedr is open: [known quirk](../concepts/#generate-and-apply). Close Trig Seedr first |

What each key does to a take, step by step: [Generate and apply](../concepts/#generate-and-apply).

On the Syntakt, the keys are the same; the keys that close Trig Seedr are SYN, FLTR, AMP, LFO, FX,
GLOBAL and MENU ([Syntakt](../../devices/syntakt/#opening-and-closing)).

## Messages

A message replaces the preview strip for 2 seconds. When YES could not write, nothing was written.

| Message | Means |
|---|---|
| RECORD TO GENERATE | YES with grid recording off: nothing is written. Turn RECORD on (not RECORD + PLAY) |
| APPLIED | FUNC + YES kept the take |
| BACK TO THE ORIGINAL | NO put the track as it was before the first YES |
| NOT APPLIED | the take was pending when you changed track or pattern, closed Trig Seedr or turned RECORD off: the original is back |
| NOT ON THE FX TRACK | Trig Seedr does not write the FX track (also shown in place of the strip) |
| EUCLID IS ON: TURN IT OFF | the track is in stock's Euclidean mode; turn EUC off on the EUCLIDEAN page (also shown in place of the strip) |
| NO LOCK ROWS LEFT | the take needs a parameter lock row the pattern does not have (72 per pattern). The Rytm only: the Syntakt has no lock-row limit |
| SETTING OUT OF RANGE | the recipe has a setting the engine refuses, such as both lock lanes on one parameter |
| NOT AVAILABLE YET | the recipe uses a feature this build does not have |
| NO TRACK DATA | the track could not be read |
| LIVE | TURING: the live run started; while it runs the page shows LIVE and the step |
| YES: START | TURING: YES starts the live run |
| RESTARTED | TURING: YES while running rewrote every step from the loop |
| REMEMBERED, RECALLED, NOTHING REMEMBERED | TURING: FUNC + UP kept the loop; FUNC + DOWN brought it back, or there was none |
| YES TO START | TURING: FUNC + UP or FUNC + DOWN with no live run |
| FAILED | the take could not be written for another reason; please report it ([Feedback](../../beta/feedback/)) |
| ROLL 12 | YOLO played roll 12 |
| ROTATED 13 TRACKS | ROT ALL rotated that many tracks |
| a groove's name, or NO GROOVE | TIME's GRV is turning |
| a range in full (VEL 60-127) | a range too wide for its cell is turning |
