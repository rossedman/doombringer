# Listen

Every sound on this page is rendered by the same DSP core the patch.Init firmware and the VCV plugin run, through the same panel mapping, so each one is knob positions, CVs and an input you can patch yourself. They are dry (no reverb, no effects), mono, normalised to −1 dBFS, and rebuilt from the current code every time the docs are published. Above each player, the spectrogram shows time left to right and frequency from 30 Hz to 16 kHz on a log scale, so the low end isn't squeezed into a sliver; the WAV link downloads the 48 kHz original.

## The three macros

Each macro on its own, swept from nothing to full. Each clip's input is in [`test/patches.hpp`](https://github.com/rossedman/doombringer/blob/main/test/patches.hpp): a saw, a sequence, a chord, or nothing at all.

<div class="clip" markdown>
<div class="clip__head" markdown>
**WRATH** A1 saw at 1.5 V. Ringing, then past the edge the filter screams over the saw.
</div>
![Spectrogram of WRATH: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/wrath_sweep.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/wrath_sweep.mp3"></audio><a class="clip__wav" href="../assets/audio/wrath_sweep.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**RUIN** A1 saw at 1 V, WRATH at half. The resonance folds, then bends, then breaks free of the input.
</div>
![Spectrogram of RUIN: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/ruin_sweep.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/ruin_sweep.mp3"></audio><a class="clip__wav" href="../assets/audio/ruin_sweep.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**DOOM** A2 saw. An octave below arrives, then two, then the undertone lurches the cutoff.
</div>
![Spectrogram of DOOM: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/doom_sweep.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/doom_sweep.mp3"></audio><a class="clip__wav" href="../assets/audio/doom_sweep.wav" download>WAV</a></div>
</div>

## Five patches

The patches from [Five patches](patches.md), each ten seconds.

<div class="clip" markdown>
<div class="clip__head" markdown>
**1 · Acid rite** A sixteen-step bassline that bites; HUNGER is the envelope, and RUIN comes in halfway. [How to patch it](patches.md#1-acid-rite).
</div>
![Spectrogram of 1 · Acid rite: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/p1_acid_rite.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/p1_acid_rite.mp3"></audio><a class="clip__wav" href="../assets/audio/p1_acid_rite.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**2 · Pitch-drop kicks** No input: a clock pings the filter, and RUIN makes each hit fall in pitch. [How to patch it](patches.md#2-pitch-drop-kicks).
</div>
![Spectrogram of 2 · Pitch-drop kicks: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/p2_pitch_drop.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/p2_pitch_drop.mp3"></audio><a class="clip__wav" href="../assets/audio/p2_pitch_drop.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**3 · The scream** No input: the filter is the oscillator, sequenced through its cutoff CV while RUIN rises. [How to patch it](patches.md#3-the-scream).
</div>
![Spectrogram of 3 · The scream: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/p3_scream.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/p3_scream.mp3"></audio><a class="clip__wav" href="../assets/audio/p3_scream.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**4 · Undertow** A slow riff dragged down an octave and two. [How to patch it](patches.md#4-undertow).
</div>
![Spectrogram of 4 · Undertow: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/p4_undertow.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/p4_undertow.mp3"></audio><a class="clip__wav" href="../assets/audio/p4_undertow.wav" download>WAV</a></div>
</div>

<div class="clip" markdown>
<div class="clip__head" markdown>
**5 · Devourer** A chord stab whose attack is swallowed while the tail blooms: HUNGER turned back. [How to patch it](patches.md#5-devourer).
</div>
![Spectrogram of 5 · Devourer: time runs left to right, frequency from 30 Hz to 16 kHz on a log scale, bottom to top.](assets/audio/p5_devourer.jpg){ .clip__spec loading=lazy }
<div class="clip__play"><audio controls preload="none" src="../assets/audio/p5_devourer.mp3"></audio><a class="clip__wav" href="../assets/audio/p5_devourer.wav" download>WAV</a></div>
</div>
