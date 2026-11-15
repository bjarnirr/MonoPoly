# MonoPoly — MPC SDK plugin project

This folder is a plugin project for the inMusic MPC Plugin Builder SDK supplied by the user.

## Target
- Akai MPC Live MK1 (Gen1 ARM32)
- MPC OS 3.10.1 or later
- Native instrument plugin, 0 audio inputs / 2 outputs
- MPC VST2 ABI, SDK framework adapter

## Build
Place this `MonoPoly/` folder inside the SDK's `plugins/` directory, then from the SDK root run:

```sh
python3 tools/build.py plugins/MonoPoly
```

The SDK's default Zig toolchain builds the native host-simulator library and the ARM32 + ARM64 libraries, generates the MPC plugin folder and validates it. For the MPC Live MK1, the required binary is `VST/monopoly-arm32.so`.

Before copying to the MPC, run:

```sh
python3 tools/validate.py plugins/MonoPoly
```

The install folder produced by the SDK is:

`plugins/MonoPoly/build/MonoPoly Project - VST - MonoPoly/`

Copy that whole folder into `VST2 Plugins/` at the root of a USB/SD drive. On the MPC enable **Preferences → Enhancements (Beta) → Enable Enhancements** and **External Plugin Support**.

## Current build status

- Native Linux shared library: built successfully.
- SDK host simulator: 19 passed, 0 failures, 0 warnings.
- Touch UI: generated (5 pages, 42 controls).
- ARM32/ARM64 cross-build: not completed in this environment because Zig is not installed and outbound network access prevented the SDK from downloading its `ziglang` package. Install Zig locally or provide a preinstalled SDK toolchain, then run the build command above.

## Sound engine caveat

This is an independent circuit-inspired recreation, not a port of Korg's proprietary VST binary or source. The uploaded VST archive contained a compiled macOS binary and resources, not source code. The service manual informs the intended architecture, but this prototype's four-stage nonlinear low-pass filter is not yet a component-accurate SSM2044 simulation. Oscillator waveforms are basic digital models and need further aliasing / calibration work.

## Included
- `src/MonoPolyPlugin.cpp`: MPC SDK instrument adapter, 42 parameters, 4 programs, MIDI handling.
- `src/MonoPolyEngine.cpp/.h`: portable DSP engine.
- `plugin.json`: MPC plugin manifest (UID MnPl).
- `ui.json`: touch UI layout.
- `Plugin Skins/`: generated TUI and Q-Link files.
