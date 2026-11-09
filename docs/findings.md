# Findings

What was learned about the DWM3001CDK, Qorvo's firmware and the tools, newest last. Each says how it was found.

## 2026-11-07: talking to a blank board

- **probe-rs drives the board's J-Link without SEGGER's software** (probe-rs-tools 0.32.0 from Homebrew). `probe-rs list`
  shows "J-Link -- 1366:0105:000760230082", and `probe-rs info --protocol swd` "Detected chip: nRF52833_xxAA". SEGGER's
  J-Link pack, which Qorvo's guides use, is a cask that needs an administrator; nothing here needs it.
- **Apple's clang compiles for the board** (`--target=thumbv7em-none-eabi`), and Homebrew's arm-none-eabi-binutils
  links. That is all `uwbprobe` needs: no SDK, no C library, registers by address.
- **The board came blank**: flash and UICR all 0xFF (backed up then), so the serial port said nothing.
- **The DWT cycle counter hangs every delay after probe-rs detaches.** probe-rs clears DEMCR.TRCENA when it lets go,
  which stops CYCCNT; `uwbprobe`'s first build waited forever in its first delay. It times with TIMER1 at 1 MHz since.
  The same clearing means DWT program-counter sampling does not survive between probe-rs calls; to see where the CPU is,
  `probe-rs gdb` with `lldb --batch -o "gdb-remote 1337"`.
- **`probe-rs debug` needs a terminal of its own** (a TTY), so it does not run from a script.

## 2026-11-07: Qorvo's QANI and an iPhone

- **Qorvo's Nearby Interaction firmware is only in the SDK now** ("DW3xxx & QM3xxx SDK v1.1.1", the DWM3001CDK page's
  Design Resources): a form, Qorvo's licence, a reCAPTCHA, and a one-time link by email.
- **It ranged with an iPhone 17 Pro** in Qorvo's iPhone app ("Qorvo Nearby Interaction", App Store id1615369084), after
  `probe-rs erase` and `probe-rs download --binary-format hex --verify`.
- **The first connection crashed** with error 71 (`_ERR_Cannot_Alloc_Memory` in `fira_start_niq`), read from its
  error handler's arguments with lldb. A reset fixed it.
- **Polling it silenced it.** A watcher that read the board with probe-rs every two seconds stopped its advertising,
  and its "busy" answers were misread as error codes. A reset brought advertising back. Rule since: read once, after.

## 2026-11-08: seeking it with hero-goggles-ios

- hero-goggles-ios's seek ranged it, with the distance in jarededwards's Ray-Ban Display glasses; the lens's top line then
  showed the beacon's Bluetooth name, and shows the app's own title since, "Hero Finder" from
  2026-11-09 (hero-goggles-ios `docs/findings.md`). A test on a
  power bank, away from the Mac, was next.
- The distances seemed "a touch off and stuck at weird increments" to jarededwards. The app's spoken rounding was
  coarse, and is finer since; the app logs every reading for the rest (hero-goggles-ios, `seek-*.csv`).

## 2026-11-09: calibration, licences, the toolchain

- **The board ranges on Qorvo's default calibration.** Qorvo keeps the UWB calibration in the nRF52833's flash, and a
  chip erase puts the defaults back, which Qorvo says range less accurately (developer manual, "Calibration important
  notes"). This board was blank and then chip-erased, so the defaults are all it has had (`docs/qani.md`).
- **Qorvo's SDK has two licences.** Its application source (`Projects/`, `Src/`: 236 files) is `LicenseRef-QORVO-2`,
  BSD-like, sharable with its notice; the UWB stack's headers and libraries (`Libs/uwbstack_libs`: 114 files) are
  `LicenseRef-QORVO-1`, never to be shared. Hence `CLAUDE.md`, Licences.
- **Qorvo's toolchain is an Intel build on macOS.** gcc-arm-none-eabi 10.3-2021.10 for macOS is x86_64 only, 159 MB: on
  Apple silicon it needs Rosetta 2 (`qani/README.md`).
- **The CLI firmware lets two boards range without a phone** (`INITF`, `RESPF`; developer manual, chapter 7). Over J20,
  the nRF52833's own USB, by default. Not tried yet.
