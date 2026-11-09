# beacon-firmware

Firmware and tools for Darkshift's UWB beacons, Qorvo DWM3001CDK boards: our own test firmware (`uwbprobe/`), our
patches to Qorvo's Nearby Interaction firmware (`qani/`), and the scripts and tools that flash a board and talk to it.
The iPhone app that seeks a beacon is darkshiftio/spike-rayban; the pages for people are infrared-docs-internal's
`docs/beacons/`. darkshiftio/workspace's `CLAUDE.md` applies here too.

## Licences

Qorvo's SDK (DW3_QM33_SDK_1.1.1) came from Qorvo's site under its licence, accepted for Darkshift. Its files carry one
of two licences, named in each file's header (`SPDX-License-Identifier`):

- **`LicenseRef-QORVO-2`**: Qorvo's application source (`Projects/`, `Src/`), BSD-like. It may be shared with Qorvo's
  copyright notice and licence kept, used only with Qorvo's chips, and nothing built from it may have "Qorvo" in its
  name. Our patches to these files live in `qani/patches/`, with the notice untouched.
- **`LicenseRef-QORVO-1`**: proprietary, "dissemination ... strictly forbidden": the UWB stack's headers and libraries
  (`Libs/uwbstack_libs`, the `.a` files). Never in this repo, in any form, not even as a patch's context lines.

So nothing of the SDK is committed: not the zip, its binaries, its PDFs, or its source. `scripts/sdk` unpacks a person's
own copy into the git-ignored `sdk/`, and `make verify` fails if any of it is tracked. Never disassemble, decompile or
otherwise read a Qorvo binary's code (clause 4): work from the source Qorvo ships.

## Boards

- **A board is a person's.** Flashing, a serial session and Bluetooth need it plugged into their Mac. The gate needs no
  board.
- **Back a board up before its first flash** (`scripts/backup`). The read-backs are a board's own data: `backups/` is
  git-ignored.
- **Never poll a board with probe-rs while Qorvo's QANI firmware runs.** Reads every two seconds silenced its
  advertising until a reset. Read once, after a test.
- **`scripts/flash --erase` erases the UWB calibration too.** Qorvo keeps it in the nRF52833's flash; a chip erase puts the
  board back on Qorvo's defaults, which Qorvo says range less accurately (`docs/qani.md`, Calibration).
- **Never time firmware with the DWT cycle counter.** probe-rs clears DEMCR.TRCENA when it detaches, which freezes CYCCNT
  and hangs every delay. Use a TIMER or the RTC (`docs/findings.md`).

## Downloads

Arm's GNU toolchain 10.3-2021.10, which Qorvo's build needs, and Qorvo's own tools (UWB Explorer, uwb-qorvo-tools'
Python packages) are downloads: a person's go-ahead first, naming the file, where it comes from and its size.

Gate: `make verify`. Conventional commits, no AI attribution.

The agent harness for this repo, its map, gate and logs (ADR 0019): @AGENTS.md
