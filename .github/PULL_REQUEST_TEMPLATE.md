## What and why

<!-- What does this change, and why? Link the issue it closes. -->

## How it was tested

<!-- Host tests, a render you listened to, the firmware on a real patch.Init, VCV Rack. -->

## Checklist

- [ ] `make check` passes (lint, host tests, panel drift)
- [ ] DSP changes come with a host test, and `dsp/` stays heap-free and platform-free
- [ ] Firmware still fits and runs in real time (checked with `make -C hw CPU_METER=1` if the audio path got heavier)
- [ ] README, docs and `CHANGELOG.md` updated for user-facing changes
- [ ] The PR title is a Conventional Commit (`feat(dsp): ...`, `fix(hw): ...`)
