# Tools for beacon-firmware on macOS: brew bundle --file=Brewfile.
# Apple's clang, from Xcode or its Command Line Tools, compiles uwbprobe and the tools.

# probe-rs drives the board's on-board J-Link over SWD: no SEGGER software is needed.
brew "probe-rs-tools"
# GNU ld, objcopy and size for ARM: clang compiles uwbprobe, GNU ld links it.
brew "arm-none-eabi-binutils"
# The gate's check of the scripts.
brew "shellcheck"
# Qorvo's build (qani/build) needs CMake 3.23 or later, and Arm's GNU toolchain 10.3-2021.10,
# which Homebrew does not carry: qani/README.md says where it comes from.
brew "cmake"
