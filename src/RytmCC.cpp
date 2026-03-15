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

// Helper to make a Knob element cleanly
static constexpr MetaModule::Knob makeKnob(float x, float y,
                                            std::string_view sname,
                                            std::string_view lname) {
    MetaModule::Knob k{};
    k.x_mm       = x;
    k.y_mm       = y;
    k.short_name = sname;
    k.long_name  = lname;
    k.min_value  = 0.f;
    k.max_value  = 1.f;
    k.default_value = 0.f;
    return k;
}

static constexpr MetaModule::MomentaryButton makeBtn(float x, float y,
                                                      std::string_view sname,
                                                      std::string_view lname) {
    MetaModule::MomentaryButton b{};
    b.x_mm       = x;
    b.y_mm       = y;
    b.short_name = sname;
    b.long_name  = lname;
    return b;
}

struct Info : MetaModule::ModuleInfoBase {
    static constexpr std::string_view slug         = "RytmCC";
    static constexpr std::string_view description  = "6-knob MIDI CC for Elektron Analog Rytm MkII";
    static constexpr uint32_t         width_hp     = 8;
    static constexpr std::string_view png_filename = "RytmCC/RytmCC.png";
    static constexpr std::string_view svg_filename = "";

    static constexpr std::array<MetaModule::Element, 7> Elements {{
        makeKnob(13.f, 50.f, "K1", "Knob 1 Red"),
        makeKnob(30.f, 50.f, "K2", "Knob 2 Orange"),
        makeKnob(47.f, 50.f, "K3", "Knob 3 Yellow"),
        makeKnob(13.f, 76.f, "K4", "Knob 4 Green"),
        makeKnob(30.f, 76.f, "K5", "Knob 5 Blue"),
        makeKnob(47.f, 76.f, "K6", "Knob 6 Purple"),
        makeBtn (30.f, 100.f, "Set", "Next Set"),
    }};

    enum Params { K1, K2, K3, K4, K5, K6, SetBtn };
};

// ---- Processor ----
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
        float btn = params[Info::SetBtn];
        if (btn > 0.5f && lastBtn <= 0.5f) {
            activeSet = (activeSet + 1) % NumSets;
            for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
        }
        lastBtn = btn;

        for (int k = 0; k < NumKnobs; k++) {
            float v = params[k];
            if (v != lastVal[k]) {
                lastVal[k] = v;
                rack::midi::Message msg;
                msg.bytes[0] = 0xB0;
                msg.bytes[1] = cc[activeSet][k] & 0x7F;
                msg.bytes[2] = static_cast<uint8_t>(v * 127.f) & 0x7F;
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
    float   params[7]              = {};
    float   lastVal[NumKnobs]      = {};
    float   lastBtn                = 0.f;
    int     activeSet              = 0;
    uint8_t cc[NumSets][NumKnobs]  = {};
    rack::midi::Output midiOut;
};

} // namespace RytmCC

extern "C" void init() {
    MetaModule::register_module<RytmCC::Module, RytmCC::Info>("RytmCC");
}
