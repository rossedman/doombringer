// patch.Init() firmware for Doombringer, the ladder filter.
// All the sound lives in ../dsp — this file only maps panel + jacks to Params.
//
// Panel map (see README for the full table)
//                           normal                          | hold button (shift)
//   CV_1 knob + CV_5 jack : CUTOFF (jack = 1V/oct)          | MORPH (LP4, LP2, BP, HP2, HP4)
//   CV_2 knob + CV_6 jack : WRATH (resonance, drive)        | FM depth for IN_R (noon = none)
//   CV_3 knob + CV_7 jack : RUIN  (fold, self-FM, overdrive)| HUNGER (noon = none)
//   CV_4 knob + CV_8 jack : DOOM  (undertones, lurch)       | FINE tune (noon = 0, +-1 semitone)
//   (jacks always drive the front-panel parameter)
//   IN_L        : audio in                    IN_R : cutoff FM (audio rate)
//   GATE_IN_1   : ping                        GATE_IN_2 : restart the undertones
//   Button tap  : ping                        Toggle : up = bass compensation, down = thin
//   OUT_L       : filter (the morph)          OUT_R : four-pole low-pass
//   CV_OUT_1    : input envelope, 0-5 V       CV_OUT_2 (LED) : envelope / shift
#include "daisy_patch_sm.h"
#include "util/CpuLoadMeter.h"

#include "../dsp/Panel.hpp"
#include "../dsp/Pickup.hpp"

using namespace daisy;
using namespace patch_sm;

namespace {

// ---- hardware calibration ---------------------------------------------------
// Volts that correspond to a full-scale (+-1.0) codec sample. Too high = quieter (safe).
constexpr float kAudioFullScaleV = 9.f;
// The Patch SM's analog audio stages invert polarity; flip it back on the way
// in and out, so a filtered ramp keeps its direction.
constexpr bool kInvertAudio = true;
// 1V/oct trim for CV_5. Fill these from a two-point calibration
// (send exactly 1 V and 3 V; libDaisy's VoctCalibration does the math).
constexpr float kPitchCvScale  = 1.f;
constexpr float kPitchCvOffset = 0.f;

// A tap shorter than this (with no shift knob moved) pings the filter.
constexpr uint32_t kTapMs = 400;

// ---- state ------------------------------------------------------------------
DaisyPatchSM hw;
Switch       button, toggle;

doom::Engine<4> filter;
CpuLoadMeter    cpu; // build with CPU_METER=1 to print load over USB serial

enum Layer { kNormal = 0, kShift = 1 };
doom::PickupKnob knobs[2][4]; // [layer][knob]

bool     shiftWas     = false;
bool     shiftUsed    = false; // a shift knob was turned during this hold
bool     pingPending  = false;
uint32_t pressStartMs = 0;

inline float rawKnob(int i) { return doom::clampf(hw.GetAdcValue(CV_1 + i), 0.f, 1.f); }
inline float cvVolts(int idx) { return hw.GetAdcValue(idx) * 5.f; } // CV_5..8: +-5 V

void handleButtonAndLayers() {
    button.Debounce();
    toggle.Debounce();

    const bool shift = button.Pressed();
    if (shift != shiftWas) {
        const int active = shift ? kShift : kNormal;
        for (int i = 0; i < 4; ++i) knobs[active][i].lock(rawKnob(i));
        if (shift) {
            pressStartMs = System::GetNow();
            shiftUsed    = false;
        } else if (!shiftUsed && System::GetNow() - pressStartMs < kTapMs) {
            pingPending = true; // it was a tap
        }
        shiftWas = shift;
    }

    const int active = shift ? kShift : kNormal;
    for (int i = 0; i < 4; ++i)
        if (knobs[active][i].update(rawKnob(i)) && shift) shiftUsed = true;
}

void AudioCallback(AudioHandle::InputBuffer in, AudioHandle::OutputBuffer out, size_t size) {
    cpu.OnBlockStart();
    hw.ProcessAllControls();
    handleButtonAndLayers(); // callback runs at 1 kHz, the debounce rate

    if (hw.gate_in_1.Trig() || pingPending) filter.ping();
    pingPending = false;
    if (hw.gate_in_2.Trig()) filter.resetDivider();

    const doom::PickupKnob* n = knobs[kNormal];
    const doom::PickupKnob* s = knobs[kShift];

    doom::PanelState ps;
    ps.cutoff   = n[0].value;
    ps.wrath    = n[1].value;
    ps.ruin     = n[2].value;
    ps.doom     = n[3].value;
    ps.morph    = s[0].value;
    ps.fm       = s[1].value;
    ps.hunger   = s[2].value;
    ps.fine     = s[3].value;
    ps.bassComp = toggle.Pressed();
    ps.cutoffCv = cvVolts(CV_5) * kPitchCvScale + kPitchCvOffset;
    ps.wrathCv  = cvVolts(CV_6);
    ps.ruinCv   = cvVolts(CV_7);
    ps.doomCv   = cvVolts(CV_8);
    filter.setParams(doom::panelToParams(ps));

    const float inScale  = (kInvertAudio ? -1.f : 1.f) * kAudioFullScaleV;
    const float outScale = (kInvertAudio ? -1.f : 1.f) / kAudioFullScaleV;
    for (size_t i = 0; i < size; ++i) {
        float main, lp;
        filter.process(in[0][i] * inScale, in[1][i] * inScale, main, lp);
        out[0][i] = main * outScale;
        out[1][i] = lp * outScale;
    }

    // CV outs are 0..5 V. The envelope HUNGER and DOOM follow, and the LED shows
    // it too (full while shift is held).
    const float env = doom::clampf(filter.envelopeV(), 0.f, 5.f);
    hw.WriteCvOut(CV_OUT_1, env);
    hw.WriteCvOut(CV_OUT_2, shiftWas ? 5.f : 0.3f + 0.94f * env);
    cpu.OnBlockEnd();
}

} // namespace

