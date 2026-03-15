#include "plugin.hpp"

// 6 knobs, 4 sets, each knob has its own CC number and MIDI channel
// Sends MIDI CC out via USB on parameter change

static const int NUM_KNOBS = 6;
static const int NUM_SETS = 4;

struct KnobConfig {
    uint8_t cc = 0;
    uint8_t channel = 0; // 0-indexed (0 = ch1)
};

struct RytmCC : Module {
    enum ParamIds {
        KNOB1_PARAM, KNOB2_PARAM, KNOB3_PARAM,
        KNOB4_PARAM, KNOB5_PARAM, KNOB6_PARAM,
        SET_BUTTON_PARAM,
        NUM_PARAMS
    };
    enum InputIds { NUM_INPUTS };
    enum OutputIds { NUM_OUTPUTS };
    enum LightIds {
        SET1_LIGHT, SET2_LIGHT, SET3_LIGHT, SET4_LIGHT,
        NUM_LIGHTS
    };

    KnobConfig configs[NUM_SETS][NUM_KNOBS];
    int activeSet = 0;
    float lastValues[NUM_KNOBS] = {};
    float setButtonTimer = 0.f;

    // Display
    std::string displayText = "Rytm CC Controller";
    std::string displaySub = "Turn a knob to send CC";
    float displayTimer = 0.f;

    // Knob names from patch builder export
    std::string knobNames[NUM_SETS][NUM_KNOBS];

    RytmCC() {
        config(NUM_PARAMS, NUM_INPUTS, NUM_OUTPUTS, NUM_LIGHTS);
        for (int i = 0; i < NUM_KNOBS; i++) {
            configParam(KNOB1_PARAM + i, 0.f, 1.f, 0.f,
                string::f("Knob %d", i + 1));
        }
        configButton(SET_BUTTON_PARAM, "Set");

        // Default CC assignments (Performance macros on ch1)
        uint8_t defaultCCs[NUM_KNOBS] = {35, 36, 37, 39, 40, 41};
        for (int s = 0; s < NUM_SETS; s++) {
            for (int k = 0; k < NUM_KNOBS; k++) {
                configs[s][k].cc = defaultCCs[k];
                configs[s][k].channel = 0;
                knobNames[s][k] = string::f("Perf %d", k + 1);
            }
        }

        for (int k = 0; k < NUM_KNOBS; k++) {
            lastValues[k] = -1.f;
        }
    }

    void process(const ProcessArgs& args) override {
        // Set button — cycle through sets
        if (params[SET_BUTTON_PARAM].getValue() > 0.5f) {
            if (setButtonTimer <= 0.f) {
                activeSet = (activeSet + 1) % NUM_SETS;
                setButtonTimer = 0.5f;
                displayText = string::f("Set %d", activeSet + 1);
                displaySub = "";
                displayTimer = 2.f;
                for (int k = 0; k < NUM_KNOBS; k++) lastValues[k] = -1.f;
            }
        }
        if (setButtonTimer > 0.f) setButtonTimer -= args.sampleTime;

        // Update set lights
        for (int i = 0; i < NUM_SETS; i++) {
            lights[SET1_LIGHT + i].setBrightness(i == activeSet ? 1.f : 0.08f);
        }

        // Display timer
        if (displayTimer > 0.f) {
            displayTimer -= args.sampleTime;
            if (displayTimer <= 0.f) {
                displayText = string::f("Set %d", activeSet + 1);
                displaySub = "";
            }
        }

        // Check knobs and send MIDI CC
        for (int k = 0; k < NUM_KNOBS; k++) {
            float val = params[KNOB1_PARAM + k].getValue();
            if (val != lastValues[k]) {
                lastValues[k] = val;
                uint8_t midiVal = (uint8_t)(val * 127.f);
                uint8_t cc = configs[activeSet][k].cc;
                uint8_t ch = configs[activeSet][k].channel;

                // Send MIDI CC
                midi::Message msg;
                msg.setSize(3);
                msg.bytes[0] = 0xB0 | (ch & 0x0F);
                msg.bytes[1] = cc & 0x7F;
                msg.bytes[2] = midiVal & 0x7F;
                midiOutput.sendMessage(msg);

                // Update display
                displayText = knobNames[activeSet][k];
                displaySub = string::f("CC %d  |  %d", cc, midiVal);
                displayTimer = 2.f;
            }
        }
    }

    void onReset() override {
        activeSet = 0;
        for (int k = 0; k < NUM_KNOBS; k++) lastValues[k] = -1.f;
    }

    json_t* dataToJson() override {
        json_t* root = json_object();
        json_object_set_new(root, "activeSet", json_integer(activeSet));
        json_t* setsArr = json_array();
        for (int s = 0; s < NUM_SETS; s++) {
            json_t* setObj = json_object();
            json_t* knobsArr = json_array();
            for (int k = 0; k < NUM_KNOBS; k++) {
                json_t* kObj = json_object();
                json_object_set_new(kObj, "cc", json_integer(configs[s][k].cc));
                json_object_set_new(kObj, "channel", json_integer(configs[s][k].channel));
                json_object_set_new(kObj, "name", json_string(knobNames[s][k].c_str()));
                json_array_append_new(knobsArr, kObj);
            }
            json_object_set_new(setObj, "knobs", knobsArr);
            json_array_append_new(setsArr, setObj);
        }
        json_object_set_new(root, "sets", setsArr);
        return root;
    }

