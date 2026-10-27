// VCV Rack adapter for Doombringer, the ladder filter. The engine is the exact
// same ../dsp code the patch.Init firmware runs, and the panel mirrors the
// hardware: the cutoff and the three macros on top, the patch.Init's
// shift-layer settings below. Knobs and CVs go through the same
// doom::panelToParams the firmware uses.
#include "PanelLayout.hpp"
#include "plugin.hpp"
#include "../../dsp/Panel.hpp"

struct DoombringerModule : Module {
    enum ParamId {
        CUTOFF_PARAM,
        WRATH_PARAM,
        RUIN_PARAM,
        DOOM_PARAM,
        MORPH_PARAM,
        FM_PARAM,
        HUNGER_PARAM,
        FINE_PARAM,
        BASS_COMP_PARAM,
        PING_PARAM,
        PARAMS_LEN
    };
    enum InputId { CUTOFF_INPUT, WRATH_INPUT, RUIN_INPUT, DOOM_INPUT, AUDIO_INPUT, FM_INPUT, PING_INPUT, RESET_INPUT, INPUTS_LEN };
    enum OutputId { ENV_OUTPUT, LP4_OUTPUT, MAIN_OUTPUT, OUTPUTS_LEN };
    enum LightId { ENV_LIGHT, LIGHTS_LEN };

    doom::Engine<4>     filter;
    dsp::SchmittTrigger pingTrigger, resetTrigger;
    dsp::BooleanTrigger pingButton;
    dsp::ClockDivider   paramDivider;

    // Names the morph's stops, and the blend between two of them.
    struct MorphQuantity : ParamQuantity {
        std::string getDisplayValueString() override {
            static const char* names[doom::kNumModes] = {"LP4", "LP2", "BP", "HP2", "HP4"};
            const float        pos                    = getValue() * (doom::kNumModes - 1);
            const int          i                      = std::min(static_cast<int>(pos), doom::kNumModes - 2);
            const float        t                      = pos - static_cast<float>(i);
            if (t < 0.02f) return names[i];
            if (t > 0.98f) return names[i + 1];
            return string::f("%s %d%% %s", names[i], static_cast<int>(std::round((1.f - t) * 100.f)), names[i + 1]);
        }
    };

    DoombringerModule() {
        config(PARAMS_LEN, INPUTS_LEN, OUTPUTS_LEN, LIGHTS_LEN);
        const doom::PanelState d;
        // Cutoff spans ten octaves: Hz = (C4 / 16) * 1024^knob, 16 Hz to 16.7 kHz.
        configParam(CUTOFF_PARAM, 0.f, 1.f, d.cutoff, "Cutoff", " Hz", 1024.f, dsp::FREQ_C4 / 16.f);
        configParam(WRATH_PARAM, 0.f, 1.f, 0.f, "Wrath: resonance, drive", "%", 0.f, 100.f);
        configParam(RUIN_PARAM, 0.f, 1.f, 0.f, "Ruin: fold, self-FM, overdrive", "%", 0.f, 100.f);
        configParam(DOOM_PARAM, 0.f, 1.f, 0.f, "Doom: undertones, lurch", "%", 0.f, 100.f);
        configParam<MorphQuantity>(MORPH_PARAM, 0.f, 1.f, d.morph, "Morph");
        configParam(FM_PARAM, 0.f, 1.f, d.fm, "FM depth", "%", 0.f, 200.f, -100.f);
        configParam(HUNGER_PARAM, 0.f, 1.f, d.hunger, "Hunger: envelope to cutoff", "%", 0.f, 200.f, -100.f);
        configParam(FINE_PARAM, 0.f, 1.f, d.fine, "Fine tune", " semitones", 0.f, 2.f, -1.f);
        configSwitch(BASS_COMP_PARAM, 0.f, 1.f, 1.f, "Bass", {"Thin (the loop eats the bass)", "Compensated"});
        configButton(PING_PARAM, "Ping");
        configInput(CUTOFF_INPUT, "Cutoff 1V/oct");
        configInput(WRATH_INPUT, "Wrath CV (+-5 V sweeps the range)");
        configInput(RUIN_INPUT, "Ruin CV (+-5 V sweeps the range)");
        configInput(DOOM_INPUT, "Doom CV (+-5 V sweeps the range)");
        configInput(AUDIO_INPUT, "Audio");
        configInput(FM_INPUT, "Cutoff FM (audio rate)");
        configInput(PING_INPUT, "Ping (trigger)");
        configInput(RESET_INPUT, "Restart the undertones (trigger)");
        configOutput(ENV_OUTPUT, "Input envelope (0-5 V)");
        configOutput(LP4_OUTPUT, "Four-pole low-pass");
        configOutput(MAIN_OUTPUT, "Filter (morph)");
        configLight(ENV_LIGHT, "Input envelope");
        configBypass(AUDIO_INPUT, MAIN_OUTPUT);
        paramDivider.setDivision(4);
        filter.init(48000.f);
    }

