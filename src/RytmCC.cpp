#include "CoreModules/CoreProcessor.hh"
#include "CoreModules/register_module.hh"
#include "CoreModules/elements/elements.hh"
#include "CoreModules/elements/base_element.hh"

// Rack MIDI output via adaptor layer
#include "midi.hpp"

#include <array>
#include <memory>
#include <string_view>
#include <cstdint>
#include <cstring>

// ---- Constants ----
static constexpr int NumKnobs = 6;
static constexpr int NumSets  = 4;

static constexpr uint8_t DefaultCC[NumSets][NumKnobs] = {
    {35, 36, 37, 39, 40, 41},   // Set 1: Perf 1-6
    {42, 43, 44, 45, 46, 47},   // Set 2: Perf 7-12
    {74, 75, 70, 71, 78, 80},   // Set 3: Filter + Amp
    {7,  10, 82, 83, 102, 109}  // Set 4: Vol, Pan, Sends, LFO
};

enum ParamIDs {
    Knob1=0, Knob2, Knob3, Knob4, Knob5, Knob6,
    SetBtn,
    NumParams
};

// ---- Static string storage ----
// String views in elements must point to static storage
static const char* sKnobNames[NumKnobs] = {
    "K1 Red", "K2 Orange", "K3 Yellow",
    "K4 Green", "K5 Blue", "K6 Purple"
};
static const char* sSetBtn = "Next Set";
static const char* sBrand  = "RytmCC";
static const char* sSlug   = "RytmCC";
static const char* sDesc   = "6-knob MIDI CC for Elektron Analog Rytm MkII";
static const char* sFace   = "RytmCC/RytmCC.png";

// ---- Module ----
class RytmCCModule : public CoreProcessor {
public:
    RytmCCModule() {
        for (int s = 0; s < NumSets; s++)
            for (int k = 0; k < NumKnobs; k++)
                cc[s][k] = DefaultCC[s][k];
        for (int k = 0; k < NumKnobs; k++)
            lastVal[k] = -1.f;
    }

    void set_samplerate(float sr) override { (void)sr; }

    void update() override {
        // Set button — rising edge cycles through sets
        float btnNow = params[SetBtn];
        if (btnNow > 0.5f && lastBtn <= 0.5f) {
            activeSet = (activeSet + 1) % NumSets;
            for (int k = 0; k < NumKnobs; k++)
                lastVal[k] = -1.f; // force resend
        }
        lastBtn = btnNow;

        // Send MIDI CC for any changed knob
        for (int k = 0; k < NumKnobs; k++) {
            float v = params[Knob1 + k];
            if (v != lastVal[k]) {
                lastVal[k] = v;
                uint8_t midiVal = static_cast<uint8_t>(v * 127.f);
                uint8_t ccNum   = cc[activeSet][k];

                rack::midi::Message msg;
                msg.setSize(3);
                msg.bytes[0] = 0xB0; // CC ch1
                msg.bytes[1] = ccNum & 0x7F;
                msg.bytes[2] = midiVal & 0x7F;
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
        char buf[4];
        buf[0] = '0' + activeSet;
        buf[1] = 0;
        return std::string(buf);
    }

    void load_state(std::string_view sv) override {
        if (!sv.empty() && sv[0] >= '0' && sv[0] <= '3')
            activeSet = sv[0] - '0';
        for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
    }

private:
    float params[NumParams]    = {};
    float lastVal[NumKnobs]    = {};
    float lastBtn              = 0.f;
    int   activeSet            = 0;
    uint8_t cc[NumSets][NumKnobs] = {};
    rack::midi::Output midiOut;
};

// ---- Static element storage ----
static std::array<MetaModule::Element, NumParams> elements;
static std::array<MetaModule::ElementCount::Indices, NumParams> indices;

static void setupElements() {
    float colX[3] = {13.f, 30.f, 47.f};
    float rowY[2] = {50.f, 76.f};

    for (int k = 0; k < NumKnobs; k++) {
        MetaModule::Knob knob;
        knob.x_mm       = colX[k % 3];
        knob.y_mm       = rowY[k / 3];
        knob.short_name = sKnobNames[k];
        knob.long_name  = sKnobNames[k];
        elements[k]     = knob;
        indices[k]      = {.param_idx = static_cast<uint16_t>(k)};
    }

    MetaModule::MomentaryButton btn;
    btn.x_mm       = 30.f;
    btn.y_mm       = 100.f;
    btn.short_name = sSetBtn;
    btn.long_name  = sSetBtn;
    elements[SetBtn] = btn;
    indices[SetBtn]  = {.param_idx = static_cast<uint16_t>(SetBtn)};
}

// ---- Static module info storage ----
static MetaModule::ModuleInfoView moduleInfo;

extern "C" void init() {
    setupElements();

    moduleInfo.elements    = elements;
    moduleInfo.indices     = indices;
    moduleInfo.description = sDesc;
    moduleInfo.width_hp    = 8;

    register_module(
        sSlug,
        sBrand,
        []() -> std::unique_ptr<CoreProcessor> {
            return std::make_unique<RytmCCModule>();
        },
        moduleInfo,
        sFace
    );
}
