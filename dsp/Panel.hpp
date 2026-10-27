// Doombringer's panel: the cutoff and three character macros on the front,
// four settings on the patch.Init's shift layer. The firmware and the VCV
// plugin both map knobs and CVs through panelToParams, and the host tests
// render the documented patches through it, so a patch written as knob
// positions sounds the same everywhere.
//
//   WRATH  resonance: clean, then ringing, then it screams on its own, then the input stage squashes it
//   RUIN   corruption: the resonance folds, then bends its own cutoff, then is pushed over the edge
//   DOOM   undertones: an octave down, then two, then the undertone lurches the cutoff
#pragma once

#include "Curve.hpp"
#include "Doombringer.hpp"

namespace doom {

namespace macro {
// WRATH: the edge of self-oscillation (resonance 1) sits at 0.64.
constexpr Breakpoint kResonance[] = {{0.f, 0.f}, {0.6f, 0.97f}, {0.7f, 1.04f}, {1.f, 1.1f}};
constexpr Breakpoint kDrive[]     = {{0.5f, 0.f}, {1.f, 0.8f}};
// RUIN: the lower half folds and bends without adding gain, so a pinged
// filter still dies away (a kick whose pitch falls); the loop gain comes last.
constexpr Breakpoint kFold[]      = {{0.f, 0.f}, {0.5f, 0.6f}, {1.f, 1.f}};
constexpr Breakpoint kSelfFm[]    = {{0.25f, 0.f}, {0.75f, 0.7f}, {1.f, 1.f}};
constexpr Breakpoint kOverdrive[] = {{0.55f, 0.f}, {1.f, 0.6f}};
// DOOM
constexpr Breakpoint kSub1[]  = {{0.f, 0.f}, {0.35f, 0.9f}};
constexpr Breakpoint kSub2[]  = {{0.35f, 0.f}, {0.7f, 0.8f}};
constexpr Breakpoint kSubFm[] = {{0.7f, 0.f}, {1.f, 1.f}};
} // namespace macro

struct MacroValues {
    float wrath = 0.f; // 0..1, knob + CV
    float ruin  = 0.f;
    float doom  = 0.f;
};

inline void applyMacros(const MacroValues& m, Params& p) {
    const float wrath = clampf(m.wrath, 0.f, 1.f), ruin = clampf(m.ruin, 0.f, 1.f), doomV = clampf(m.doom, 0.f, 1.f);
    p.resonance = curve(macro::kResonance, wrath);
    p.drive     = curve(macro::kDrive, wrath);
    p.fold      = curve(macro::kFold, ruin);
    p.selfFm    = curve(macro::kSelfFm, ruin);
    p.overdrive = curve(macro::kOverdrive, ruin);
    p.sub1      = curve(macro::kSub1, doomV);
    p.sub2      = curve(macro::kSub2, doomV);
    p.subFm     = curve(macro::kSubFm, doomV);
}

// Knobs are 0..1 (7 o'clock .. 5 o'clock). CVs are volts at the jacks.
struct PanelState {
    float cutoff = 0.6f, wrath = 0.f, ruin = 0.f, doom = 0.f;        // front knobs
    float morph = 0.f, fm = 0.5f, hunger = 0.5f, fine = 0.5f;        // shift 1-4 (noon = none for FM, HUNGER, fine)
    bool  bassComp = true;                                           // toggle up
    float cutoffCv = 0.f, wrathCv = 0.f, ruinCv = 0.f, doomCv = 0.f; // volts
};

// The cutoff knob spans ten octaves, 16 Hz to 16.7 kHz (C4 at 0.4).
inline float cutoffKnobToV(float knob01) { return knob01 * 10.f - 4.f; }

// A knob whose noon means none, -1..1, with a dead band at noon: a real knob
// never sits exactly there, and "none" should be none (the FM upsampler idles).
inline float bipolarKnob(float knob01) {
    constexpr float kDead = 0.04f;
    const float     d     = clampf(knob01 * 2.f - 1.f, -1.f, 1.f);
    return std::fabs(d) < kDead ? 0.f : (d - std::copysign(kDead, d)) / (1.f - kDead);
}

inline Params panelToParams(const PanelState& s) {
    Params p;
    p.cutoffV  = cutoffKnobToV(s.cutoff) + (s.fine * 2.f - 1.f) / 12.f + s.cutoffCv;
    p.morph    = clampf(s.morph, 0.f, 1.f);
    p.fmDepth  = bipolarKnob(s.fm);
    p.hunger   = bipolarKnob(s.hunger);
    p.bassComp = s.bassComp;

    // The character macros: knob plus CV, where +-5 V sweeps the whole range.
    MacroValues m;
    m.wrath = s.wrath + s.wrathCv * 0.2f;
    m.ruin  = s.ruin + s.ruinCv * 0.2f;
    m.doom  = s.doom + s.doomCv * 0.2f;
    applyMacros(m, p);
    return p;
}

} // namespace doom
