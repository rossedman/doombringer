// Host-side tests + WAV renders for the portable DSP core.
// Build & run:  make -C test && ./test/build/host_tests test/out
#include <chrono>
#include <complex>
#include <memory>
#include <cstdio>
#include <functional>
#include <random>
#include <string>
#include <vector>

#include "../dsp/Panel.hpp"
#include "patches.hpp"

using namespace doom;

static constexpr float  kFs        = 48000.f;
static constexpr double kTau       = 6.28318530717959;
static int              g_failures = 0;

#define CHECK(cond, ...)                         \
    do {                                         \
        if (!(cond)) {                           \
            std::printf("  FAIL: " __VA_ARGS__); \
            std::printf("\n");                   \
            ++g_failures;                        \
        }                                        \
    } while (0)

// ---------------------------------------------------------------- helpers
// Mono 16-bit WAV, 8 V = full scale (the output never passes 7.2 V).
static void writeWav(const std::string& path, const std::vector<float>& x) {
    FILE* f = std::fopen(path.c_str(), "wb");
    if (!f) {
        std::perror(path.c_str());
        return;
    }
    const uint32_t n = static_cast<uint32_t>(x.size()), bytes = n * 2, rate = 48000;
    auto           u32 = [&](uint32_t v) { std::fwrite(&v, 4, 1, f); };
    auto           u16 = [&](uint16_t v) { std::fwrite(&v, 2, 1, f); };
    std::fwrite("RIFF", 1, 4, f);
    u32(36 + bytes);
    std::fwrite("WAVEfmt ", 1, 8, f);
    u32(16);
    u16(1);
    u16(1);
    u32(rate);
    u32(rate * 2);
    u16(2);
    u16(16);
    std::fwrite("data", 1, 4, f);
    u32(bytes);
    for (float v : x) {
        const int16_t q = static_cast<int16_t>(clampf(v / 8.f, -1.f, 1.f) * 32767.f);
        std::fwrite(&q, 2, 1, f);
    }
    std::fclose(f);
    std::printf("  wrote %s\n", path.c_str());
}

// Frequency by interpolated rising zero crossings over [from, to).
static double measureHz(const std::vector<float>& x, size_t from, size_t to) {
    double first = -1, last = -1;
    int    count = 0;
    for (size_t i = from + 1; i < to && i < x.size(); ++i) {
        if (x[i - 1] < 0.f && x[i] >= 0.f) {
            const double t = static_cast<double>(i - 1) + x[i - 1] / (x[i - 1] - x[i]);
            if (first < 0) first = t;
            else ++count;
            last = t;
        }
    }
    return count > 0 ? count * kFs / (last - first) : 0.0;
}

// Power at one frequency relative to the whole signal (Hann window, Goertzel), dB.
// hz must land on an exact bin: a whole number of cycles in the buffer.
static double binDb(const std::vector<float>& x, double hz) {
    const size_t        n = x.size();
    std::vector<double> w(n);
    double              total = 0.0;
    for (size_t i = 0; i < n; ++i) {
        w[i] = x[i] * (0.5 - 0.5 * std::cos(kTau * static_cast<double>(i) / n));
        total += w[i] * w[i];
    }
    total *= static_cast<double>(n);
    auto power = [&](double k) {
        const double c  = 2.0 * std::cos(kTau * k / n);
        double       s1 = 0, s2 = 0;
        for (double v : w) {
            const double s0 = v + c * s1 - s2;
            s2              = s1;
            s1              = s0;
        }
        return s1 * s1 + s2 * s2 - c * s1 * s2;
    };
    const double k = hz * static_cast<double>(n) / kFs;
    double       e = 0.0;
    for (int d = -1; d <= 1; ++d) e += 2.0 * power(k + d); // Hann main lobe, +/- freqs
    return 10.0 * std::log10(e / (total + 1e-30) + 1e-30);
}

// Share of the energy that is NOT on harmonics of f0 (exact bins, as above).
static double inharmonic(const std::vector<float>& x, double f0) {
    double harm = 0.0;
    for (int h = 1; h * f0 < kFs / 2 - 2; ++h) harm += std::pow(10.0, binDb(x, h * f0) / 10.0);
    return std::max(0.0, 1.0 - harm);
}

