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

static constexpr int NumKnobs = 6;
static constexpr int NumSets  = 4;

static constexpr uint8_t DefaultCC[NumSets][NumKnobs] = {
    {35, 36, 37, 39, 40, 41},
    {42, 43, 44, 45, 46, 47},
    {74, 75, 70, 71, 78, 80},
    {7,  10, 82, 83, 102, 109},
};

namespace RytmCC {

static constexpr MetaModule::Knob makeKnob(float px, float py,
                                            std::string_view sn,
                                            std::string_view ln) {
    MetaModule::Knob knob{};
    knob.x_mm          = px;
    knob.y_mm          = py;
    knob.short_name    = sn;
    knob.long_name     = ln;
    knob.min_value     = 0.f;
    knob.max_value     = 1.f;
    knob.default_value = 0.f;
    return knob;
}

static constexpr MetaModule::KnobSnapped makeCCKnob(float px, float py,
                                                     std::string_view sn,
                                                     std::string_view ln) {
    MetaModule::KnobSnapped knob{};
    knob.x_mm          = px;
    knob.y_mm          = py;
    knob.short_name    = sn;
    knob.long_name     = ln;
    knob.min_value     = 0.f;
    knob.max_value     = 1.f;
    knob.default_value = 0.f;
    knob.num_pos       = 128;
    return knob;
}

static constexpr MetaModule::MomentaryButton makeBtn(float px, float py,
                                                      std::string_view sn,
                                                      std::string_view ln) {
    MetaModule::MomentaryButton btn{};
    btn.x_mm       = px;
    btn.y_mm       = py;
    btn.short_name = sn;
    btn.long_name  = ln;
    return btn;
}

struct Info : MetaModule::ModuleInfoBase {
    static constexpr std::string_view slug         = "RytmCC";
    static constexpr std::string_view description  = "6-knob MIDI CC for Elektron Analog Rytm MkII";
    static constexpr uint32_t         width_hp     = 16;
    static constexpr std::string_view png_filename = "RytmCC/RytmCC.png";
    static constexpr std::string_view svg_filename = "";

    // 6 value knobs + 6 CC selector knobs + 1 set button = 13 elements
    static constexpr std::array<MetaModule::Element, 13> Elements {{
        // Row 1: value knobs
        makeKnob  (13.8f,  56.2f, "K1",    "Knob 1 Red"),
        makeKnob  (40.6f,  56.2f, "K2",    "Knob 2 Orange"),
        makeKnob  (67.5f,  56.2f, "K3",    "Knob 3 Yellow"),
        // Row 2: value knobs
        makeKnob  (13.8f,  95.3f, "K4",    "Knob 4 Green"),
        makeKnob  (40.6f,  95.3f, "K5",    "Knob 5 Blue"),
        makeKnob  (67.5f,  95.3f, "K6",    "Knob 6 Purple"),
        // Set button
        makeBtn   (40.6f, 117.2f, "Set",   "Next Set"),
        // CC selectors — small knobs, placed above each value knob
        makeCCKnob(13.8f,  40.0f, "CC1",   "CC Number K1"),
        makeCCKnob(40.6f,  40.0f, "CC2",   "CC Number K2"),
        makeCCKnob(67.5f,  40.0f, "CC3",   "CC Number K3"),
        makeCCKnob(13.8f,  79.0f, "CC4",   "CC Number K4"),
        makeCCKnob(40.6f,  79.0f, "CC5",   "CC Number K5"),
        makeCCKnob(67.5f,  79.0f, "CC6",   "CC Number K6"),
    }};

    enum Params {
        K1, K2, K3, K4, K5, K6,
        SetBtn,
        CC1, CC2, CC3, CC4, CC5, CC6,
        NumParams
    };
};

class Module : public CoreProcessor {
public:
    Module() {
        // Init CC numbers from defaults for set 0
        for (int k = 0; k < NumKnobs; k++) {
            for (int s = 0; s < NumSets; s++)
                savedCC[s][k] = DefaultCC[s][k];
            // Set CC selector params to default values (normalised 0-1)
            params[Info::CC1 + k] = DefaultCC[0][k] / 127.f;
            lastVal[k] = -1.f;
        }
    }

    void set_samplerate(float) override {}

    void update() override {
        // Set button — rising edge cycles active set
        float btnVal = params[Info::SetBtn];
        if (btnVal > 0.5f && lastBtn <= 0.5f) {
            // Save current CC selections for this set
            for (int k = 0; k < NumKnobs; k++)
                savedCC[activeSet][k] = static_cast<uint8_t>(
                    params[Info::CC1 + k] * 127.f);

            activeSet = (activeSet + 1) % NumSets;

            // Load CC selections for new set
            for (int k = 0; k < NumKnobs; k++) {
                params[Info::CC1 + k] = savedCC[activeSet][k] / 127.f;
                lastVal[k] = -1.f;
            }
        }
        lastBtn = btnVal;

        // Send MIDI CC for any changed knob
        for (int k = 0; k < NumKnobs; k++) {
            float v = params[k];
            if (v != lastVal[k]) {
                lastVal[k] = v;
                uint8_t ccNum = static_cast<uint8_t>(
                    params[Info::CC1 + k] * 127.f) & 0x7F;
                rack::midi::Message msg;
                msg.bytes[0] = 0xB0;
                msg.bytes[1] = ccNum;
                msg.bytes[2] = static_cast<uint8_t>(v * 127.f) & 0x7F;
                midiOut.sendMessage(msg);
            }
        }
    }

    void set_param(int id, float val) override {
        if (id >= 0 && id < Info::NumParams) params[id] = val;
    }

    float get_param(int id) const override {
        if (id >= 0 && id < Info::NumParams) return params[id];
        return 0.f;
    }

    void set_input(int, float) override {}
    float get_output(int) const override { return 0.f; }

    std::string save_state() override {
        // Save active set + all CC assignments
        // Format: "S,cc0,cc1,cc2,cc3,cc4,cc5,..."  (4 sets x 6 CCs)
        std::string out;
        out += static_cast<char>('0' + activeSet);
        for (int s = 0; s < NumSets; s++)
            for (int k = 0; k < NumKnobs; k++)
                out += static_cast<char>(savedCC[s][k]);
        return out;
    }

    void load_state(std::string_view sv) override {
        if (sv.empty()) return;
        if (sv[0] >= '0' && sv[0] < '0' + NumSets)
            activeSet = sv[0] - '0';
        if (sv.size() >= 1 + NumSets * NumKnobs) {
            for (int s = 0; s < NumSets; s++)
                for (int k = 0; k < NumKnobs; k++)
                    savedCC[s][k] = static_cast<uint8_t>(
                        sv[1 + s * NumKnobs + k]);
        }
        // Load CC params for active set
        for (int k = 0; k < NumKnobs; k++) {
            params[Info::CC1 + k] = savedCC[activeSet][k] / 127.f;
            lastVal[k] = -1.f;
        }
    }

private:
    float   params[Info::NumParams] = {};
    float   lastVal[NumKnobs]       = {};
    float   lastBtn                 = 0.f;
    int     activeSet               = 0;
    uint8_t savedCC[NumSets][NumKnobs] = {};
    rack::midi::Output midiOut;
};

} // namespace RytmCC

extern "C" void init() {
    MetaModule::register_module<RytmCC::Module, RytmCC::Info>("RytmCC");
}
