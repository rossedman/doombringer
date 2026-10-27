# Doombringer

<figure class="diagram" markdown>
![The Doombringer panel: the cutoff and the three macro knobs on top under their sigils, four settings knobs below, the bass switch, the envelope light and the ping button, eight inputs, the Portal mark (a doorway with an eye), and three outputs on a red altar, all over a faint summoning circle.](assets/panel.png){ width="360" }
</figure>

A four-pole ladder filter that summons what isn't there, for the Electrosmith patch.Init and VCV Rack. One portable DSP core runs on both, so a patch you find in one sounds the same in the other. It is the companion to [Death's Hand](https://rossedman.io/deaths-hand/), the evil complex oscillator, and reads the same way: the cutoff and three character macros on the front, four settings on the patch.Init's shift layer, sigils instead of words.

Under the macros it is a careful ladder. The feedback loop is solved without a sample of delay, so it self-oscillates in tune from 33 Hz to 8 kHz (within 0.06 cents) and you can play it as an oscillator. A morph knob moves through five responses: four-pole low-pass, two-pole low-pass, band-pass, two-pole high-pass, four-pole high-pass. The extra parts:

- **WRATH** is the resonance and drive: clean, then ringing, then it screams on its own, then the input stage squares off what you feed it.
- **RUIN** corrupts the resonance: the feedback path folds, the output's own level bends the cutoff, and at the top the loop is pushed past the resonance knob until it breaks free of the input.
- **DOOM** summons undertones: it hears the input's pitch and mixes in squares an octave and two octaves below, which follow the input's envelope and die with it.
- **HUNGER** (shift layer) feeds the input's envelope to the cutoff, opening it on loud notes or, turned the other way, closing it.
- **PING** strikes the filter with a half-cycle at the cutoff, so it rings at the same level at any pitch: a drum with no input at all.

## Where to go

- [Listen](listen.md): the macros and five patches, rendered from the DSP.
- [Getting started](getting-started.md): flash the patch.Init or install the VCV plugin.
- [Reading the panel](panel.md): what each sigil means, and what each macro does along its knob.
- [patch.Init control map](patch-init.md): four knobs, two layers.
- [Five patches](patches.md) to start from.
- [How it works](how-it-works.md) and [what the tests measure](measurements.md).
