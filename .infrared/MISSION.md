# Mission: beacon-firmware

**What this repo is, in two sentences.** The firmware and tools for Darkshift's UWB beacons, Qorvo DWM3001CDK boards:
our own test firmware, our patches to Qorvo's Nearby Interaction firmware, and the scripts that flash a board and talk
to it. It exists so that anyone with a board and a Mac can program one, and so that the beacon a phone and Ray-Ban
Display glasses seek (spike-rayban's "Hero Finder") runs firmware we can change.

**What we are doing next.** The first build of QANI from Qorvo's source with our patches (`qani/build`), which waits on
a go-ahead for Arm's toolchain, flashed onto a board without erasing it, so that it advertises as "Hero Finder (<its ID>)".
Then the board's UWB calibration, which a chip erase reset to Qorvo's defaults (`docs/qani.md`, Calibration).

**How we know it worked.** `tools/blescan` lists "Hero Finder (86E7C53A)", spike-rayban ranges it as before, and the distance it
reads at a measured two metres is within ten centimetres, written down in `PROGRESS.md`.

## The loop

Read `PROGRESS.md`, change one thing, run `make verify`. A change to firmware is proven only on a board, plugged in by a
person: say which half is proven. Append to `PROGRESS.md`, commit.
