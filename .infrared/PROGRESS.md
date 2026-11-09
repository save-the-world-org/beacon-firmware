# Progress

Append-only. Newest last. Dated, absolute dates only: "yesterday" means nothing
to the session that reads this in three weeks.

Each entry: **Done** (what changed), **Learned** (what surprised you, which is
the half worth keeping), **Next** (specific enough to start on).

---

## 2026-11-09: the repo, from two days with the first board

**Done.** At jarededwards's ask ("if we need to host our firmware upstream somewhere, let's create a repository"), the
repo, private, with what the first board taught from 2026-11-07: `uwbprobe`, our test firmware, recovered from the
session that wrote it (it rebuilds to the same 6,844 bytes of text that were flashed); `tools/serialtalk.py` and
`tools/blescan`, the serial and Bluetooth tools of those days; `scripts/identify`, `backup` and `flash`, from the
probe-rs commands that worked on the board; `scripts/sdk`, which unpacks Qorvo's SDK into the git-ignored `sdk/` after
checking its SHA-256; `qani/patches/0001-kelsey-finder-name.patch`, which applies to the SDK 1.1.1 source; and
`qani/build`, not run. `docs/board.md`, `docs/qani.md` and `docs/findings.md` hold the board, Qorvo's firmware and the
findings. The harness from template 0.3.2, in trunk mode, checked by `make verify`. The pages for people are
infrared-docs-internal's `docs/beacons/`.

**Learned.** Qorvo's manual says a chip erase resets the board's UWB calibration to defaults that range less
accurately, and this board was erased before QANI went on: a likely reason for distances "a touch off". Qorvo's
application source is `LicenseRef-QORVO-2` and may be shared with its notice; its UWB stack is `LicenseRef-QORVO-1` and
may not be shared at all, so the SDK stays out of git. Qorvo's toolchain, gcc-arm-none-eabi 10.3-2021.10, is an Intel
build on macOS (159 MB), so an Apple silicon Mac needs Rosetta 2 for it. No board was plugged in while this was written:
`scripts/identify`, `backup` and `flash` are the commands that ran on 2026-11-07, put in scripts, and not yet run as
scripts on a board.

**Next.** The owner's go-ahead on a toolchain (`qani/README.md`), then `qani/build` and a flash without `--erase`; on
the next board plugged in, run `scripts/identify`, `scripts/backup` and `make flash-uwbprobe`, and say here that they
worked.

## 2026-11-09: moved to save-the-world-org; the patch renamed

**Done.** The repo moved to the GitHub org save-the-world-org at the owner's word, its first push the owner's. The name
patch is `qani/patches/0001-hero-finder-name.patch` and the beacon's name "Hero Finder", generic at the owner's
preference; `tools/blescan` looks for it.

**Next.** As before: a toolchain go-ahead for `qani/build`. A beacon needs a stable identity the phone can read (its
advertised name carries the low 32 bits of its address today) once people and beacons are registered.

## 2026-11-09, later: the beacon keeps its ID in its name

**Done.** The name patch changes only `BoardName`, so a board advertises "Hero Finder (86E7C53A)", not a bare
"Hero Finder". A review of save-the-world-org's influence-api found that it names a beacon by its `hardwareId`, matched
exactly, and the eight hex digits (DEVICEADDR[0]) are the only ID a phone can read without connecting: QANI takes one
connection at a time and iOS hides the Bluetooth address. A bare name would have made every beacon the same.

**Next.** As before, a toolchain go-ahead for `qani/build`.
