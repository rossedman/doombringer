"""Turn the renders in test/out/listen into the docs' listening page assets.

    python docs/assets/make_audio.py test/out/listen docs/assets/audio   (make docs-audio)

For every WAV: an MP3 for the page's player (mono, 160 kbps), a spectrogram JPEG
in the site's colours, and the WAV itself for download. Nothing here is
committed; the docs build regenerates it, so the page always plays the current DSP.
"""

import shutil
import sys
from pathlib import Path

import lameenc
import matplotlib
import numpy as np
from scipy.io import wavfile
from scipy.signal import spectrogram

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

INK = "#0c0708"


def mp3(samples: np.ndarray, rate: int, path: Path) -> None:
    enc = lameenc.Encoder()
    enc.set_bit_rate(160)
    enc.set_in_sample_rate(rate)
    enc.set_channels(1)
    enc.set_quality(2)
    path.write_bytes(enc.encode(samples.astype(np.int16).tobytes()) + enc.flush())


def spectrum(samples: np.ndarray, rate: int, path: Path) -> None:
    x = samples.astype(np.float64) / 32768.0
    f, t, s = spectrogram(x, rate, nperseg=4096, noverlap=3584, window="blackmanharris")
    db = 10 * np.log10(s + 1e-12)
    fig = plt.figure(figsize=(12, 2.2), dpi=100)
    ax = fig.add_axes([0, 0, 1, 1])
    # Log frequency, 30 Hz to 16 kHz: the low end, where this module lives, gets room.
    ax.pcolormesh(t, f, db, cmap="magma", vmin=db.max() - 85, vmax=db.max(), shading="auto", rasterized=True)
    ax.set_yscale("log")
    ax.set_ylim(30, 16000)
    ax.set_axis_off()
    fig.savefig(path, facecolor=INK, pil_kwargs={"quality": 82})
    plt.close(fig)


def main(src: str, dst: str) -> None:
    out = Path(dst)
    out.mkdir(parents=True, exist_ok=True)
    wavs = sorted(Path(src).glob("*.wav"))
    if not wavs:
        sys.exit(f"no WAVs in {src}; run make -C test renders first")
    for wav in wavs:
        rate, data = wavfile.read(wav)
        mono = data[:, 0] if data.ndim == 2 else data
        mp3(mono, rate, out / f"{wav.stem}.mp3")
        spectrum(mono, rate, out / f"{wav.stem}.jpg")
        shutil.copyfile(wav, out / wav.name)
        print(f"  {wav.stem}: mp3, jpg, wav")


if __name__ == "__main__":
    main(*sys.argv[1:3])
