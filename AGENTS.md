# AGENTS.md: beacon-firmware

This file stays at the repo root and stays short. Every agent reads it: Claude
Code through `CLAUDE.md`, which imports it, Codex directly, and Infrared's runner
because its prompt says to. The rules for working here are in `CLAUDE.md` and in
the workspace's `CLAUDE.md` one directory up; this file is the map.

## Where things are

Paths are declared once, in `.infrared/harness.json`. Resolve one rather than
typing it: `node .infrared/bin/bb-harness.mjs path <key>`.

| Key | What it holds | When you read it |
|-----|---------------|------------------|
| `mission` | what this repo is, what is next, how you would know it worked | first, every session |
| `progress` | done, learned, next: append-only and dated | every session; append before you finish |
| `decisions` | where decisions live (ADRs in infrared-docs-internal), and the few too small for one | before proposing something they settle |
| `goldenRules` | the rules a reviewer checks first, each with its source | before the first commit |

`README.md` says what is here and how to flash a board. `docs/board.md` is the
hardware, `docs/qani.md` Qorvo's Nearby Interaction firmware (its Bluetooth
protocol, calibration, building it), and `docs/findings.md` what was learned.
The firmware is `uwbprobe/` (ours) and `qani/` (our patches to Qorvo's); the
tools are `tools/` and `scripts/`.

## The gate

`make verify` is free and needs no board: the harness, shellcheck, `uwbprobe`
built with clang and GNU ld, the tools compiled, the patches checked for
Qorvo's notices, and a check that nothing of Qorvo's SDK is tracked. Flashing
a board (`scripts/flash`) and anything over its serial port or Bluetooth needs
the board plugged in: a person's. There is no CI.

## How a change lands

Straight to `main` (the manifest's `delivery.mode` is `trunk`, ADR 0019): run the
gate and read its output, `git status`, stage by path, one conventional commit
per change, rebase on `origin/main`, push. Nothing is built or deployed from a
commit. Commit as the allowlisted account this clone is set up for (ADR 0015),
with no AI attribution of any kind.

Inside an Infrared run there is no git: the workspace is a plain copy and the runner commits for you. Skip the git steps. A run has no board and no Apple toolchain: say in your report that the gate could not run; this repo has no CI, so a person runs it.

## Never

| Never | Why |
|-------|-----|
| commit any part of Qorvo's SDK: the zip, its binaries, its documents, its source, or a file under `LicenseRef-QORVO-1` in any form | it is licensed to Darkshift, and its UWB stack may not be shared at all (`CLAUDE.md`, Licences) |
| disassemble or decompile a Qorvo binary | Qorvo's licence forbids it; work from the source they ship |
| poll a board with probe-rs while Qorvo's QANI firmware runs | reads every two seconds silenced its Bluetooth advertising until a reset (`docs/findings.md`) |
| `git push --force`, delete `main`, `--no-verify`, `git reset --hard`, `git clean -f`, `rm -rf` | it destroys work nobody can get back; `.claude/hooks/guard.mjs` refuses these and says what to do instead |
| `git add -A`, `git add .`, `git commit -a` | the checkouts are shared, and another session's change is not yours to commit |
| edit a file under `.infrared/bin/`, `.infrared/scripts/` or `.claude/` | they are vendored from the harness template at the version in the manifest, and `make verify` fails on a hand edit |