static double rmsOf(const std::vector<float>& x, size_t from = 0) {
    double e = 0.0;
    for (size_t i = from; i < x.size(); ++i) e += static_cast<double>(x[i]) * x[i];
    return std::sqrt(e / static_cast<double>(x.size() - from));
}

static float peakOf(const std::vector<float>& x, size_t from = 0) {
    float p = 0.f;
    for (size_t i = from; i < x.size(); ++i) p = std::max(p, std::fabs(x[i]));
    return p;
}

// Run the engine for `seconds` after a 0.25 s settle; input(i) gives the input volts.
static Engine<4>          g_eng;
static std::vector<float> run(const Params& p, float seconds, const std::function<float(int)>& input, bool ping = false) {
    g_eng.init(kFs);
    g_eng.setParams(p);
    g_eng.snapSmoothers();
    float o, l;
    for (int i = 0; i < 12000; ++i) g_eng.process(input(i - 12000), 0.f, o, l);
    if (ping) g_eng.ping();
    std::vector<float> out(static_cast<size_t>(seconds * kFs));
    for (size_t i = 0; i < out.size(); ++i) g_eng.process(input(static_cast<int>(i)), 0.f, out[i], l);
    return out;
}

static float silence(int) { return 0.f; }

static std::function<float(int)> sine(double hz, double volts) {
    return [=](int i) { return static_cast<float>(volts * std::sin(kTau * hz * i / kFs)); };
}

static std::function<float(int)> saw(float hz) {
    auto s = std::make_shared<Saw>();
    return [=](int) { return s->tick(hz, kFs); };
}

// Gain of a sine through the filter (morph output), dB.
static double gainDb(const Params& p, double hz) {
    auto y = run(p, 0.5f, sine(hz, 1.0));
    return 20.0 * std::log10(rmsOf(y) / (1.0 / std::sqrt(2.0)));
}

// ---------------------------------------------------------------- tests
static void testResponse() {
    std::printf("response: five modes, a four-pole slope, bass compensation\n");
    Params p;
    p.cutoffV       = 1.f; // 523 Hz
    const double fc = kC4Hz * 2.0;
    // Each mode against its analog shape at fc/4, fc and 4 fc: products of the
    // one-pole L = 1 / (1 + j f / fc) and 1 - L (TPT matches them to within a
    // few hundredths of a dB this far below Nyquist at 192 kHz).
    const char* names[kNumModes] = {"LP4", "LP2", "BP", "HP2", "HP4"};
    double      worst            = 0.0;
    for (int m = 0; m < kNumModes; ++m) {
        p.morph = static_cast<float>(m) / (kNumModes - 1);
        std::printf("  %-3s", names[m]);
        for (double ratio : {0.25, 1.0, 4.0}) {
            const std::complex<double> L = 1.0 / std::complex<double>(1.0, ratio);
            const TapMix&              t = kModes[m];
            const double want = 20.0 * std::log10(std::abs(std::complex<double>(t.u) + double(t.y1) * L + double(t.y2) * L * L + double(t.y3) * L * L * L +
                                                           double(t.y4) * L * L * L * L) +
                                                  1e-30);
            const double got  = gainDb(p, fc * ratio);
            std::printf("  %4s %6.1f dB", ratio < 1 ? "fc/4" : ratio > 1 ? "4fc" : "fc", got);
            worst = std::max(worst, std::fabs(got - want));
        }
        std::printf("\n");
    }
    std::printf("  worst error against the analog shapes: %.2f dB\n", worst);
    CHECK(worst < 0.1, "a mode's response is off by %.2f dB", worst);

    // Bass compensation: at resonance 0.95 the loop costs the passband 1 / (1 + 3.8).
    Params r          = p;
    r.morph           = 0.f;
    r.resonance       = 0.95f;
    const double comp = gainDb(r, fc / 16);
    r.bassComp        = false;
    const double thin = gainDb(r, fc / 16);
    std::printf("  passband at resonance 0.95: compensated %.1f dB, thin %.1f dB\n", comp, thin);
    CHECK(std::fabs(comp) < 0.5, "bass compensation should hold the passband (%.1f dB)", comp);
    CHECK(std::fabs(thin + 13.6) < 0.5, "thin should lose 13.6 dB (%.1f dB)", thin);
}

