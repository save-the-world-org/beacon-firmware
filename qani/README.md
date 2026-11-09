# Building QANI with our patches

Qorvo's Nearby Interaction firmware, built from the SDK's source with the patches in `patches/`:

| Patch | What it does |
|---|---|
| `0001-kelsey-finder-name.patch` | The beacon advertises as "Kelsey Finder", not "DWM3001CDK (86E7C53A)", and no name in the build says "Qorvo" |

**Not built yet (2026-11-09).** `qani/build` follows Qorvo's README, "Building from command line", but has not run: the
toolchain below is not installed on any Mac yet. The patch applies to the SDK 1.1.1 source (`patch --dry-run`, and
`make verify` checks it whenever `sdk/` is unpacked).

## What it needs

- **Qorvo's SDK**, unpacked by `scripts/sdk <path to DW3_QM33_SDK_1.1.1.zip>` into `sdk/`. Never committed (`CLAUDE.md`,
  Licences).
- **CMake 3.23 or later** (`brew bundle --file=Brewfile`).
- **Python 3.10 or later.** The first build makes `.build/qani/venv` and installs Qorvo's `requirements.txt` into it from
  PyPI: poetry 1.7.1, poethepoet 0.24.4, pyyaml, and Qorvo's own `Projects/Common/scripts`.
- **Arm's GNU toolchain**, which Homebrew's `arm-none-eabi-binutils` is not (that has no compiler). Two ways, both a
  download, so a person's go-ahead first:
  - **Qorvo's version, 10.3-2021.10.** Qorvo's README names it, and its libraries were built with it:
    `https://developer.arm.com/-/media/Files/downloads/gnu-rm/10.3-2021.10/gcc-arm-none-eabi-10.3-2021.10-mac.tar.bz2`,
    159 MB (158,961,466 bytes). Its compiler is built for Intel Macs, so on Apple silicon it needs Rosetta 2, which
    jarededwards's Mac does not have installed; installing Rosetta asks for an administrator and Apple's licence. Qorvo
    unpacks it into `/opt/gcc/` and puts its `bin/` on PATH; `ARM_TOOLCHAIN=/opt/gcc/gcc-arm-none-eabi-10.3-2021.10`
    does without the PATH change.
  - **A current Arm GNU Toolchain, native on Apple silicon**, such as Homebrew's cask `gcc-arm-embedded`. Untried with
    Qorvo's libraries: it may link, or it may not. `qani/build` warns when the compiler is not 10.3.1.

## Build and flash

```bash
scripts/sdk ~/Downloads/DW3_QM33_SDK_1.1.1.zip     # once
qani/build                                          # Release, into .build/qani/source/BuildOutput/QANI/FreeRTOS/DWM3001CDK/Release/
scripts/flash .build/qani/source/BuildOutput/QANI/FreeRTOS/DWM3001CDK/Release/<the .hex it lists>
```

Leave `--erase` off when the board already runs QANI: the application changes and the calibration stays. A board on
`uwbprobe` or another of Qorvo's firmware needs `--erase` first, which puts it back on Qorvo's default calibration
(`docs/qani.md`, Calibration).

Then `tools/blescan` should list "Kelsey Finder", and spike-rayban finds it by Qorvo's service as before.
