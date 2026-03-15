#include "CoreModules/CoreProcessor.hh"
#include "CoreModules/elements/elements.hh"
#include "CoreModules/elements/element_counter.hh"
#include "CoreModules/register_module.hh"
#include "midi/midi_message.hh"
#include "midi/midi_sender.hh"

#include <array>
#include <string_view>
#include <cstdint>

namespace RytmCC {

static constexpr int NumKnobs = 6;
static constexpr int NumSets  = 4;

// Default CC assignments per set — Performance macros, Filter, Amp
static constexpr uint8_t DefaultCC[NumSets][NumKnobs] = {
    {35, 36, 37, 39, 40, 41},  // Set 1: Perf 1-6
    {42, 43, 44, 45, 46, 47},  // Set 2: Perf 7-12
    {74, 75, 70, 71, 78, 80},  // Set 3: Filter + Amp
    {7,  10, 82, 83, 102, 109} // Set 4: Vol, Pan, Sends, LFO
};

static constexpr uint8_t DefaultCh[NumSets][NumKnobs] = {
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 0},
};

enum ParamIDs {
    Knob1, Knob2, Knob3, Knob4, Knob5, Knob6,
    SetBtn,
    NumParams
};

class RytmCCModule : public MetaModule::CoreProcessor {
public:
    RytmCCModule() {
        for (int s = 0; s < NumSets; s++) {
            for (int k = 0; k < NumKnobs; k++) {
                cc[s][k]  = DefaultCC[s][k];
                ch[s][k]  = DefaultCh[s][k];
            }
        }
        for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
    }

    void update() override {
        // Set button — cycle on rising edge
        float btnNow = get_param(SetBtn);
        if (btnNow > 0.5f && lastBtn <= 0.5f) {
            activeSet = (activeSet + 1) % NumSets;
            for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
        }
        lastBtn = btnNow;

        // Check each knob and send MIDI CC if changed
        for (int k = 0; k < NumKnobs; k++) {
            float v = get_param(Knob1 + k);
            if (v != lastVal[k]) {
                lastVal[k] = v;
                uint8_t midiVal = static_cast<uint8_t>(v * 127.f);
                uint8_t ccNum   = cc[activeSet][k];
                uint8_t chNum   = ch[activeSet][k];

                MetaModule::MidiMessage msg;
                msg.bytes[0] = 0xB0 | (chNum & 0x0F);
                msg.bytes[1] = ccNum & 0x7F;
                msg.bytes[2] = midiVal & 0x7F;
                MetaModule::midi_send(msg);
            }
        }
    }

    void set_param(int param_id, float val) override {
        if (param_id < NumParams) params[param_id] = val;
    }

    float get_param(int param_id) const override {
        if (param_id < NumParams) return params[param_id];
        return 0.f;
    }

    void set_input(int, float) override {}
    float get_output(int) const override { return 0.f; }
    void set_light_brightness(int, float) override {}
    float get_light_brightness(int) const override { return 0.f; }

private:
    float params[NumParams] = {};
    float lastVal[NumKnobs] = {};
    float lastBtn = 0.f;
    int   activeSet = 0;

    uint8_t cc[NumSets][NumKnobs];
    uint8_t ch[NumSets][NumKnobs];
};

// Element layout — 6 knobs in 2 rows of 3 + 1 button
static std::array<MetaModule::Element, NumParams> elements;
static std::array<ElementCount::Indices, NumParams> indices;

void setupElements() {
    // Row 1 knobs
    float rowY1 = 45.f, rowY2 = 75.f;
    float colX[3] = {15.f, 30.f, 45.f};
    const char* names[NumKnobs] = {"K1","K2","K3","K4","K5","K6"};

    for (int k = 0; k < NumKnobs; k++) {
        MetaModule::Knob knob;
        knob.x_mm      = colX[k % 3];
        knob.y_mm      = (k < 3) ? rowY1 : rowY2;
        knob.short_name = names[k];
        knob.image     = "RytmCC/comps/knob.png";
        elements[k]    = knob;
        indices[k]     = {.param_idx = static_cast<uint16_t>(k)};
    }

    // Set button
    MetaModule::MomentaryButton btn;
    btn.x_mm       = 30.f;
    btn.y_mm       = 95.f;
    btn.short_name  = "Set";
    btn.image      = "RytmCC/comps/button.png";
    elements[SetBtn] = btn;
    indices[SetBtn]  = {.param_idx = SetBtn};
}

} // namespace RytmCC

void init() {
    RytmCC::setupElements();

    MetaModule::ModuleInfoView info {
        .elements = RytmCC::elements,
        .indices  = RytmCC::indices,
        .description = "6-knob MIDI CC for Elektron Analog Rytm MkII",
        .width_hp = 8,
    };

    MetaModule::register_module(
        "RytmCC",
        "RytmCC",
        []() -> std::unique_ptr<MetaModule::CoreProcessor> {
            return std::make_unique<RytmCC::RytmCCModule>();
        },
        info,
        "RytmCC/RytmCC.png"
    );
}
