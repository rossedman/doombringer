// Doombringer — a four-pole ladder filter that summons what isn't there.
//
//                               ┌──── RUIN: the output's level raises the cutoff ◄────┐
//                               ▼                                                      │
//   in ─► + ─► 4× up ─► drive ─► (−) ─► clip ─► pole ─► pole ─► pole ─► pole ─┬─► taps ─► morph ─► 4× down ─► OUT
//         ▲                       ▲                                           │  LP4 LP2 BP HP2 HP4
//         │                       └──── resonance ◄── RUIN: the feedback folds ┘                    └─► LP4
//         │
//   DOOM: the input's pitch divided by 2 and 4 (flip-flops, enveloped), mixed back in as undertones
//   HUNGER: the input's envelope moves the cutoff      PING: strike the filter so it rings
//
// The ladder is a zero-delay-feedback (TPT) design: the feedback loop is solved
// exactly, then clipped, so resonance tracks the cutoff and self-oscillates in
// tune at k = 4. Everything inside runs at Oversample x the host rate.
// Portable: no heap, no exceptions, no platform headers. Volts in, volts out.
#pragma once

#include <algorithm>
#include <initializer_list>

#include "Decimator.hpp"
#include "Math.hpp"

namespace doom {

struct Params {
    float cutoffV   = 0.f;  // 1V/oct, 0 V = C4
    float resonance = 0.f;  // 0..1.1; 1 is the edge of self-oscillation (k = 4)
    float drive     = 0.f;  // 0..1 into the saturating input stage
    float fold      = 0.f;  // 0..1 the resonance path folds back on itself (RUIN)
    float selfFm    = 0.f;  // 0..1 the output's level raises the cutoff (RUIN)
    float overdrive = 0.f;  // 0..1 the loop's gain past the resonance knob, up to 2x (RUIN)
    float sub1      = 0.f;  // 0..1 level of the undertone an octave down (DOOM)
    float sub2      = 0.f;  // 0..1 level of the undertone two octaves down (DOOM)
    float subFm     = 0.f;  // 0..1 the undertone lurches the cutoff (DOOM)
    float morph     = 0.f;  // 0..1 across LP4, LP2, BP, HP2, HP4
    float fmDepth   = 0.f;  // -1..1 external FM input -> cutoff
    float hunger    = 0.f;  // -1..1 input envelope -> cutoff (negative closes it)
    bool  bassComp  = true; // true: the passband holds its level as resonance rises
};

// The morph's five stops, as weights on the ladder's taps (input, poles 1-4).
// Each is a product of one-pole low-passes L and high-passes 1 - L.
struct TapMix {
    float u, y1, y2, y3, y4;
};
constexpr TapMix kModes[] = {
    {0.f, 0.f, 0.f, 0.f, 1.f},   // LP4: L^4
    {0.f, 0.f, 1.f, 0.f, 0.f},   // LP2: L^2
    {0.f, 0.f, 4.f, -8.f, 4.f},  // BP:  4 L^2 (1-L)^2, unity at the peak
    {1.f, -2.f, 1.f, 0.f, 0.f},  // HP2: (1-L)^2
    {1.f, -4.f, 6.f, -4.f, 1.f}, // HP4: (1-L)^4
};
constexpr int kNumModes = sizeof(kModes) / sizeof(kModes[0]);

// The ladder core: four TPT one-pole stages in a loop. Signals are in units
// of 5 V. One call is one sample at the oversampled rate.
struct Ladder {
    float s[4] = {};          // integrator states
    float u = 0.f, y[4] = {}; // last input-stage value and pole outputs
    float dc = 0.f;           // the feedback's DC, which RUIN leaves unfolded

