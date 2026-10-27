# patch.Init control map

Four knobs, two layers: the front is the cutoff and the three [character macros](panel.md#the-macros); the shift layer holds the settings. **Hold the button** to reach the shift layer. A parameter only moves once its knob passes through the stored value (soft takeover), so switching layers never makes the sound jump.

| Knob | Front | Hold button (shift) |
|---|---|---|
| 1 (CV_1) + jack CV_5 | **Cutoff**, ten octaves, 16 Hz to 16.7 kHz. Jack is 1V/oct | **Morph**: LP4 → LP2 → BP → HP2 → HP4, continuous |
| 2 (CV_2) + jack CV_6 | **WRATH** | **FM depth** for audio IN R (noon = none; ±4 octaves per 5 V at the ends) |
| 3 (CV_3) + jack CV_7 | **RUIN** | **HUNGER** (noon = none; clockwise the envelope opens the filter, counter-clockwise it closes it) |
| 4 (CV_4) + jack CV_8 | **DOOM** | **Fine tune**, ±1 semitone (noon = in tune) |

Jacks always drive the front-panel parameter.

| Other | Does |
|---|---|
| Button tap | Ping |
| Toggle | Up = bass compensated (the passband holds its level as resonance rises), down = thin (it loses up to 14 dB, like the original ladder) |
| Audio IN L | Audio in |
| Audio IN R | Cutoff FM, audio rate, scaled by the FM depth |
| Gate 1 / Gate 2 | Ping / restart the undertones (both flip-flops high, so they line up with a beat) |
| Audio OUT L / R | Filter (the morph) / four-pole low-pass |
| CV OUT 1 | The input's envelope, 0 to 5 V (the one HUNGER and DOOM follow) |
| CV OUT 2 / LED | The envelope; full while shift is held |

To flash it, see [Getting started](getting-started.md). The constants that depend on your unit are on the [Calibration](calibration.md) page.