    void dataFromJson(json_t* root) override {
        json_t* as = json_object_get(root, "activeSet");
        if (as) activeSet = json_integer_value(as);
        json_t* setsArr = json_object_get(root, "sets");
        if (setsArr) {
            for (int s = 0; s < NUM_SETS && s < (int)json_array_size(setsArr); s++) {
                json_t* setObj = json_array_get(setsArr, s);
                json_t* knobsArr = json_object_get(setObj, "knobs");
                if (knobsArr) {
                    for (int k = 0; k < NUM_KNOBS && k < (int)json_array_size(knobsArr); k++) {
                        json_t* kObj = json_array_get(knobsArr, k);
                        json_t* cc = json_object_get(kObj, "cc");
                        json_t* ch = json_object_get(kObj, "channel");
                        json_t* nm = json_object_get(kObj, "name");
                        if (cc) configs[s][k].cc = json_integer_value(cc);
                        if (ch) configs[s][k].channel = json_integer_value(ch);
                        if (nm) knobNames[s][k] = json_string_value(nm);
                    }
                }
            }
        }
    }

    midi::Output midiOutput;
};

// ---- Widget ----

struct RytmCCDisplay : TransparentWidget {
    RytmCC* module = nullptr;

    void draw(const DrawArgs& args) override {
        if (!module) return;
        NVGcontext* vg = args.vg;

        // Background
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 0, 0, box.size.x, box.size.y, 4);
        nvgFillColor(vg, nvgRGB(12, 12, 18));
        nvgFill(vg);

        // Border
        nvgBeginPath(vg);
        nvgRoundedRect(vg, 0.5, 0.5, box.size.x - 1, box.size.y - 1, 4);
        nvgStrokeColor(vg, nvgRGB(60, 60, 80));
        nvgStrokeWidth(vg, 0.5f);
        nvgStroke(vg);

        // Main text
        nvgFontSize(vg, 11.f);
        nvgFontFaceId(vg, APP->window->uiFont->handle);
        nvgTextAlign(vg, NVG_ALIGN_CENTER | NVG_ALIGN_MIDDLE);
        nvgFillColor(vg, nvgRGB(220, 220, 255));
        nvgText(vg, box.size.x * 0.5f, box.size.y * 0.35f,
            module->displayText.c_str(), nullptr);

        // Sub text
        nvgFontSize(vg, 9.f);
        nvgFillColor(vg, nvgRGB(120, 200, 120));
        nvgText(vg, box.size.x * 0.5f, box.size.y * 0.68f,
            module->displaySub.c_str(), nullptr);
    }
};

struct SetLight : GrayModuleLightWidget {
    SetLight() { addBaseColor(SCHEME_GREEN); }
};

static const NVGcolor KNOB_COLORS[6] = {
    nvgRGB(220, 48, 48),   // red
    nvgRGB(220, 120, 32),  // orange
    nvgRGB(200, 168, 0),   // yellow
    nvgRGB(40, 160, 80),   // green
    nvgRGB(24, 120, 208),  // blue
    nvgRGB(128, 64, 192),  // purple
};

struct ColoredKnob : RoundKnob {
    int colorIndex = 0;
    ColoredKnob() { setSvg(Svg::load(asset::plugin(pluginInstance, "res/knob.svg"))); }
};

struct RytmCCWidget : ModuleWidget {
    RytmCCWidget(RytmCC* module) {
        setModule(module);
        setPanel(createPanel(asset::plugin(pluginInstance, "res/RytmCC.svg")));

        // Screws
        addChild(createWidget<ScrewSilver>(Vec(4, 4)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 16, 4)));
        addChild(createWidget<ScrewSilver>(Vec(4, RACK_GRID_HEIGHT - RACK_GRID_WIDTH + 4)));
        addChild(createWidget<ScrewSilver>(Vec(box.size.x - 16, RACK_GRID_HEIGHT - RACK_GRID_WIDTH + 4)));

        // Display
        RytmCCDisplay* display = createWidget<RytmCCDisplay>(mm2px(Vec(2, 8)));
        display->box.size = mm2px(Vec(56, 14));
        display->module = module;
        addChild(display);

        // Set lights
        float lx = mm2px(5.f);
        for (int i = 0; i < NUM_SETS; i++) {
            addChild(createLightCentered<SmallLight<GreenLight>>(
                mm2px(Vec(lx + i * 8.f, 26.f)),
                module, RytmCC::SET1_LIGHT + i));
        }

        // Set button
        addParam(createParamCentered<TL1105>(
            mm2px(Vec(50.f, 26.f)), module, RytmCC::SET_BUTTON_PARAM));

        // 6 knobs — 2 rows of 3
        float kx[3] = {12.f, 30.f, 48.f};
        float ky[2] = {42.f, 62.f};
        for (int row = 0; row < 2; row++) {
            for (int col = 0; col < 3; col++) {
                int k = row * 3 + col;
                addParam(createParamCentered<RoundLargeBlackKnob>(
                    mm2px(Vec(kx[col], ky[row])),
                    module, RytmCC::KNOB1_PARAM + k));
            }
        }

        // MIDI output port
        addChild(createWidget<MidiWidget>(mm2px(Vec(2, 76))));
    }
};

Model* modelRytmCC = createModel<RytmCC, RytmCCWidget>("RytmCC");