    // g = prewarp(fc, fs). k = feedback, 4 = self-oscillation.
    // sat = the input stage's clip level. fold = 0..1 the feedback's fold.
    float process(float x, float g, float k, float sat, float fold) {
        const float G = g / (1.f + g), b = 1.f - G;
        // Each stage is y = G * in + b * s, so the last one is G^4 * u + S.
        const float S  = b * (s[3] + G * (s[2] + G * (s[1] + G * s[0])));
        const float G4 = G * G * G * G;
        // Solve u = x - k * y4 exactly (zero delay), then estimate y4 from it.
        const float uLin = (x - k * S) / (1.f + k * G4);
        float       u0   = uLin;
        const float y4   = G4 * uLin + S;
        if (fold > 0.f) {
            // RUIN: the resonance path is a sine folder, sin(w y) / w. Its slope
            // at zero is 1, so small signals ring exactly as before; loud ones
            // fold back and the loop's gain turns over and over. Only the AC
            // part folds: a folded DC path has stable states off zero, and the
            // filter would latch onto one.
            const float w  = fold * kFoldW;
            const float ac = y4 - dc;
            u0             = x - k * (dc + sinTurns(w * ac * (1.f / kTwoPi)) / w);
        }
        dc += kDcCoeff * (y4 - dc);
        // The input stage clips, which bounds self-oscillation. It is linear to
        // half the clip level, so once the filter screams it does so at nearly
        // the same level however far past the edge the resonance is.
        u        = softLimit(u0, kKnee * sat, sat);
        float in = u;
        for (int i = 0; i < 4; ++i) {
            const float v = G * (in - s[i]);
            y[i]          = v + s[i];
            s[i]          = y[i] + v;
            in            = y[i];
        }
        return y[3];
    }
    float mix(const TapMix& m) const { return m.u * u + m.y1 * y[0] + m.y2 * y[1] + m.y3 * y[2] + m.y4 * y[3]; }
    void  reset() { *this = Ladder{}; }

    static constexpr float kDcCoeff = 2e-4f; // about 6 Hz at 192 kHz
    static constexpr float kKnee    = 0.5f;  // the input stage is linear to half its clip level
    static constexpr float kFoldW   = 8.f;   // radians of fold per unit at RUIN's top
};

template <int Oversample = 4, int FirTaps = 129>
class Engine {
  public:
    static constexpr float kInVolts      = 5.f; // +-5 V is one unit inside
    static constexpr float kMinHz        = 8.f;
    static constexpr float kMaxHz        = 22000.f;
    static constexpr float kMaxK         = 4.f;             // resonance 1 -> k = 4, self-oscillation
    static constexpr float kFmOctaves    = 4.f;             // octaves of cutoff per 5 V at full FM depth
    static constexpr float kHungerOct    = 5.f;             // octaves of cutoff at full HUNGER and a 5 V envelope
    static constexpr float kSelfFmOct    = 4.f;             // octaves per unit of output at full RUIN
    static constexpr float kSubFmOct     = 1.5f;            // octaves the undertone swings the cutoff
    static constexpr float kSat          = 2.5f;            // input-stage clip level below resonance
    static constexpr float kSelfOscLevel = 0.9f;            // self-oscillation peak, in units of 5 V
    static constexpr float kRingPerSat   = 0.87f;           // the loop's ring amplitude per unit of clip, at resonance 1.04 (measured)
    static constexpr float kMaxDrive     = 12.f;            // the drive stage's gain at full drive
    static constexpr float kStrike       = 0.7f;            // PING's half-sine, in units of 5 V
    static constexpr float kStageMax     = 1.3f;            // the output stage never passes 6.5 V
    static constexpr float kCeilKnee     = 6.5f / kInVolts; // output soft ceiling: identity below 6.5 V
    static constexpr float kCeilMax      = 7.2f / kInVolts; // and never above 7.2 V

