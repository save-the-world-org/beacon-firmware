# beacon-firmware

Firmware and tools for Darkshift's UWB beacons, Qorvo DWM3001CDK boards: a DWM3001C module (Nordic's nRF52833 for
Bluetooth LE, Qorvo's DW3110 for Ultra Wideband) with an ST LIS2DH12 accelerometer and an on-board J-Link. A phone finds
a beacon over Bluetooth, then measures the way to it over Ultra Wideband. save-the-world-org/spike-rayban is the iPhone app
that seeks one ("Hero Finder"), and infrared-docs-internal's `docs/beacons/` are the pages for people, from plugging
in a board to the phone and the glasses.

| Where | What |
|---|---|
| `uwbprobe/` | Our own test firmware: a serial shell (`id`, `led`, `uwb`, `accel`, `watch`, `temp`, `uptime`, `reset`) with no SDK, built by Apple's clang |
| `qani/` | Qorvo's Nearby Interaction firmware (QANI), which an iPhone ranges with: our patches to Qorvo's source, and its build |
| `tools/serialtalk.py` | Talks to a serial port, with only Python's standard library |
| `tools/blescan` | Lists the beacons a Mac hears over Bluetooth |
| `scripts/` | `identify`, `backup`, `flash`, `sdk` (unpacks Qorvo's SDK), `verify` (the gate) |
| `docs/` | `board.md` (the hardware), `qani.md` (Qorvo's firmware: its Bluetooth protocol, calibration, gotchas), `findings.md` |

Qorvo's SDK is never in this repo: it is licensed to Darkshift, and its UWB stack may not be shared (`CLAUDE.md`,
Licences). `scripts/sdk` unpacks your copy into the git-ignored `sdk/`.

## Start here

You need a Mac, a DWM3001CDK and a micro-USB cable that carries data.

1. **Tools:** `brew bundle --file=Brewfile` (probe-rs, GNU's ARM binutils, shellcheck, CMake). Apple's clang, from Xcode
   or its Command Line Tools, compiles the firmware. No SEGGER software: probe-rs drives the board's J-Link.
2. **Plug in J9**, the J-Link's USB port. `probe-rs list` should show `J-Link -- 1366:0105:<serial number>`, and
   `ls /dev/cu.usbmodem*` its serial port, `/dev/cu.usbmodem<serial number>1`.
3. **Identify it:** `scripts/identify` prints the chip, its Bluetooth address and whether it has firmware.
4. **Back it up** before its first flash: `scripts/backup`, into `backups/` (git-ignored).
5. **Our firmware:** `make flash-uwbprobe`. The LEDs run twice round and D9 blinks a heartbeat. Then talk to it:

   ```bash
   tools/serialtalk.py /dev/cu.usbmodem<serial number>1 --send 'id\r' --send 'accel\r'
   screen /dev/cu.usbmodem<serial number>1 115200     # by hand; quit with Ctrl-A, then K
   ```

   SW2 lights D10 and says "button SW2 down"; `watch` streams the accelerometer.
6. **Qorvo's firmware, for an iPhone:** `scripts/sdk <path to DW3_QM33_SDK_1.1.1.zip>`, then
   `scripts/flash --erase sdk/SDK/Binaries/DWM3001CDK/DWM3001CDK-QANI-FreeRTOS.hex`. It advertises once a second as
   "DWM3001CDK (<address>)", and `tools/blescan` lists it. Range with Qorvo's iPhone app or spike-rayban; close one before
   the other, since it takes one connection at a time.

Two boards and no phone: Qorvo's CLI firmware (`DWM3001CDK-CLI-FreeRTOS.hex`) ranges board to board, `INITF` on one and
`RESPF` on the other, over J20, the nRF52833's own USB (`docs/qani.md`, Calibration). Not tried yet.

## Rules that cost something

- **Never poll a board with probe-rs while QANI runs**: it silenced its advertising until a reset.
- **`--erase` erases the UWB calibration** that Qorvo's firmware keeps in flash: a board then ranges on Qorvo's
  defaults, less accurately (`docs/qani.md`).
- **Never time firmware with the DWT cycle counter**: probe-rs stops it when it detaches (`docs/findings.md`).

## The gate

`make verify`: the harness, shellcheck, `uwbprobe` built, the tools compiled, the patches checked against Qorvo's licence,
and nothing of Qorvo's SDK tracked. It needs no board and no network.
