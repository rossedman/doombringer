// FIR interpolator and decimator for oversampled processing.
//
// The filter runs at Factor x the host rate. Its input is upsampled first
// (Interpolator: zero-stuff, then low-pass at the input Nyquist), and before
// going back to the host rate we low-pass at the output Nyquist and keep every
// Factor-th sample (Decimator). Taps are designed once at init with a
// Kaiser-windowed sinc; no heap, fixed size.
#pragma once

#include "Math.hpp"

namespace doom {

namespace fir {
inline double besselI0(double x) {
    double sum = 1.0, term = 1.0, q = x * x / 4.0;
    for (int k = 1; k < 64; ++k) {
        term *= q / (static_cast<double>(k) * k);
        sum += term;
        if (term < 1e-12 * sum) break;
    }
    return sum;
}
inline double kaiserBeta(double a) {
    if (a > 50.0) return 0.1102 * (a - 8.7);
    if (a >= 21.0) return 0.5842 * std::pow(a - 21.0, 0.4) + 0.07886 * (a - 21.0);
    return 0.0;
}
// A linear-phase low-pass at the Nyquist of the rate Factor x lower, with unity DC gain.
template <int Taps>
inline void designLowpass(float (&h)[Taps], int factor, double stopbandDb) {
    const double beta   = kaiserBeta(stopbandDb);
    const double cutoff = 0.5 / factor; // normalized to the oversampled rate
    const double i0b    = besselI0(beta);
    const int    mid    = Taps / 2;
    double       hd[Taps];
    double       sum = 0.0;
    for (int n = 0; n < Taps; ++n) {
        int    k    = n - mid;
        double sinc = k == 0 ? 2.0 * cutoff : std::sin(2.0 * 3.14159265358979 * cutoff * k) / (3.14159265358979 * k);
        double r    = static_cast<double>(k) / mid;
        double w    = besselI0(beta * std::sqrt(1.0 - r * r)) / i0b;
        hd[n]       = sinc * w;
        sum += hd[n];
    }
    for (int n = 0; n < Taps; ++n) h[n] = static_cast<float>(hd[n] / sum);
}
} // namespace fir

template <int Factor, int Taps>
class Decimator {
    static_assert(Factor >= 1, "Factor must be >= 1");
    static_assert(Taps % 2 == 1, "use an odd tap count (linear phase, integer delay)");

  public:
    // stopbandDb sets the Kaiser beta. ~90 dB keeps aliases below the noise floor
    // of a 24-bit codec in practice.
    void init(float stopbandDb = 90.f) {
        fir::designLowpass(h_, Factor, stopbandDb);
        reset();
    }

    void reset() {
        for (float& s : buf_) s = 0.f;
        pos_   = 0;
        phase_ = 0;
    }

    // Push one oversampled sample. Returns true when an output is ready in `out`.
    bool push(float x, float& out) {
        buf_[pos_]        = x;
        buf_[pos_ + Taps] = x; // mirrored so the convolution never wraps
        if (++pos_ == Taps) pos_ = 0;
        if (++phase_ < Factor) return false;
        phase_ = 0;
        // Oldest sample is at pos_, newest at pos_ + Taps - 1.
        const float* p   = &buf_[pos_];
        float        acc = 0.f;
        for (int n = 0; n < Taps; ++n) acc += h_[n] * p[n];
        out = acc;
        return true;
    }

    static constexpr int latency() { return Taps / 2; } // in oversampled samples

  private:
    float h_[Taps]       = {};
    float buf_[2 * Taps] = {};
    int   pos_ = 0, phase_ = 0;
};

// Factor 1 = no oversampling (handy for A/B-testing aliasing).
template <int Taps>
class Decimator<1, Taps> {
  public:
    void init(float = 0.f) {}
    void reset() {}
    bool push(float x, float& out) {
        out = x;
        return true;
    }
    static constexpr int latency() { return 0; }
};

// Polyphase FIR interpolator: one sample in, Factor samples out at Factor x the
// rate, with the images above the input Nyquist removed. Same Kaiser design as
// the decimator, so an upsample/decimate round trip is flat to 20 kHz.
template <int Factor, int Taps>
class Interpolator {
    static_assert(Factor >= 1, "Factor must be >= 1");
    static_assert(Taps % 2 == 1, "use an odd tap count (linear phase, integer delay)");
    static constexpr int kPhaseTaps = (Taps + Factor - 1) / Factor; // taps per output phase

  public:
    void init(float stopbandDb = 90.f) {
        float h[Taps];
        fir::designLowpass(h, Factor, stopbandDb);
        // Zero-stuffing divides the level by Factor; the taps win it back.
        for (int p = 0; p < Factor; ++p)
            for (int m = 0; m < kPhaseTaps; ++m) {
                const int n = p + m * Factor;
                h_[p][m]    = n < Taps ? h[n] * static_cast<float>(Factor) : 0.f;
            }
        reset();
    }

    void reset() {
        for (float& s : buf_) s = 0.f;
        pos_ = 0;
    }

    // Push one input sample and write Factor oversampled samples to out.
    void process(float x, float (&out)[Factor]) {
        push(x);
        const float* p = &buf_[pos_];
        for (int ph = 0; ph < Factor; ++ph) {
            float acc = 0.f;
            for (int m = 0; m < kPhaseTaps; ++m) acc += h_[ph][m] * p[m];
            out[ph] = acc;
        }
    }

    // Push one input sample without computing the outputs, to keep the history
    // current while nobody is listening.
    void push(float x) {
        if (--pos_ < 0) pos_ = kPhaseTaps - 1;
        buf_[pos_]              = x;
        buf_[pos_ + kPhaseTaps] = x; // mirrored: newest at pos_, older ones after it
    }

    static constexpr int latency() { return Taps / 2; } // in oversampled samples

  private:
    float h_[Factor][kPhaseTaps] = {};
    float buf_[2 * kPhaseTaps]   = {};
    int   pos_                   = 0;
};

template <int Taps>
class Interpolator<1, Taps> {
  public:
    void                 init(float = 0.f) {}
    void                 reset() {}
    void                 process(float x, float (&out)[1]) { out[0] = x; }
    void                 push(float) {}
    static constexpr int latency() { return 0; }
};

} // namespace doom
