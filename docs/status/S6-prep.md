# S6-prep: the Syntakt files and steps for Alan's unit

State: ready for review
Branch: worktree-s6-prep-st (from 9d8fb98)

Nothing here was flashed, sent to or checked on a unit; the files stay in `build/`.

## Done

- **`tools/build_release_st.sh`** (SD6): runs `tools/st_unit.sh` and stops on any failure, then rebuilds the checked
  unit file with elektron-firmware-tool (`-r -V`) with only the OS version field set (T071 for 0.7.1, as the Rytm's
  script names it), checks it again and writes its facts. 0.7.1: `build/trig-seedr-0.7.1-st-os1.42.syx`, container
  1,998,368 of 2,029,392 bytes, packing level 1, in-place margin 10,791, area 0x47a00000-0x47b00000, section 3 the
  only one changed; st_unit 9 PASS, then 4 PASS (every section as the checked file's, the tool's checksums,
  elekloader's syx.verify with version T071, a boot in analog-emu-st); about 5 minutes.
- **`mods/canary-st`** and **`tools/st_canary.sh`**: the memory canary for S6.3. Built in
  ST_AREA=47bf0000-47c00000 (the top 64 KiB of U1's range), it fills 0x478b0000-0x47bf0000 (seedr's area inside it)
  with an address-dependent pattern, 64 KiB a tick, then checks 16 KiB a tick (a whole pass about every 7 s) and
  draws FILL, OK and the passes, or BAD and the first changed address, in the bottom right corner. In analog-emu-st:
  OK through boot, keys, grid recording, PLAY, the TRIG pages and a pattern change (9 passes); BAD 47900000 after a
  word was poked there. 5 PASS; its unit file verifies (level 3, margin 17,294).
- **`docs/flashing-st.md`**, kept off the site: the way back, the builds, the order of the first flashes (S6.1-S6.4),
  15 first checks adapted from `flashing.md`, and a *Tested on hardware* table.
- The plan's S6 split into S6.1-S6.5; the milestone's "For Alan".

## Findings

- elektron-firmware-tool reports a version only when it looks like `1.42`: with T071 its summary leaves the line
  out. No section changes with `-V` (the field is in the container header).

## Open points (unchecked until a unit)

- Where and whether the Syntakt shows the OS version field (Synfact's profile has `version_optional`).
- The STARTUP menu's TRIG 4 = OS UPGRADE, and whether it takes the file over USB (from analog-emu's notes and Synfact).
- Whether Transfer accepts the unit files over USB at this size (U3's open point).