int main() {
    hw.Init();
    hw.SetAudioBlockSize(48); // 1 ms blocks at 48 kHz
    hw.SetAudioSampleRate(SaiHandle::Config::SampleRate::SAI_48KHZ);

    button.Init(DaisyPatchSM::B7);
    toggle.Init(DaisyPatchSM::B8);

    // Front knobs start where they physically are. Shift settings start at
    // their defaults (LP4, no FM, no HUNGER, in tune) and pick up the first
    // time you hold the button and sweep a knob through that value.
    hw.ProcessAllControls();
    for (int i = 0; i < 4; ++i) {
        knobs[kNormal][i].value = rawKnob(i);
        knobs[kNormal][i].last  = rawKnob(i);
    }
    const doom::PanelState d;
    knobs[kShift][0].value = d.morph;
    knobs[kShift][1].value = d.fm;
    knobs[kShift][2].value = d.hunger;
    knobs[kShift][3].value = d.fine;
    for (int i = 0; i < 4; ++i) knobs[kShift][i].fresh = true;

    filter.init(hw.AudioSampleRate());
    cpu.Init(hw.AudioSampleRate(), hw.AudioBlockSize());
#if CPU_METER
    hw.StartLog(false); // open a serial monitor on the USB port to read it
#endif
    hw.StartAudio(AudioCallback);

    uint32_t lastLog = 0;
    while (true) {
#if CPU_METER
        if (System::GetNow() - lastLog > 1000) {
            lastLog = System::GetNow();
            hw.PrintLine("cpu avg " FLT_FMT3 "%%  max " FLT_FMT3 "%%", FLT_VAR3(cpu.GetAvgCpuLoad() * 100.f), FLT_VAR3(cpu.GetMaxCpuLoad() * 100.f));
        }
#else
        (void)lastLog;
#endif
        System::Delay(10);
    }
}