static void testSelfOscillation() {
    std::printf("self-oscillation: in tune (1V/oct), the same level in every mode\n");
    double worst = 0.0;
    for (float v = -3.f; v <= 5.01f; v += 1.f) {
        Params p;
        p.cutoffV         = v;
        p.resonance       = 1.04f;
        auto         y    = run(p, 2.f, silence, true);
        const double want = kC4Hz * std::pow(2.0, v), got = measureHz(y, 48000, y.size());
        const double c = 1200.0 * std::log2(got / want);
        std::printf("  %+4.1f V  want %8.2f Hz  got %8.2f Hz  %+6.2f cents  peak %.2f V\n", v, want, got, c, peakOf(y, 48000));
        worst = std::max(worst, std::fabs(c));
    }
    std::printf("  worst error: %.2f cents\n", worst);
    CHECK(worst < 1.0, "self-oscillation out of tune by %.2f cents", worst);

    float lo = 99.f, hi = 0.f;
    std::printf("  peak by mode and resonance:");
    for (float r : {1.01f, 1.04f, 1.1f})
        for (int m = 0; m < kNumModes; ++m) {
            Params p;
            p.resonance   = r;
            p.morph       = static_cast<float>(m) / (kNumModes - 1);
            const float k = peakOf(run(p, 2.f, silence, true), 48000);
            std::printf(" %.1f", k);
            lo = std::min(lo, k);
            hi = std::max(hi, k);
        }
    std::printf(" V\n");
    CHECK(lo > 3.5f && hi < 6.f, "self-oscillation should sit at 3.5..6 V (%.2f..%.2f V)", lo, hi);

    // Below the edge it rings and dies away.
    Params p;
    p.resonance       = 0.95f;
    const float decay = peakOf(run(p, 2.f, silence, true), 72000);
    std::printf("  resonance 0.95, 1.5 s after a ping: %.4f V\n", decay);
    CHECK(decay < 0.01f, "below the edge the ring must die away (%.4f V)", decay);
}

static void testPing() {
    std::printf("ping: rings at about the same level at any cutoff\n");
    float lo = 99.f, hi = 0.f;
    for (float v : {-3.f, 0.f, 3.f, 5.f})
        for (float r : {0.8f, 0.95f, 0.99f}) {
            Params p;
            p.cutoffV     = v;
            p.resonance   = r;
            const float k = peakOf(run(p, 0.5f, silence, true));
            lo            = std::min(lo, k);
            hi            = std::max(hi, k);
        }
    std::printf("  peak %.2f .. %.2f V across 16 Hz..8 kHz and resonance 0.8..0.99\n", lo, hi);
    CHECK(lo > 2.5f && hi < 6.f, "ping level should be 2.5..6 V (%.2f..%.2f V)", lo, hi);
}

static void testDrive() {
    std::printf("drive: a 5 V peak stays 5 V, quiet signals come up, harmonics grow\n");
    double prevH3 = -999.0;
    for (float d : {0.f, 0.5f, 1.f}) {
        Params p;
        p.cutoffV         = 7.f; // wide open
        p.drive           = d;
        auto         loud = run(p, 1.f, sine(220.0, 5.0));
        auto         soft = run(p, 1.f, sine(220.0, 0.5));
        const double h3   = binDb(loud, 660.0) - binDb(loud, 220.0);
        const float  pk   = peakOf(loud);
        std::printf("  drive %.1f  5 V in: peak %.2f V, H3 %6.1f dB   0.5 V in: rms %.2f V\n", d, pk, h3, rmsOf(soft));
        CHECK(std::fabs(pk - 5.f) < 0.25f, "a 5 V peak should stay 5 V (%.2f V)", pk);
        if (d == 0.f) CHECK(h3 < -45.0, "no drive should be clean (H3 %.1f dB)", h3);
        CHECK(h3 > prevH3, "drive should add harmonics");
        prevH3 = h3;
    }
}