    void init(float sampleRate) {
        fs_    = sampleRate;
        fsInt_ = sampleRate * Oversample;
        up_.init();
        upFm_.init();
        downOut_.init();
        downLp_.init();
        ladder_.reset();
        last_ = 0.f;
        smCutoff_.setTime(0.001f, fs_);
        for (Smoother* s : {&smRes_, &smDrive_, &smFold_, &smSelfFm_, &smOver_, &smSub1_, &smSub2_, &smSubFm_, &smMorph_, &smFm_, &smHunger_, &smComp_})
            s->setTime(0.01f, fs_);
        // Envelope follower: 1 ms attack, 80 ms release.
        envAtk_ = 1.f - std::exp(-1.f / (0.001f * fs_));
        envRel_ = 1.f - std::exp(-1.f / (0.08f * fs_));
        // The pitch detector listens below 600 Hz; the undertone is rounded above 1.2 kHz.
        detCoeff_ = 1.f - std::exp(-kTwoPi * 600.f / fs_);
        subCoeff_ = 1.f - std::exp(-kTwoPi * 1200.f / fs_);
        resetDivider();
        env_ = 0.f;
        setParams(Params{});
        snapSmoothers();
    }

    void setParams(const Params& p) { p_ = p; }

    void snapSmoothers() {
        smCutoff_.reset(p_.cutoffV);
        smRes_.reset(p_.resonance);
        smDrive_.reset(p_.drive);
        smFold_.reset(p_.fold);
        smSelfFm_.reset(p_.selfFm);
        smOver_.reset(p_.overdrive);
        smSub1_.reset(p_.sub1);
        smSub2_.reset(p_.sub2);
        smSubFm_.reset(p_.subFm);
        smMorph_.reset(p_.morph);
        smFm_.reset(p_.fmDepth);
        smHunger_.reset(p_.hunger);
        smComp_.reset(p_.bassComp ? 1.f : 0.f);
    }

    // Strike the filter: a half-sine at the cutoff, so it rings at any pitch.
    void ping() { strike_ = 0.f; }

    // Restart the undertones' flip-flops (both high), so they line up with a beat.
    void resetDivider() {
        det1_ = det2_ = 0.f;
        armed_        = true;
        div1_ = div2_ = 1.f;
        sq1_ = sq2_ = 1.f;
        subA_ = subB_ = 0.f;
    }

