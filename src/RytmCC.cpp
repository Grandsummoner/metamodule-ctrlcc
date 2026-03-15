#include "plugin.hpp"

struct RytmCC : Module {
    enum ParamIds {
        // Defining 6 knobs for now; add more to reach 24
        KNOB_1, KNOB_2, KNOB_3, KNOB_4, KNOB_5, KNOB_6,
        NUM_PARAMS
    };

    midi::OutputQueue midiOutput;

    RytmCC() {
        config(NUM_PARAMS, 0, 0, 0);
        configParam(KNOB_1, 0.f, 1.f, 0.f, "Filter Cutoff");
        configParam(KNOB_2, 0.f, 1.f, 0.f, "Resonance");
        // Add more configParams here matching your HTML tool CCs
    }

    void process(const ProcessArgs& args) override {
        // Logic to send MIDI CCs would go here
    }
};

// The Widget defines how the module looks on the hardware screen
struct RytmCCWidget : ModuleWidget {
    RytmCCWidget(RytmCC* module) {
        setModule(module);

        // Hardware requires these 'virtual' knobs to map physical controls
        addParam(createParam<Knob>(Vec(0, 0), module, RytmCC::KNOB_1));
        addParam(createParam<Knob>(Vec(20, 0), module, RytmCC::KNOB_2));
        addParam(createParam<Knob>(Vec(40, 0), module, RytmCC::KNOB_3));
        addParam(createParam<Knob>(Vec(60, 0), module, RytmCC::KNOB_4));
        addParam(createParam<Knob>(Vec(80, 0), module, RytmCC::KNOB_5));
        addParam(createParam<Knob>(Vec(100, 0), module, RytmCC::KNOB_6));
    }
};

Model* modelRytmCC = createModel<RytmCC, RytmCCWidget>("RytmCC");
