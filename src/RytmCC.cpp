#include "CoreModules/CoreProcessor.hh"
#include "CoreModules/register_module.hh"
#include "CoreModules/elements/elements.hh"
#include "CoreModules/elements/element_counter.hh"
#include "CoreModules/elements/element_info.hh"
#include "midi.hpp"

#include <array>
#include <memory>
#include <string_view>
#include <cstdint>

// ============================================================
// Constants
// ============================================================
static constexpr int NumKnobs = 6;
static constexpr int NumSets  = 4;

static constexpr uint8_t DefaultCC[NumSets][NumKnobs] = {
    {35, 36, 37, 39, 40, 41},    // Set 1: Perf 1-6
    {42, 43, 44, 45, 46, 47},    // Set 2: Perf 7-12
    {74, 75, 70, 71, 78, 80},    // Set 3: Filter + Amp
    {7,  10, 82, 83, 102, 109},  // Set 4: Vol, Pan, Sends, LFO
};

// ============================================================
// Module info struct — the SDK template approach
// ============================================================
namespace RytmCC {

struct Info : MetaModule::ModuleInfoBase {
    static constexpr std::string_view slug        = "RytmCC";
    static constexpr std::string_view description = "6-knob MIDI CC for Elektron Analog Rytm MkII";
    static constexpr uint32_t         width_hp    = 8;
    static constexpr std::string_view png_filename = "RytmCC/RytmCC.png";
    static constexpr std::string_view svg_filename = "";

    static constexpr std::array<MetaModule::Element, 7> Elements = {{
        // Row 1: Knobs 1-3 (Red, Orange, Yellow)
        MetaModule::Knob{{ .x_mm=13.f, .y_mm=50.f, .short_name="K1", .long_name="Knob 1" }},
        MetaModule::Knob{{ .x_mm=30.f, .y_mm=50.f, .short_name="K2", .long_name="Knob 2" }},
        MetaModule::Knob{{ .x_mm=47.f, .y_mm=50.f, .short_name="K3", .long_name="Knob 3" }},
        // Row 2: Knobs 4-6 (Green, Blue, Purple)
        MetaModule::Knob{{ .x_mm=13.f, .y_mm=76.f, .short_name="K4", .long_name="Knob 4" }},
        MetaModule::Knob{{ .x_mm=30.f, .y_mm=76.f, .short_name="K5", .long_name="Knob 5" }},
        MetaModule::Knob{{ .x_mm=47.f, .y_mm=76.f, .short_name="K6", .long_name="Knob 6" }},
        // Set button
        MetaModule::MomentaryButton{{ .x_mm=30.f, .y_mm=100.f, .short_name="Set", .long_name="Next Set" }},
    }};

    enum Params { K1, K2, K3, K4, K5, K6, SetBtn };
};

// ============================================================
// Module processor
// ============================================================
class Module : public CoreProcessor {
public:
    Module() {
        for (int s = 0; s < NumSets; s++)
            for (int k = 0; k < NumKnobs; k++)
                cc[s][k] = DefaultCC[s][k];
        for (int k = 0; k < NumKnobs; k++)
            lastVal[k] = -1.f;
    }

    void set_samplerate(float) override {}

    void update() override {
        // Set button: rising edge cycles active set
        float btn = params[Info::SetBtn];
        if (btn > 0.5f && lastBtn <= 0.5f) {
            activeSet = (activeSet + 1) % NumSets;
            for (int k = 0; k < NumKnobs; k++)
                lastVal[k] = -1.f;
        }
        lastBtn = btn;

        // Send MIDI CC for any knob that changed
        for (int k = 0; k < NumKnobs; k++) {
            float v = params[k];
            if (v != lastVal[k]) {
                lastVal[k] = v;
                uint8_t midiVal = static_cast<uint8_t>(v * 127.f);
                rack::midi::Message msg;
                msg.bytes[0] = 0xB0; // CC on channel 1
                msg.bytes[1] = cc[activeSet][k] & 0x7F;
                msg.bytes[2] = midiVal & 0x7F;
                midiOut.sendMessage(msg);
            }
        }
    }

    void set_param(int id, float val) override {
        if (id >= 0 && id < 7) params[id] = val;
    }

    float get_param(int id) const override {
        if (id >= 0 && id < 7) return params[id];
        return 0.f;
    }

    void set_input(int, float) override {}
    float get_output(int) const override { return 0.f; }

    std::string save_state() override {
        return std::string(1, '0' + activeSet);
    }

    void load_state(std::string_view sv) override {
        if (!sv.empty() && sv[0] >= '0' && sv[0] < '0' + NumSets)
            activeSet = sv[0] - '0';
        for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
    }

private:
    float params[7]            = {};
    float lastVal[NumKnobs]    = {};
    float lastBtn              = 0.f;
    int   activeSet            = 0;
    uint8_t cc[NumSets][NumKnobs] = {};
    rack::midi::Output midiOut;
};

} // namespace RytmCC

// ============================================================
// Plugin entry point
// ============================================================
extern "C" void init() {
    MetaModule::register_module<RytmCC::Module, RytmCC::Info>("RytmCC");
}