    // Render one sample. inV is the audio input, fmV the cutoff FM input (volts).
    // outV is the morph output; lpV is always the four-pole low-pass.
    void process(float inV, float fmV, float& outV, float& lpV) {
        const float cutV   = smCutoff_.process(p_.cutoffV);
        const float res    = clampf(smRes_.process(p_.resonance), 0.f, 1.1f);
        const float drive  = clampf(smDrive_.process(p_.drive), 0.f, 1.f);
        const float fold   = clampf(smFold_.process(p_.fold), 0.f, 1.f);
        const float selfFm = clampf(smSelfFm_.process(p_.selfFm), 0.f, 1.f);
        const float overK  = clampf(smOver_.process(p_.overdrive), 0.f, 1.f);
        const float sub1   = clampf(smSub1_.process(p_.sub1), 0.f, 1.f);
        const float sub2   = clampf(smSub2_.process(p_.sub2), 0.f, 1.f);
        const float subFm  = clampf(smSubFm_.process(p_.subFm), 0.f, 1.f);
        const float morph  = clampf(smMorph_.process(p_.morph), 0.f, 1.f);
        const float fmD    = clampf(smFm_.process(p_.fmDepth), -1.f, 1.f);
        const float hunger = clampf(smHunger_.process(p_.hunger), -1.f, 1.f);
        const float comp   = smComp_.process(p_.bassComp ? 1.f : 0.f);

        const float x = clampf(inV, -12.f, 12.f) * (1.f / kInVolts);

        // HUNGER and DOOM both follow the input's envelope (peak, in units of 5 V).
        const float ax = std::fabs(x);
        env_ += (ax > env_ ? envAtk_ : envRel_) * (ax - env_);

        // DOOM: a flip-flop on the input's rising zero crossings is a square an
        // octave down; a second one on the first is two octaves down. The
        // detector hears the fundamental through two 600 Hz poles, with
        // hysteresis that scales with the envelope so quiet tails can't chatter.
        // An edge lands between samples, so that sample takes the average of
        // the two levels across it (a box-filtered step) instead of a hard jump.
        float edge1 = 1.f, edge2 = 1.f; // fraction of this sample spent at the new level
        {
            const float d0 = det2_;
            det1_ += detCoeff_ * (x - det1_);
            det2_ += detCoeff_ * (det1_ - det2_);
            const float thr = 0.12f * env_ + 1e-4f;
            if (det2_ < -thr) armed_ = true;
            if (armed_ && det2_ >= thr) {
                armed_           = false;
                const float frac = clampf((thr - d0) / (det2_ - d0 + 1e-12f), 0.f, 1.f);
                edge1            = 1.f - frac;
                div1_            = -div1_;
                if (div1_ > 0.f) {
                    div2_ = -div2_;
                    edge2 = edge1;
                }
            }
        }
        sq1_               = lerpf(-div1_, div1_, edge1);
        sq2_               = lerpf(-div2_, div2_, edge2);
        const float subRaw = (sub1 * sq1_ + sub2 * sq2_) * env_;
        subA_ += subCoeff_ * (subRaw - subA_);
        subB_ += subCoeff_ * (subA_ - subB_);
        // The undertone's own shape (no envelope), for the cutoff lurch.
        const float subShape = sub1 + sub2 > 0.f ? (sub1 * sq1_ + sub2 * sq2_) / (sub1 + sub2) : 0.f;
        lurch_ += subCoeff_ * (subShape - lurch_);

        // PING: one half-sine at the cutoff. It rings at about 4.5 V, whatever the cutoff.
        float       strikeV = 0.f;
        const float fc0     = clampf(voltsToHz(cutV), kMinHz, kMaxHz);
        if (strike_ < 1.f) {
            strikeV = kStrike * sinTurns(0.5f * strike_);
            strike_ = std::fmin(1.f, strike_ + 2.f * fc0 / fs_);
        }

        float xs[Oversample], fms[Oversample] = {};
        up_.process(x + subB_ + strikeV, xs);
        // The FM input costs a full upsampler, so it only runs while FM is turned up.
        const float fmIn = clampf(fmV, -12.f, 12.f) * (1.f / kInVolts);
        if (fmD != 0.f) upFm_.process(fmIn, fms);
        else upFm_.push(fmIn);

        // The cutoff in octaves around C4, before the audio-rate terms.
        const float baseOct = cutV + hunger * kHungerOct * env_ + subFm * kSubFmOct * lurch_ * std::fmin(1.f, 4.f * env_);
        const float k       = res * kMaxK; // the loop runs at k * (1 + overdrive); bass compensation follows the knob
        // Bass compensation: the loop costs the passband 1 / (1 + k). Win it back
        // on the low-pass taps; the high-pass ones never paid it.
        const float lpGain = 1.f + comp * k;

        // Morph between neighbouring stops.
        const float   mPos           = morph * (kNumModes - 1);
        const int     mI             = std::min(static_cast<int>(mPos), kNumModes - 2);
        const float   mF             = mPos - static_cast<float>(mI);
        const float   lpW[kNumModes] = {lpGain, lpGain, std::sqrt(lpGain), 1.f, 1.f};
        const TapMix& a              = kModes[mI];
        const TapMix& c              = kModes[mI + 1];
        const float   wa = (1.f - mF) * lpW[mI], wc = mF * lpW[mI + 1];
        const TapMix  m{wa * a.u + wc * c.u, wa * a.y1 + wc * c.y1, wa * a.y2 + wc * c.y2, wa * a.y3 + wc * c.y3, wa * a.y4 + wc * c.y4};

        // The input stage's clip level. Below resonance it sits high, so a 5 V
        // signal passes nearly clean. Toward self-oscillation it moves to where
        // the ringing comes out at kSelfOscLevel in every mode: the loop's
        // amplitude is set by the clip, and each mode hears it through its own
        // gain at the cutoff, |sum of tap weights * L^n| with L = 1 / (1 + j).
        const float re    = m.u + 0.5f * m.y1 - 0.25f * m.y3 - 0.25f * m.y4;
        const float im    = -0.5f * m.y1 - 0.5f * m.y2 - 0.25f * m.y3;
        const float atFc  = std::fmax(std::sqrt(re * re + im * im), 0.05f);
        const float ringT = clampf((res - 0.6f) * 2.5f, 0.f, 1.f);
        const float sat   = lerpf(kSat, kSelfOscLevel / (kRingPerSat * atFc), ringT * ringT);
        // Drive is its own saturator in front of the ladder, tanh(D x) / tanh(D):
        // a 5 V peak stays 5 V and everything quieter is pushed up toward it.
        // (D starts near zero, where the curve is a straight line.) It squares
        // the input off without touching the resonance's own level.
        const float dGain = 0.05f + (kMaxDrive - 0.05f) * drive * drive;
        const float dNorm = 1.f / fastTanhPos(dGain);

        const float maxHz = std::fmin(kMaxHz, 0.3f * fsInt_);
        float       yO = 0.f, yL = 0.f;
        for (int i = 0; i < Oversample; ++i) {
            const float oct = baseOct + fmD * kFmOctaves * fms[i] + selfFm * kSelfFmOct * std::fabs(last_);
            const float fc  = clampf(kC4Hz * fastExp2(oct), kMinHz, maxHz);
            const float y4  = ladder_.process(fastTanh(dGain * xs[i]) * dNorm + kNoise * noise(), prewarp(fc, fsInt_), k * (1.f + overK), sat, fold);
            // The output stage rounds off ringing past 5 V here, oversampled,
            // rather than at the final ceiling where it would alias.
            last_ = softLimit(ladder_.mix(m), 1.f, kStageMax);
            float o;
            if (downOut_.push(last_, o)) yO = o;
            if (downLp_.push(softLimit(y4 * lpGain, 1.f, kStageMax), o)) yL = o;
        }
        outV = softLimit(yO, kCeilKnee, kCeilMax) * kInVolts;
        lpV  = softLimit(yL, kCeilKnee, kCeilMax) * kInVolts;
    }

