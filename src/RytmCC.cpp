#include "CoreModules/CoreProcessor.hh"
#include "CoreModules/elements/elements.hh"
#include "CoreModules/register_module.hh"

#include "rack.hpp"

#include <array>
#include <string_view>
#include <cstdint>
#include <memory>

namespace RytmCC {

static constexpr int NumKnobs = 6;
static constexpr int NumSets  = 4;

static constexpr uint8_t DefaultCC[NumSets][NumKnobs] = {
    {35, 36, 37, 39, 40, 41},
    {42, 43, 44, 45, 46, 47},
    {74, 75, 70, 71, 78, 80},
    {7,  10, 82, 83, 102, 109}
};

enum ParamIDs {
    Knob1, Knob2, Knob3, Knob4, Knob5, Knob6,
    SetBtn,
    NumParams
};

// Static strings — must outlive module
static const char* knobNames[NumKnobs] = {"K1","K2","K3","K4","K5","K6"};
static const char* setBtnName = "Set";

class RytmCCModule : public MetaModule::CoreProcessor {
public:
    RytmCCModule() {
        for (int s = 0; s < NumSets; s++)
            for (int k = 0; k < NumKnobs; k++)
                cc[s][k] = DefaultCC[s][k];
        for (int k = 0; k < NumKnobs; k++)
            lastVal[k] = -1.f;
    }

    void set_samplerate(float sr) override { sampleRate = sr; }

    void update() override {
        // Set button rising edge detection
        float btnNow = params[SetBtn];
        if (btnNow > 0.5f && lastBtn <= 0.5f) {
            activeSet = (activeSet + 1) % NumSets;
            for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
        }
        lastBtn = btnNow;

        // Send MIDI CC for any changed knob
        for (int k = 0; k < NumKnobs; k++) {
            float v = params[Knob1 + k];
            if (v != lastVal[k]) {
                lastVal[k] = v;
                uint8_t val  = static_cast<uint8_t>(v * 127.f);
                uint8_t ccNum = cc[activeSet][k];

                rack::midi::Message msg;
                msg.setSize(3);
                msg.bytes[0] = 0xB0; // CC on channel 1
                msg.bytes[1] = ccNum & 0x7F;
                msg.bytes[2] = val & 0x7F;
                midiOut.sendMessage(msg);
            }
        }
    }

    void set_param(int id, float val) override {
        if (id >= 0 && id < NumParams) params[id] = val;
    }

    float get_param(int id) const override {
        if (id >= 0 && id < NumParams) return params[id];
        return 0.f;
    }

    void set_input(int, float) override {}
    float get_output(int) const override { return 0.f; }

    std::string save_state() override {
        std::string s;
        s += std::to_string(activeSet);
        return s;
    }

    void load_state(std::string_view sv) override {
        if (!sv.empty()) activeSet = sv[0] - '0';
        for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
    }

private:
    float params[NumParams] = {};
    float lastVal[NumKnobs] = {};
    float lastBtn  = 0.f;
    float sampleRate = 48000.f;
    int   activeSet  = 0;
    uint8_t cc[NumSets][NumKnobs];
    rack::midi::Output midiOut;
};

// ---- Static element storage ----
static std::array<MetaModule::Element, NumParams> elements;
static std::array<MetaModule::ElementCount::Indices, NumParams> indices;
static bool elementsReady = false;

static void setupElements() {
    if (elementsReady) return;

    float colX[3] = {14.f, 30.f, 46.f};
    float rowY[2] = {50.f, 75.f};

    for (int k = 0; k < NumKnobs; k++) {
        MetaModule::Knob knob;
        knob.x_mm       = colX[k % 3];
        knob.y_mm       = rowY[k / 3];
        knob.short_name = knobNames[k];
        knob.long_name  = knobNames[k];
        knob.image      = "";
        elements[k]     = knob;
        indices[k]      = {.param_idx = static_cast<uint16_t>(k)};
    }

    MetaModule::MomentaryButton btn;
    btn.x_mm       = 30.f;
    btn.y_mm       = 98.f;
    btn.short_name = setBtnName;
    btn.long_name  = setBtnName;
    btn.image      = "";
    elements[SetBtn] = btn;
    indices[SetBtn]  = {.param_idx = SetBtn};

    elementsReady = true;
}

} // namespace RytmCC

extern "C" void init() {
    RytmCC::setupElements();

    MetaModule::ModuleInfoView info;
    info.elements    = RytmCC::elements;
    info.indices     = RytmCC::indices;
    info.description = "6-knob MIDI CC for Elektron Analog Rytm MkII";
    info.width_hp    = 8;

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
