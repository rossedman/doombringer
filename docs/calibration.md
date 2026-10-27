# Calibration

Four constants at the top of `hw/main.cpp` depend on your unit. Check them once after the first flash. If you also run Death's Hand, the numbers are the same for both; copy them across.

| Constant | What it does | How to set it |
|---|---|---|
| `kAudioFullScaleV` | The volts a full-scale codec sample produces, on the way in and on the way out | Set conservatively. If the output is quiet, measure it with a scope and lower the number. RUIN's leash and DOOM's detector both hear the input's level, so this sets where they bite |
| `kInvertAudio` | Flips the Patch SM's inverting audio stages back, on IN L/R and OUT L/R | Doesn't matter for tones; it does for a ramp filtered as CV |
| `kPitchCvScale` / `kPitchCvOffset` | 1V/oct trim for CV_5, the cutoff | Send exactly 1 V and 3 V into CV_5 and use libDaisy's `VoctCalibration` to get the two numbers. It matters when you play the self-oscillation as an oscillator |
| `ps.bassComp` | Toggle direction | If "up" turns out to be thin, invert it |

Rebuild and flash after changing them (`make firmware`, then `make -C hw program-dfu`).