    // The input envelope HUNGER and DOOM follow, in volts (0..5 for a +-5 V input).
    float envelopeV() const { return env_ * kInVolts; }
    float sampleRate() const { return fs_; }

  private:
    // A whisper of noise (-94 dB), like a circuit's, so self-oscillation starts
    // on its own and the states never go denormal. Growing from it takes a few
    // hundred cycles, so a low, silent cutoff takes seconds to wake: PING it.
    static constexpr float kNoise = 2e-5f;
    uint32_t               seed_  = 22222u;
    float                  noise() {
        seed_ = seed_ * 1664525u + 1013904223u;
        return static_cast<float>(static_cast<int32_t>(seed_)) * (1.f / 2147483648.f);
    }

    Params p_;
    float  fs_ = 48000.f, fsInt_ = 48000.f * Oversample;

    Ladder                            ladder_;
    Interpolator<Oversample, FirTaps> up_, upFm_;
    Decimator<Oversample, FirTaps>    downOut_, downLp_;

    Smoother smCutoff_, smRes_, smDrive_, smFold_, smSelfFm_, smOver_, smSub1_, smSub2_, smSubFm_, smMorph_, smFm_, smHunger_, smComp_;

    float env_ = 0.f, envAtk_ = 1.f, envRel_ = 1.f;
    float det1_ = 0.f, det2_ = 0.f, detCoeff_ = 1.f;
    bool  armed_ = true;
    float div1_ = 1.f, div2_ = 1.f, sq1_ = 1.f, sq2_ = 1.f;
    float subA_ = 0.f, subB_ = 0.f, subCoeff_ = 1.f, lurch_ = 0.f;
    float strike_ = 1.f; // 0..1 through the ping's half-sine; 1 = idle
    float last_   = 0.f; // the last oversampled output, for RUIN's self-FM
};

} // namespace doom
