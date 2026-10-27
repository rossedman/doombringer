# Measurements

Every number here is printed by the host tests (`make test`), which run the same DSP core the firmware and the plugin use. CI runs them on every pull request.

| | |
|---|---|
| Five responses against the analog shapes, at fc/4, fc and 4fc | within 0.01 dB |
| Self-oscillation, 1V/oct, −3 V … +5 V | worst error 0.06 cents |
| Self-oscillation level, every mode, resonance 1.01 … 1.1 | 3.8 … 5.8 V |
| Passband at resonance 0.95 | 0.0 dB compensated, −13.6 dB thin |
| Ping, 16 Hz … 8 kHz, resonance 0.8 … 0.99 | 4.3 … 4.9 V |
| Drive 0 / 0.5 / 1, 5 V sine | peak 5.00 V; third harmonic −75 / −12 / −10 dB |
| DOOM at full, 220 Hz saw | −1 octave +6 dB, −2 octaves +7 dB re the fundamental; silent 0.5 s after the input stops |
| HUNGER −1 / 0 / +1: fourth harmonic | −52 / −21 / −12 dB re the fundamental |
| RUIN 0.9 into a 1 V / 5 V saw: inharmonic share | 0.91 / 0.0002 |
| 60 s, every macro at full, 4 cutoffs, with and without input | finite, alive, DC under 0.17 V |
| patch.Init flash / RAM | 107 KB of 128 KB / 37 KB (arm-gcc 13.3) |

## What the suite checks

The five responses against the analog shapes, bass compensation, self-oscillation tracking and its level in every mode, that the ring dies away below the edge, the ping's level across cutoffs, drive (level and harmonics), the undertones (and that they die with the input), HUNGER in both directions, RUIN (harmonic at zero, broken with a quiet input, held with a hot one, and the falling pitch of a pinged hit), stability under random abusive settings with hot input and FM, 60-second runs with every macro at full, and the macros themselves (no jumps in their curves, WRATH's edge, DOOM's undertones arriving in order, RUIN's upper half). Any failed check makes `make test` exit non-zero.
