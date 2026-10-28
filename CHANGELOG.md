# Changelog

All notable changes to this project are documented here. The format follows [Keep a Changelog](https://keepachangelog.com/en/1.1.0/), and versions follow [Semantic Versioning](https://semver.org/).

## 0.1.0

The first release. The project version lives in `VERSION` and releases are tagged to match (`v0.1.0`). The VCV plugin keeps its own version in `vcv/plugin.json` (`2.0.0`), because VCV Rack 2 refuses to load a plugin whose version doesn't start with `2.`; its package files are named for that version.

### Added

- A four-pole zero-delay-feedback ladder filter, 4x oversampled. It self-oscillates in tune (0.06 cents from 33 Hz to 8 kHz) at the same level in every mode, and a morph knob moves through LP4, LP2, BP, HP2 and HP4.
- Three character macros on the front panel, each with its own CV jack: WRATH (resonance, then drive), RUIN (the feedback folds, the output's level bends the cutoff, then the loop is pushed past the resonance knob) and DOOM (undertones an octave and two below the input, then a lurch of the cutoff).
- HUNGER, which feeds the input's envelope to the cutoff in either direction, and PING, which strikes the filter with a half-cycle at its cutoff.
- patch.Init firmware with a shift layer and soft takeover, and a VCV Rack 2 plugin under the Portal Electronics brand with a generated sigil panel and the Portal Electronics mark, both built from the same DSP core.
- Host tests for the responses, self-oscillation, ping, drive, the undertones, HUNGER, RUIN, the macros and long runs at full.
- A docs site with a listening page rendered from the DSP: the three macros and five patches.
- CI for every target, and signed releases: the firmware `.bin` and the VCV plugin for Linux x64, Windows x64, macOS x64 and macOS arm64, with cosign signatures and build provenance.
