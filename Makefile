# make verify is the gate (the harness first). README.md says what each target does; a board on J9 is needed for the
# flash and scan targets.
.PHONY: verify harness uwbprobe flash-uwbprobe scan

verify:
	scripts/verify

harness:
	node .infrared/bin/bb-harness.mjs validate

uwbprobe:
	uwbprobe/build

flash-uwbprobe: uwbprobe
	scripts/flash .build/uwbprobe/uwbprobe.elf

scan:
	tools/blescan 20