static void testDoom() {
    std::printf("DOOM: undertones an octave and two below the input\n");
    // A 220 Hz saw: 110 and 55 Hz land on exact bins of a 2 s buffer.
    for (float lvl : {0.f, 1.f}) {
        Params p;
        p.cutoffV       = 3.f;
        p.sub1          = lvl;
        p.sub2          = lvl;
        auto         y  = run(p, 2.f, saw(220.f));
        const double o1 = binDb(y, 110.0) - binDb(y, 220.0);
        const double o2 = binDb(y, 55.0) - binDb(y, 220.0);
        std::printf("  undertones %.0f: -1 oct %6.1f dB, -2 oct %6.1f dB (re the fundamental)\n", lvl, o1, o2);
        if (lvl == 0.f) CHECK(o1 < -60.0 && o2 < -60.0, "no DOOM should mean no undertones");
        else CHECK(o1 > -6.0 && o2 > -12.0, "DOOM should summon both undertones");
    }
    // When the input stops, so do they: no stuck square, no DC.
    Params p;
    p.cutoffV         = 3.f;
    p.sub1            = 1.f;
    p.sub2            = 1.f;
    auto        y     = run(p, 1.5f, [](int i) { return i < 24000 ? 5.f * std::sin(static_cast<float>(kTau) * 110.f * static_cast<float>(i) / kFs) : 0.f; });
    const float after = peakOf(y, 48000);
    std::printf("  0.5 s after the input stops: %.4f V\n", after);
    CHECK(after < 0.01f, "undertones must die with the input (%.4f V)", after);
}

static void testHunger() {
    std::printf("HUNGER: the input's envelope moves the cutoff\n");
    double c[3];
    int    i = 0;
    for (float h : {-1.f, 0.f, 1.f}) {
        Params p;
        p.cutoffV    = 1.f;
        p.hunger     = h;
        auto      y  = run(p, 1.f, saw(110.f));
        const int hz = 440; // the fourth harmonic, where the cutoff sits
        c[i++]       = binDb(y, hz) - binDb(y, 110.0);
        std::printf("  hunger %+.0f: 4th harmonic %6.1f dB re the fundamental\n", h, c[i - 1]);
    }
    CHECK(c[2] > c[1] + 6.0, "positive HUNGER should open the filter");
    CHECK(c[0] < c[1] - 6.0, "negative HUNGER should close it");
}

static void testRuin() {
    std::printf("RUIN: a quiet input lets it break free, a hot one keeps it harmonic; kicks whose pitch falls\n");
    // A 1 V saw through resonance 0.8 (WRATH half). The corrupted loop fights the
    // input for the pitch: quiet, it wins and the spectrum breaks; at 5 V the
    // input drags it back into step (injection locking).
    double r[3];
    int    i = 0;
    for (float ruin : {0.f, 0.9f, 0.9f}) {
        PanelState s;
        s.cutoff      = 0.45f;
        s.wrath       = 0.5f;
        s.ruin        = ruin;
        const float v = i == 2 ? 5.f : 1.f;
        r[i++]        = inharmonic(run(panelToParams(s), 2.f, [sw = std::make_shared<Saw>(), v](int) { return sw->tick(55.f, kFs) * v / 5.f; }), 55.0);
    }
    std::printf("  inharmonic share: RUIN 0 %.4f, RUIN 0.9 with 1 V in %.3f, with 5 V in %.4f\n", r[0], r[1], r[2]);
    CHECK(r[0] < 1e-3, "RUIN at zero should leave it harmonic (%.4f)", r[0]);
    CHECK(r[1] > 0.5, "RUIN should break a quiet input's harmonics (%.3f)", r[1]);
    CHECK(r[2] < 0.02, "a hot input should hold it in step (%.4f)", r[2]);

    // A pinged filter with the self-FM: loud is higher, so the pitch falls as it rings out.
    Params k;
    k.cutoffV         = -1.f;
    k.resonance       = 0.97f;
    k.selfFm          = 0.7f;
    auto         y    = run(k, 1.f, silence, true);
    const double head = measureHz(y, 0, 2400), tail = measureHz(y, 19200, 28800);
    std::printf("  pinged with self-FM: %.0f Hz in the first 50 ms, %.0f Hz at 0.4..0.6 s\n", head, tail);
    CHECK(head > tail * 1.3, "the ping's pitch should fall (%.0f -> %.0f Hz)", head, tail);
}

