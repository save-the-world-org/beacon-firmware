# Decisions

darkshift's decisions are ADRs in infrared-docs-internal (`docs/decisions/`). An
ADR is never edited to say something else; a new one supersedes it. These shape
this repo:

- **ADR 0015**: commits only from allowlisted accounts, with no AI attribution.
- **ADR 0019**: the agent harness, in trunk mode.

What was learned is in `docs/findings.md`, and infrared-docs-internal's
`docs/beacons/` says where it stands. A choice too small for an ADR goes below,
newest first, with its reason.

---

## 2026-11-09: our changes to Qorvo's firmware are patches, and the SDK stays out

**Context.** Qorvo's QANI is the firmware an iPhone ranges with. Its source is
`LicenseRef-QORVO-2` (sharable with Qorvo's notice, only with Qorvo's chips, and
no "Qorvo" in a derived product's name); the UWB stack it links is
`LicenseRef-QORVO-1` (not to be shared), and the SDK came under Qorvo's licence,
accepted for Darkshift.

**Decision.** No part of the SDK is committed. Our changes are patches to its
`Projects/` and `Src/` files, applied by `qani/build` to a copy each person
unpacks with `scripts/sdk`.

**Consequences.** Building needs each person's own copy of the SDK. A patch shows
only the lines it changes and a few around them, never a QORVO-1 file, and
`make verify` checks both. Moving to a new SDK version means re-basing the
patches on it.

## 2026-11-09: probe-rs, not SEGGER's tools

**Context.** Qorvo's guides flash with SEGGER's J-Flash Lite. SEGGER's J-Link
pack is a Homebrew cask that needs an administrator; probe-rs drives the
board's J-Link OB on its own.

**Decision.** Every script flashes and reads with probe-rs.

**Consequences.** No SEGGER install, and the same commands on any Mac. probe-rs
clears DEMCR when it detaches, so firmware never times with the DWT cycle
counter.
