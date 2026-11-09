# Qorvo's Nearby Interaction firmware (QANI)

QANI is the firmware an iPhone ranges with: Qorvo's implementation of Apple's Nearby Interaction accessory protocol for
its boards. It ships ready-made in Qorvo's SDK (`SDK/Binaries/DWM3001CDK/DWM3001CDK-QANI-FreeRTOS.hex`) and as source
(`Projects/FreeRTOS/QANI` in the SDK's firmware zip), with Qorvo's UWB stack and Nearby Interaction library as
prebuilt libraries. This page is what was learned running it on a DWM3001CDK, with an iPhone 17 Pro, from 2026-11-07.

## What it does

On power-up it starts Nearby Interaction and Bluetooth, and advertises. When a phone connects and asks, it sends its
accessory configuration data; when the phone answers with its own configuration, it starts a FiRa two-way ranging
session as the responder, and the phone measures the distance. Apple's framework, on the phone, does the rest.

Seen on jarededwards's board, with the SDK 1.1.1 build:

- It advertises once a second and never stops: Qorvo's QNIS service in the advertisement, the name in the scan response.
- It takes one connection at a time. A phone that holds it, such as Qorvo's iPhone app left open, keeps every other
  phone out.
- It logs nothing on the serial port or over RTT, and drives none of the LEDs. A board on QANI looks dead; Bluetooth is
  the only way to see it (`tools/blescan`).

## Its Bluetooth protocol

The Qorvo NI Service (QNIS), from `Src/Comm/Src/BLE/nrfx/qnis/`: a 128-bit base `2E93xxxx-6A61-11ED-A1EB-0242AC120002`.

| UUID | What | Properties |
|---|---|---|
| `2E938FD0-6A61-11ED-A1EB-0242AC120002` | The service, in the advertisement | |
| `2E93998A-6A61-11ED-A1EB-0242AC120002` | RX: the phone writes messages here | Write, write without response |
| `2E939AF2-6A61-11ED-A1EB-0242AC120002` | TX: the beacon's messages | Notify |
| `2E93941C-6A61-11ED-A1EB-0242AC120002` | SEC: reading it starts pairing ("just works") | Read |

A message is one byte of ID, then its data:

| ID | From | Message |
|---|---|---|
| `0x0A` | phone | Initialize: send your accessory configuration data |
| `0x01` | beacon | Accessory configuration data, for `NINearbyAccessoryConfiguration(data:)` |
| `0x0B` | phone | Configure and start, followed by the shareable configuration Nearby Interaction gave the phone |
| `0x02` | beacon | UWB did start |
| `0x0C` | phone | Stop |
| `0x03` | beacon | UWB did stop |

Apple's Nearby Interaction Service (ANIS, `48FE3E40-0817-4BB2-8633-3073689C2DBA`, its accessory configuration data in
`95E8D9D5-D8EF-4721-9A4E-807375F53328`) is the one iOS uses for ranging in the background, with the beacon paired
(Qorvo's NI Background quick start guide; iOS 16 and later). spike-rayban uses QNIS only.

## Calibration

Qorvo's firmware keeps the UWB calibration (the antenna delay that distance depends on, transmit power, the antenna
set) in the nRF52833's flash. It survives a power cycle and a firmware update, but not a chip erase. Qorvo's developer
manual, "Calibration important notes": a board uses Qorvo's default calibration when it is new, when its whole flash is
erased, after the CLI's `RESTORE` or after `reset_calibration`, and "using the default calibration leads to decreased
ranging accuracy".

jarededwards's board came blank and was chip-erased before QANI went on, so it ranges on the defaults. That fits the
distances it gave on 2026-11-08, "a touch off". Two ways to calibrate it, neither tried yet:

- **On the beacon** (Qorvo's manual, "Distance and Angle Calibration"): two boards on Qorvo's CLI firmware, about two
  metres apart; `INITF` on one and `RESPF` on the other; stop the one being calibrated, adjust its antenna delay with
  `CALKEY ant<x>.ch<y>.ant_delay <value>` (antenna path x, channel y; `LISTCAL` lists every key), and repeat until the
  distance reads true. Then flash QANI without `--erase`, so the calibration stays. Over UCI the same is Qorvo's
  `load_cal` and `set_cal` scripts, with the SDK's DWM3001CDK file `dual-hoe_non_aoa.json`.
- **On the phone**: measure the error at a few known distances from spike-rayban's log and subtract it in the app.
  Nothing changes on the beacon, and each beacon needs its own numbers.

The DW3110 also holds antenna delays measured in Qorvo's production for the kit's antenna, in its one-time
programmable memory (OTP addresses 0x1A for channel 5, 0x1C for channel 9).

## Gotchas

- **Never poll it with probe-rs.** Reads every two seconds silenced its advertising; a reset brought it back. Read once,
  after a test.
- **Erase before switching to it.** Qorvo's forum: QANI over another firmware can fail at start in `fds_init` (0x860A),
  a fatal error. `scripts/flash --erase` avoids it, at the cost of the calibration.
- **Its first connection once crashed** into its error handler with error 71, `_ERR_Cannot_Alloc_Memory`, in
  `fira_start_niq`. A reset fixed it, and it has not come back. Its cause is not known.
- **Only one phone at a time.** Close Qorvo's iPhone app before another app connects.

## Where to change what

In the SDK's source (`sdk/source/` after `scripts/sdk`), all `LicenseRef-QORVO-2`:

| To change | Where |
|---|---|
| The advertised name | `Projects/FreeRTOS/QANI/DWM3001CDK/ProjectDefinition/ProjectName.c` (`BoardName`, which `qani/patches/0001-hero-finder-name.patch` sets to "Hero Finder"); `Projects/FreeRTOS/QANI/Common/src/main.c` adds the ID, `" (%08X)"` of DEVICEADDR[0]: keep it, it is the beacon's ID |
| Advertising, the services | `Src/Comm/Src/BLE/nrfx/ble.c`, `Src/Comm/Src/BLE/nrfx/qnis/` |
| The ranging session | `Projects/FreeRTOS/QANI/Common/src/fira/` (`fira_niq.c`) |
| LEDs and the button | `Src/Boards/Src/DWM3001CDK/Common/custom_board.h` names them; QANI drives none |
| The accelerometer | Nothing in QANI reads it: the LIS2DH12 on I2C (SDA P0.24, SCL P1.04) needs a TWI driver turned on in `Projects/FreeRTOS/QANI/DWM3001CDK/ProjectDefinition/sdk_config.h` and a task of its own |

Building it: `qani/README.md`.