static void testStability() {
    std::printf("stability: random extreme settings, hot input and FM\n");
    static Engine<4>                      e;
    std::mt19937                          rng(666);
    std::uniform_real_distribution<float> u(0.f, 1.f);
    e.init(kFs);
    float peak = 0.f;
    bool  bad  = false;
    Saw   s;
    for (int block = 0; block < 2000; ++block) {
        Params p;
        p.cutoffV   = -8.f + u(rng) * 18.f; // far outside the knob's range
        p.resonance = u(rng) * 1.5f;
        p.drive     = u(rng) * 1.5f;
        p.fold      = u(rng) * 1.5f;
        p.selfFm    = u(rng) * 1.5f;
        p.overdrive = u(rng) * 1.5f;
        p.sub1      = u(rng);
        p.sub2      = u(rng);
        p.subFm     = u(rng) * 1.5f;
        p.morph     = u(rng) * 1.2f - 0.1f;
        p.fmDepth   = u(rng) * 3.f - 1.5f;
        p.hunger    = u(rng) * 3.f - 1.5f;
        p.bassComp  = u(rng) > 0.5f;
        e.setParams(p);
        if (block % 37 == 0) e.ping();
        if (block % 53 == 0) e.resetDivider();
        const float hz = 20.f + u(rng) * 2000.f;
        for (int i = 0; i < 48; ++i) {
            float o, l;
            e.process(s.tick(hz, kFs) * 2.f + (u(rng) - 0.5f) * 4.f, (u(rng) - 0.5f) * 24.f, o, l);
            if (!std::isfinite(o) || !std::isfinite(l)) bad = true;
            peak = std::max(peak, std::max(std::fabs(o), std::fabs(l)));
        }
    }
    std::printf("  peak output %.2f V, finite: %s\n", peak, bad ? "NO" : "yes");
    CHECK(!bad, "non-finite output");
    CHECK(peak < 7.5f, "output peak %.2f V exceeds the +-7.5 V budget", peak);
}

static void testLongRun() {
    std::printf("long-run stability: 60 s of every macro at full, per cutoff\n");
    for (float v : {-3.f, 0.f, 3.f, 6.f})
        for (int src = 0; src < 2; ++src) {
            PanelState s;
            s.cutoff   = (v + 4.f) / 10.f;
            s.wrath    = 1.f;
            s.ruin     = 1.f;
            s.doom     = 1.f;
            s.hunger   = 1.f;
            auto   in  = src ? saw(55.f) : std::function<float(int)>(silence);
            auto   y   = run(panelToParams(s), 60.f, in, true);
            double dc  = 0.0;
            bool   bad = false;
            for (float x : y) {
                dc += x;
                bad = bad || !std::isfinite(x);
            }
            dc /= static_cast<double>(y.size());
            const double tailRms = rmsOf(std::vector<float>(y.end() - 48000, y.end()));
            std::printf("  cutoff %+.0f V, %s: rms (last 1 s) %.2f V, DC %+.3f V, peak %.2f V%s\n", v, src ? "saw  " : "alone", tailRms, dc, peakOf(y),
                        bad ? "  NON-FINITE" : "");
            CHECK(!bad, "non-finite output");
            CHECK(std::fabs(dc) < 0.5, "DC lock-up (%.3f V)", dc);
            CHECK(tailRms > 0.3, "went silent (%.2f V rms)", tailRms);
        }
}

