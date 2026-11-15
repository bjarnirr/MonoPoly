# MonoPoly SDK build and test report

Date: 2026-11-15

## Native build
Command:
`python3 tools/build.py plugins/MonoPoly --target native --toolchain gcc`

Result: PASS. Built `build/native/monopoly.so` and extracted 42 host parameters.

## Host simulator
Command:
`tools/host_sim plugins/MonoPoly/build/native/monopoly.so test --notes 48,55,60,64 --seconds 3`

Result: PASS — 19 passed, 0 failures, 0 warnings.

Checks included:
- MPC VST2 load sequence and plugin identity
- 0-in / 2-out instrument configuration
- 4 built-in programs
- MIDI note rendering
- block sizes 32, 128 and 512
- parameter sweep and state chunk round-trip
- all-notes-off silence
- no NaN/Inf, clipping or denormal output
- average desktop test CPU 0.56% per block (desktop-only reference, not an MPC measurement)

## Touch UI
`tools/gen_ui.py` generated `Plugin Skins/TUI.json` and `Plugin Skins/Q-Links.json`: 5 pages and 42 controls.

## ARM cross-build
NOT RUN / BLOCKED. The environment has no Zig, ARM GNU cross-compilers, Docker or Podman. The SDK attempted to install `ziglang`, but outbound network access was unavailable. No ARM32 `.so` is included and this package must not be installed on the MPC yet.
