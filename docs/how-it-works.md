# How it works

One portable DSP core: a four-pole ladder, the three macros' machinery around it, and a 4× oversampled loop.

```text
                               ┌──── RUIN: the output's level raises the cutoff ◄────┐
                               ▼                                                      │
 in ─► + ─► 4× up ─► drive ─► (−) ─► clip ─► pole ─► pole ─► pole ─► pole ─┬─► taps ─► morph ─► 4× down ─► OUT
       ▲                       ▲                                           │                             └─► LP4
       │                       └──── resonance ◄── RUIN: the feedback folds ┘
       │
 DOOM: the input's pitch divided by 2 and 4, enveloped, mixed back in as undertones
```

The DSP is `dsp/Doombringer.hpp`; the macros and the panel mapping are `dsp/Panel.hpp`.

**A ladder in tune.** Each of the four poles is a trapezoidal (TPT) one-pole, and the loop around them is solved exactly each sample, with no unit delay in the feedback. The cutoff is prewarped with a Padé approximation of the tangent (7×10⁻⁷ relative error at 20 kHz), so resonance peaks, and self-oscillation, land on the cutoff at every pitch.

**A scream at one level.** The input stage clips, which is what bounds self-oscillation. It is linear to half its clip level, so the scream barely grows the further past the edge the resonance goes. Each mode hears the loop through its own gain at the cutoff, so the clip level is set from that gain, and every mode screams at the same level.

**Five responses from four poles.** The morph mixes the input stage and the four pole outputs (Oberheim's trick): LP4 is the last pole, HP4 is the binomial `u − 4y₁ + 6y₂ − 4y₃ + y₄`, and the stops between are their neighbours. Bass compensation wins back the `1 / (1 + k)` the loop costs, on the low-pass taps only, since the high-pass ones never paid it.

**RUIN without lock-up.** A folded feedback path has stable states away from zero, and the filter would latch onto one as a stuck DC level. Only the AC part of the feedback folds; its DC stays linear. The self-FM follows the output's magnitude rather than its sign, so the loop stays symmetric and can't pump itself to one side either.

**DOOM's undertones.** A flip-flop on the input's rising zero crossings is a square an octave down; a second flip-flop on the first is two octaves down. The detector hears the fundamental through two 600 Hz poles, with hysteresis that scales with the envelope, so quiet tails can't chatter it. An edge lands between samples, so that sample takes the average of the two levels across it instead of a hard jump. The squares are scaled by the input's envelope, so silence stays silent, and rounded above 1.2 kHz.

**HUNGER's envelope** is a peak follower with a 1 ms attack and an 80 ms release. At full HUNGER a 5 V envelope moves the cutoff five octaves.

**Drive** is a separate saturator in front of the ladder, `tanh(D·x) / tanh(D)`: a 5 V peak stays 5 V and everything quieter is pushed up toward it, so turning it up adds harmonics, not level.

**PING** is a half-cycle of a sine at the cutoff. A short click would ring louder at high cutoffs than low ones; a half-cycle at the ringing frequency puts the same energy where it rings, at any pitch.

**Oversampling.** The ladder runs at 4×. A filter has to process what comes in, so it upsamples first, with a polyphase version of the same 129-tap Kaiser FIR that decimates on the way out. The FM input has its own upsampler, which idles while the FM depth is at noon.

**Waking up.** A whisper of noise (−94 dB) lets self-oscillation start on its own. It grows from there over a few hundred cycles, so at a low cutoff it can take a couple of seconds. Ping it to wake it at once.

## Layout

```text
dsp/      portable engine: no heap, no platform headers, volts in / volts out
hw/       Electrosmith patch.Init() firmware (libDaisy)
vcv/      VCV Rack 2 plugin
test/     host tests and WAV renders
```

The firmware, the plugin and the host tests all include `dsp/` unchanged, so what the tests measure is what the module plays.