    void updateParams() {
        doom::PanelState s;
        s.cutoff   = params[CUTOFF_PARAM].getValue();
        s.wrath    = params[WRATH_PARAM].getValue();
        s.ruin     = params[RUIN_PARAM].getValue();
        s.doom     = params[DOOM_PARAM].getValue();
        s.morph    = params[MORPH_PARAM].getValue();
        s.fm       = params[FM_PARAM].getValue();
        s.hunger   = params[HUNGER_PARAM].getValue();
        s.fine     = params[FINE_PARAM].getValue();
        s.bassComp = params[BASS_COMP_PARAM].getValue() > 0.5f;
        s.cutoffCv = inputs[CUTOFF_INPUT].getVoltage();
        s.wrathCv  = inputs[WRATH_INPUT].getVoltage();
        s.ruinCv   = inputs[RUIN_INPUT].getVoltage();
        s.doomCv   = inputs[DOOM_INPUT].getVoltage();
        filter.setParams(doom::panelToParams(s));
    }

    void process(const ProcessArgs& args) override {
        if (args.sampleRate != filter.sampleRate()) {
            filter.init(args.sampleRate);
            updateParams();
            filter.snapSmoothers();
        }
        const bool pinged = pingTrigger.process(inputs[PING_INPUT].getVoltage(), 0.1f, 1.f);
        if (pingButton.process(params[PING_PARAM].getValue() > 0.f) || pinged) filter.ping();
        if (resetTrigger.process(inputs[RESET_INPUT].getVoltage(), 0.1f, 1.f)) filter.resetDivider();
        if (paramDivider.process()) updateParams();

        float main, lp;
        filter.process(inputs[AUDIO_INPUT].getVoltage(), inputs[FM_INPUT].getVoltage(), main, lp);
        const float env = filter.envelopeV();
        outputs[MAIN_OUTPUT].setVoltage(main);
        outputs[LP4_OUTPUT].setVoltage(lp);
        outputs[ENV_OUTPUT].setVoltage(std::min(env, 10.f));
        lights[ENV_LIGHT].setBrightnessSmooth(env * 0.2f, args.sampleTime);
    }
};

struct DoombringerWidget : ModuleWidget {
    explicit DoombringerWidget(DoombringerModule* module) {
        using M = DoombringerModule;
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/Doombringer.svg")));

        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, 0)));
        addChild(createWidget<ScrewSilver>(Vec(RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 2 * RACK_GRID_WIDTH, RACK_GRID_HEIGHT - RACK_GRID_WIDTH)));

        auto at = [](panel::Pos p) { return mm2px(Vec(p.x, p.y)); };
        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::CUTOFF_PARAM), module, M::CUTOFF_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::WRATH_PARAM), module, M::WRATH_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::RUIN_PARAM), module, M::RUIN_PARAM));
        addParam(createParamCentered<RoundLargeBlackKnob>(at(panel::DOOM_PARAM), module, M::DOOM_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::MORPH_PARAM), module, M::MORPH_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::FM_PARAM), module, M::FM_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::HUNGER_PARAM), module, M::HUNGER_PARAM));
        addParam(createParamCentered<RoundBlackKnob>(at(panel::FINE_PARAM), module, M::FINE_PARAM));
        addParam(createParamCentered<CKSS>(at(panel::BASS_COMP_PARAM), module, M::BASS_COMP_PARAM));
        addParam(createParamCentered<VCVButton>(at(panel::PING_PARAM), module, M::PING_PARAM));
        addChild(createLightCentered<MediumLight<RedLight>>(at(panel::ENV_LIGHT), module, M::ENV_LIGHT));

        addInput(createInputCentered<PJ301MPort>(at(panel::CUTOFF_INPUT), module, M::CUTOFF_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::WRATH_INPUT), module, M::WRATH_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::RUIN_INPUT), module, M::RUIN_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::DOOM_INPUT), module, M::DOOM_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::AUDIO_INPUT), module, M::AUDIO_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::FM_INPUT), module, M::FM_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::PING_INPUT), module, M::PING_INPUT));
        addInput(createInputCentered<PJ301MPort>(at(panel::RESET_INPUT), module, M::RESET_INPUT));

        addOutput(createOutputCentered<PJ301MPort>(at(panel::ENV_OUTPUT), module, M::ENV_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(at(panel::LP4_OUTPUT), module, M::LP4_OUTPUT));
        addOutput(createOutputCentered<PJ301MPort>(at(panel::MAIN_OUTPUT), module, M::MAIN_OUTPUT));
    }
};

Model* modelDoombringer = createModel<DoombringerModule, DoombringerWidget>("Doombringer");
