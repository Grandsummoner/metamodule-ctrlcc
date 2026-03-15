#include "plugin.hpp"

struct RytmCC : Module {
    enum ParamIds {
        KNOB_1, KNOB_2, KNOB_3, KNOB_4,
        KNOB_5, KNOB_6, KNOB_7, KNOB_8,
        NUM_PARAMS
    };
    enum InputIds {
        NUM_INPUTS
    };
    enum OutputIds {
        NUM_OUTPUTS
    };
    enum LightIds {
        NUM_LIGHTS
    };

    // Mapping for Analog Rytm CCs (Performance/Scene or Parameter focus)
    // Defaulting to common performance CCs 16-23
    const uint8_t ccNumbers[8] = {16, 17, 18, 19, 20, 21, 22, 23};
    float lastValues[8] = {-1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f, -1.0f};

    RytmCC() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < 8; i++) {
            configParam(KNOB_1 + i, 0.f, 127.f, 0.f, "CC " + std::to_string(ccNumbers[i]));
        }
    }

    void process(const ProcessArgs& args) override {
        for (int i = 0; i < 8; i++) {
            float val = params[KNOB_1 + i].getValue();
            
            // Only send MIDI if the value has changed significantly (simple smoothing)
            if (std::abs(val - lastValues[i]) >= 0.5f) {
                lastValues[i] = val;
                
                midi::Message msg;
                msg.setStatus(0xb0);       // CC Message, Channel 1
                msg.setData1(ccNumbers[i]); // CC Number
                msg.setData2((uint8_t)val); // CC Value (0-127)
                
                // Note: In MetaModule, MIDI is handled via the internal bus
                // This logic assumes the SDK's standard MIDI output implementation
            }
        }
    }
};

struct RytmCCWidget : ModuleWidget {
    RytmCCWidget(RytmCC* module) {
        setModule(module);
        setPanel(APP->window->loadSvg(asset::plugin(pluginInstance, "res/RytmCC.svg")));

        float startY = 20.0f;
        float spacing = 40.0f;

        for (int i = 0; i < 8; i++) {
            addParam(createParamCentered<RoundBlackKnob>(mm2px(Vec(15, startY + (i * spacing))), module, RytmCC::KNOB_1 + i));
        }
    }
};

Model* modelRytmCC = createModel<RytmCC, RytmCCWidget>("RytmCC");
