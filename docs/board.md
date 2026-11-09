# The board: Qorvo DWM3001CDK

What a DWM3001CDK is, read from jarededwards's board on 2026-11-07 over SWD and its serial port, with the pins from
Zephyr's board files (`boards/qorvo/decawave_dwm3001cdk`) and Qorvo's board support (`Src/Boards/Src/DWM3001CDK` in the
SDK's source), and the ports from Qorvo's quick start guides.

## What is on it

| Part | What it is | Read on 2026-11-07 |
|---|---|---|
| DWM3001C module | Qorvo's module: the nRF52833, the DW3110 and a printed UWB antenna | |
| Nordic nRF52833 | Cortex-M4F at 64 MHz, 512 KB flash, 128 KB RAM, Bluetooth LE 5 | part 0x52833, variant AAA0, package 0x2008 |
| Qorvo DW3110 | The UWB radio, channels 5 and 9, one antenna: distance, and no angle of arrival on this kit | DEV_ID 0xDECA0302 over SPI |
| ST LIS2DH12 | Three-axis accelerometer on I2C | address 0x19, WHO_AM_I 0x33 |
| SEGGER J-Link OB | The on-board debugger: SWD to the nRF52833, and a serial port | USB 1366:0105, serial number 000760230082 |

A board comes blank: flash and UICR all 0xFF, so its serial port says nothing until it has firmware. Qorvo's guide says
the same ("not shipped preprogrammed").

## Ports and power

- **J9, the J-Link's USB.** Flashing and debugging over SWD, and a serial port wired to the nRF52833's UART (TX P0.19,
  RX P0.15, 115200 8N1): `/dev/cu.usbmodem<J-Link serial number>1` on a Mac, `/dev/cu.usbmodem0007602300821` for
  jarededwards's board. It powers the board. This is the one port to plug in.
- **J20, the nRF52833's own USB.** Qorvo's CLI and UCI firmware talk over it as a USB serial device by default
  (Qorvo's NI Background quick start guide). It powers the board too.
- **Power:** either USB port, the Raspberry Pi header or the VBAT connector. A USB power bank on J9 runs a beacon away
  from a computer.

## Pins (nRF52833)

| What | Pin | Notes |
|---|---|---|
| LED D9, green | P0.04 | Active low. Qorvo's `LED_1` |
| LED D10, blue | P0.05 | Active low. `LED_2` |
| LED D11, red | P0.22 | Active low. `LED_3` |
| LED D12, red | P0.14 | Active low. `LED_4` |
| Button SW2 | P0.02 | Active low; needs the pull-up |
| Button SW1 | P0.18 | Reset, when the UICR's PSELRESET says so |
| UART TX, RX | P0.19, P0.15 | The J-Link's serial port |
| DW3110 SPI: SCK, MOSI, MISO, CS | P0.03, P0.08, P0.29, P1.06 | |
| I2C (the LIS2DH12): SDA, SCL | P0.24, P1.04 | |

Not checked on a board: the DW3110's IRQ, reset and wake-up pins, and the accelerometer's interrupt pins. Zephyr's board
files and Qorvo's `custom_board.h` name them.

## Its identity (FICR)

`scripts/identify` reads it: INFO at 0x10000100 (part, variant, package, RAM, flash), DEVICEID at 0x10000060,
DEVICEADDRTYPE and DEVICEADDR at 0x100000A0. jarededwards's board has the Bluetooth address C2:59:86:E7:C5:3A (random
static), so Qorvo's QANI advertised it as "DWM3001CDK (86E7C53A)", DEVICEADDR[0] in hex.

## Flash, by firmware

| Firmware | What is in flash |
|---|---|
| `uwbprobe` | Itself from 0x0, about 7 KB: no MBR, no SoftDevice |
| Qorvo's QANI, `DWM3001CDK-QANI-FreeRTOS.hex` | Nordic's MBR and SoftDevice S113 from 0x0 to 0x1B4AC, the application from 0x1C000 to 0x4D148, nothing in the UICR. Qorvo's firmware keeps its settings and the UWB calibration in flash too (`docs/qani.md`, Calibration) |

Moving between the two takes a chip erase (`scripts/flash --erase`), which erases the calibration too.