static void testMacros() {
    std::printf("macros: WRATH, RUIN, DOOM\n");
    Params plain, p;
    applyMacros(MacroValues{}, p);
    CHECK(p.resonance == plain.resonance && p.drive == plain.drive && p.fold == plain.fold && p.selfFm == plain.selfFm && p.overdrive == plain.overdrive &&
              p.sub1 == plain.sub1 && p.sub2 == plain.sub2 && p.subFm == plain.subFm,
          "macros at zero must be the plain filter");

    float worst = 0.f;
    for (int mi = 0; mi < 3; ++mi) {
        Params prev;
        for (int i = 0; i <= 1000; ++i) {
            MacroValues m;
            (mi == 0 ? m.wrath : mi == 1 ? m.ruin : m.doom) = static_cast<float>(i) / 1000.f;
            Params q;
            applyMacros(m, q);
            if (i > 0)
                for (float d : {q.resonance - prev.resonance, q.drive - prev.drive, q.fold - prev.fold, q.selfFm - prev.selfFm, q.overdrive - prev.overdrive,
                                q.sub1 - prev.sub1, q.sub2 - prev.sub2, q.subFm - prev.subFm})
                    worst = std::max(worst, std::fabs(d));
            prev = q;
        }
    }
    std::printf("  largest parameter step per 0.001 of knob: %.4f\n", worst);
    CHECK(worst < 0.012f, "a macro curve jumps (%.4f)", worst);

    // WRATH: silent below the edge, screaming on its own above it.
    std::printf("  WRATH, no input, peak:");
    for (int i = 5; i <= 10; ++i) {
        PanelState s;
        s.wrath       = i / 10.f;
        const float k = peakOf(run(panelToParams(s), 3.f, silence), 96000);
        std::printf(" %.2f", k);
        if (i <= 6) CHECK(k < 0.05f, "WRATH %.1f should not self-oscillate (%.2f V)", s.wrath, k);
        if (i >= 7) CHECK(k > 3.5f, "WRATH %.1f should scream on its own (%.2f V)", s.wrath, k);
    }
    std::printf(" V\n");

    // DOOM: the octave-down undertone arrives first, the second one after it.
    std::printf("  DOOM -1 / -2 octaves (dB re the fundamental):");
    double prevSum = -999.0;
    bool   rising  = true;
    for (int i = 0; i <= 10; i += 2) {
        PanelState s;
        s.cutoff         = 0.7f;
        s.doom           = i / 10.f;
        auto         y   = run(panelToParams(s), 2.f, saw(220.f));
        const double o1  = binDb(y, 110.0) - binDb(y, 220.0);
        const double o2  = binDb(y, 55.0) - binDb(y, 220.0);
        const double sum = o1 + o2;
        std::printf(" %.0f/%.0f", o1, o2);
        if (i > 0 && i < 8 && sum < prevSum) rising = false;
        prevSum = sum;
    }
    std::printf("\n");
    CHECK(rising, "DOOM should bring the undertones up steadily");

    // RUIN, with a 1 V saw: harmonic through the lower half, broken in the upper.
    std::printf("  RUIN inharmonic share (1 V saw, WRATH 0.5):");
    double low = 0.0, high = 0.0;
    for (int i = 0; i <= 10; ++i) {
        PanelState s;
        s.cutoff       = 0.45f;
        s.wrath        = 0.5f;
        s.ruin         = i / 10.f;
        const double r = inharmonic(run(panelToParams(s), 2.f, [sw = std::make_shared<Saw>()](int) { return sw->tick(55.f, kFs) * 0.2f; }), 55.0);
        std::printf(" %.2f", r);
        if (i <= 5) low = std::max(low, r);
        if (i >= 8) high = std::max(high, r);
    }
    std::printf("\n");
    CHECK(low < 0.01, "RUIN's lower half should stay harmonic (%.3f)", low);
    CHECK(high > 0.5, "RUIN's upper half should break the harmonics (%.3f)", high);
}

template <int OS>
static double benchNsPerSample() {
    static Engine<OS> e;
    e.init(kFs);
    Params p;
    p.resonance = 0.8f;
    p.fold      = 0.5f;
    p.selfFm    = 0.3f;
    p.sub1      = 0.5f;
    p.morph     = 0.3f;
    e.setParams(p);
    Saw            s;
    const int      n    = 48000 * 4;
    volatile float sink = 0.f;
    auto           t0   = std::chrono::steady_clock::now();
    for (int i = 0; i < n; ++i) {
        float o, l;
        e.process(s.tick(110.f, kFs), 0.f, o, l);
        sink = sink + o + l;
    }
    auto t1 = std::chrono::steady_clock::now();
    return std::chrono::duration<double, std::nano>(t1 - t0).count() / n;
}

// ---------------------------------------------------------------- renders
static void renderPatches(const std::string& dir) {
    std::printf("renders\n");
    for (const Clip& c : clips()) writeWav(dir + "/" + c.file + ".wav", renderClip(c, kFs));
}

int main(int argc, char** argv) {
    const std::string out = argc > 1 ? argv[1] : "out";
    testResponse();
    testSelfOscillation();
    testPing();
    testDrive();
    testDoom();
    testHunger();
    testRuin();
    testStability();
    testLongRun();
    testMacros();
    std::printf("benchmark (host CPU, per output sample)\n");
    std::printf("  OS=1: %6.1f ns   OS=4: %6.1f ns\n", benchNsPerSample<1>(), benchNsPerSample<4>());
    renderPatches(out);
    std::printf(g_failures ? "\n%d FAILURE(S)\n" : "\nall tests passed\n", g_failures);
    return g_failures ? 1 : 0;
}
