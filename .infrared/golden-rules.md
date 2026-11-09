# Golden rules

The rules a reviewer checks first, each with its source. The full rules are in
`CLAUDE.md` and the workspace's `CLAUDE.md`; where this list and they disagree,
they win and this list is wrong.

1. **Nothing of Qorvo's SDK in git.** Not the zip, its binaries, its documents
   or its source; our changes are patches to `Projects/` and `Src/` only, with
   Qorvo's notices untouched. *(CLAUDE.md, Licences; scripts/verify)*
2. **Never read a Qorvo binary's code.** No disassembly or decompiling; work
   from the source Qorvo ships. *(Qorvo's licence, clause 4)*
3. **No "Qorvo" in the name of anything we build.** *(Qorvo's licence,
   clause 5; scripts/verify)*
4. **Never poll a board with probe-rs while QANI runs.** Read once, after a
   test. *(docs/findings.md, 2026-11-07)*
5. **Say what an erase costs.** `scripts/flash --erase` erases the UWB
   calibration too. *(docs/qani.md, Calibration)*
6. **Say which half is proven.** The gate proves a build; a board proves the
   firmware, plugged in by a person. *(.infrared/MISSION.md)*
