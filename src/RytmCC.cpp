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
#include <cstring>
#include <cstdio>

static constexpr int NumKnobs = 6;
static constexpr int NumSets  = 4;

static constexpr uint8_t DefaultCC[NumSets][NumKnobs] = {
    {7,  36, 37, 39, 40, 41},
    {42, 43, 44, 45, 46, 47},
    {74, 75, 70, 71, 78, 80},
    {7,  10, 82, 83, 102, 109},
};

static const char* KnobNames[NumKnobs] = {
    "K1 Red", "K2 Org", "K3 Yel",
    "K4 Grn", "K5 Blu", "K6 Pur"
};

namespace RytmCC {

static constexpr MetaModule::Knob makeKnob(float px, float py,
                                            float sz,
                                            std::string_view sn,
                                            std::string_view ln) {
    MetaModule::Knob knob{};
    knob.x_mm          = px;
    knob.y_mm          = py;
    knob.width_mm      = sz;
    knob.height_mm     = sz;
    knob.short_name    = sn;
    knob.long_name     = ln;
    knob.min_value     = 0.f;
    knob.max_value     = 1.f;
    knob.default_value = 0.f;
    knob.min_angle     = -135.f;
    knob.max_angle     = 135.f;
    return knob;
}

static constexpr MetaModule::AltParamAction makeAltAction(std::string_view sn,
                                                            std::string_view ln) {
    MetaModule::AltParamAction act{};
    act.short_name = sn;
    act.long_name  = ln;
    return act;
}

static constexpr MetaModule::DynamicTextDisplay makeDisplay(float px, float py,
                                                             float w, float h,
                                                             std::string_view sn) {
    MetaModule::DynamicTextDisplay disp{};
    disp.x_mm       = px;
    disp.y_mm       = py;
    disp.width_mm   = w;
    disp.height_mm  = h;
    disp.short_name = sn;
    disp.long_name  = sn;
    disp.wrap_mode  = MetaModule::TextDisplay::WrapMode::Wrap;
    return disp;
}

struct Info : MetaModule::ModuleInfoBase {
    static constexpr std::string_view slug         = "RytmCC";
    static constexpr std::string_view description  = "6-knob MIDI CC for Elektron Analog Rytm MkII";
    static constexpr uint32_t         width_hp     = 16;
    static constexpr std::string_view png_filename = "RytmCC/RytmCC.png";
    static constexpr std::string_view svg_filename = "";

    static constexpr float KnobSize = 21.13f;

    // Knobs: -2mm X, +2mm Y from last position (13.28, 54.34) and (93.43)
    // Display: moved right and down to sit inside dark box
    static constexpr std::array<MetaModule::Element, 8> Elements {{
        makeKnob   (11.28f, 56.34f, KnobSize, "K1", "Knob 1 Red"),
        makeKnob   (38.37f, 56.34f, KnobSize, "K2", "Knob 2 Orange"),
        makeKnob   (65.46f, 56.34f, KnobSize, "K3", "Knob 3 Yellow"),
        makeKnob   (11.28f, 95.43f, KnobSize, "K4", "Knob 4 Green"),
        makeKnob   (38.37f, 95.43f, KnobSize, "K5", "Knob 5 Blue"),
        makeKnob   (65.46f, 95.43f, KnobSize, "K6", "Knob 6 Purple"),
        makeDisplay(20.00f, 22.00f, 40.00f,  9.00f, "Display"),
        makeAltAction("Set", "Next Set"),
    }};

    enum Params  { K1, K2, K3, K4, K5, K6, NumParams };
    enum Lights  { MainDisplay };
};

class Module : public CoreProcessor {
public:
    Module() {
        for (int s = 0; s < NumSets; s++)
            for (int k = 0; k < NumKnobs; k++)
                savedCC[s][k] = DefaultCC[s][k];
        for (int k = 0; k < NumKnobs; k++)
            lastVal[k] = -1.f;
        snprintf(line1, sizeof(line1), "RYTM CC Set 1");
        snprintf(line2, sizeof(line2), "Turn a knob");
    }

    void set_samplerate(float) override {}

    void update() override {
        if (displayTimer > 0) displayTimer--;

        for (int k = 0; k < NumKnobs; k++) {
            float v = params[k];
            if (v != lastVal[k]) {
                lastVal[k] = v;
                uint8_t ccNum   = savedCC[activeSet][k];
                uint8_t midiVal = static_cast<uint8_t>(v * 127.f) & 0x7F;

                rack::midi::Message msg;
                msg.bytes[0] = 0xB0;
                msg.bytes[1] = ccNum & 0x7F;
                msg.bytes[2] = midiVal;
                midiOut.sendMessage(msg);

                snprintf(line1, sizeof(line1), "%s CC%d",
                    KnobNames[k], ccNum);
                snprintf(line2, sizeof(line2), "Val:%d Set:%d",
                    midiVal, activeSet + 1);
                displayTimer = 48000 * 2;
            }
        }
    }

    void set_param(int id, float val) override {
        if (id >= 0 && id < Info::NumParams) {
            params[id] = val;
        } else if (id == Info::NumParams) {
            if (val > 0.5f) {
                activeSet = (activeSet + 1) % NumSets;
                for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
                snprintf(line1, sizeof(line1), "Set %d", activeSet + 1);
                snprintf(line2, sizeof(line2), "Turn a knob");
                displayTimer = 48000 * 2;
            }
        }
    }

    float get_param(int id) const override {
        if (id >= 0 && id < Info::NumParams) return params[id];
        return 0.f;
    }

    size_t get_display_text(int display_id, std::span<char> buf) override {
        if (display_id != Info::MainDisplay) return 0;
        int len = snprintf(buf.data(), buf.size(), "%s\n%s", line1, line2);
        return len > 0 ? static_cast<size_t>(len) : 0;
    }

    void set_input(int, float) override {}
    float get_output(int) const override { return 0.f; }

    std::string save_state() override {
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
        if ((int)sv.size() >= 1 + NumSets * NumKnobs) {
            for (int s = 0; s < NumSets; s++)
                for (int k = 0; k < NumKnobs; k++)
                    savedCC[s][k] = static_cast<uint8_t>(
                        sv[1 + s * NumKnobs + k]);
        }
        for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
        snprintf(line1, sizeof(line1), "Set %d loaded", activeSet + 1);
        snprintf(line2, sizeof(line2), "Turn a knob");
    }

private:
    float   params[Info::NumParams]    = {};
    float   lastVal[NumKnobs]          = {};
    float   lastBtn                    = 0.f;
    int     activeSet                  = 0;
    int     displayTimer               = 0;
    uint8_t savedCC[NumSets][NumKnobs] = {};
    char    line1[32]                  = {};
    char    line2[32]                  = {};
    rack::midi::Output midiOut;
};

} // namespace RytmCC

extern "C" void init() {
    MetaModule::register_module<RytmCC::Module, RytmCC::Info>("RytmCC");
}
