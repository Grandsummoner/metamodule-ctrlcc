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

static const char* DefaultSetNames[NumSets] = {
    "Set 1", "Set 2", "Set 3", "Set 4"
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

    static constexpr float KnobSize = 13.00f;

    static constexpr std::array<MetaModule::Element, 8> Elements {{
        makeKnob   (13.55f, 56.22f, KnobSize, "K1", "Knob 1 Red"),
        makeKnob   (40.64f, 56.22f, KnobSize, "K2", "Knob 2 Orange"),
        makeKnob   (67.74f, 56.22f, KnobSize, "K3", "Knob 3 Yellow"),
        makeKnob   (13.55f, 93.70f, KnobSize, "K4", "Knob 4 Green"),
        makeKnob   (40.64f, 93.70f, KnobSize, "K5", "Knob 5 Blue"),
        makeKnob   (67.74f, 93.70f, KnobSize, "K6", "Knob 6 Purple"),
        makeDisplay( 3.25f, 11.78f, 74.78f, 15.00f, "Display"),
        makeAltAction("Set", "Next Set"),
    }};

    enum Params  { K1, K2, K3, K4, K5, K6, NumParams };
    enum Lights  { MainDisplay };
};

class Module : public CoreProcessor {
public:
    Module() {
        for (int s = 0; s < NumSets; s++) {
            for (int k = 0; k < NumKnobs; k++)
                savedCC[s][k] = DefaultCC[s][k];
            snprintf(setNames[s], sizeof(setNames[s]), "%s", DefaultSetNames[s]);
        }
        for (int k = 0; k < NumKnobs; k++)
            lastVal[k] = -1.f;
        updateDisplay(0, 0, 0);
    }

    void set_samplerate(float) override {}

    // Build the single-row display:
    // "Perf 1-6  Ch1 CC007 ||||||||  "
    // Line1: set name
    // Line2: Ch + CC + value bar
    void updateDisplay(int knobIdx, uint8_t ccNum, uint8_t midiVal) {
        snprintf(line1, sizeof(line1), "%s", setNames[activeSet]);

        // Value bar: 8 chars, filled proportionally
        int filled = (midiVal * 8) / 127;
        char bar[9];
        for (int i = 0; i < 8; i++)
            bar[i] = (i < filled) ? '|' : '.';
        bar[8] = 0;

        snprintf(line2, sizeof(line2), "Ch%d CC%03d %s",
            midiCh + 1, ccNum, bar);
    }

    void update() override {
        if (displayTimer > 0) displayTimer--;
        else {
            // Idle display — show set name and channel
            snprintf(line1, sizeof(line1), "%s", setNames[activeSet]);
            snprintf(line2, sizeof(line2), "Ch%d  Ready", midiCh + 1);
        }

        for (int k = 0; k < NumKnobs; k++) {
            float v = params[k];
            if (v != lastVal[k]) {
                lastVal[k] = v;
                uint8_t ccNum   = savedCC[activeSet][k];
                uint8_t midiVal = static_cast<uint8_t>(v * 127.f) & 0x7F;

                rack::midi::Message msg;
                msg.bytes[0] = static_cast<uint8_t>(0xB0 | (midiCh & 0x0F));
                msg.bytes[1] = ccNum & 0x7F;
                msg.bytes[2] = midiVal;
                midiOut.sendMessage(msg);

                updateDisplay(k, ccNum, midiVal);
                displayTimer = 48000 * 2;
            }
        }
    }

    void set_param(int id, float val) override {
        if (id >= 0 && id < Info::NumParams) {
            params[id] = val;
        } else if (id == Info::NumParams) {
            // AltParamAction — Next Set
            if (val > 0.5f) {
                activeSet = (activeSet + 1) % NumSets;
                for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
                snprintf(line1, sizeof(line1), "%s", setNames[activeSet]);
                snprintf(line2, sizeof(line2), "Ch%d  Ready", midiCh + 1);
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

    // Save format:
    // Byte 0: active set ('0'-'3')
    // Byte 1: midi channel (0-15)
    // Bytes 2..25: CC numbers (4 sets x 6 knobs)
    // Bytes 26..57: set names (4 x 8 chars, null padded)
    std::string save_state() override {
        std::string out;
        out += static_cast<char>('0' + activeSet);
        out += static_cast<char>(midiCh);
        for (int s = 0; s < NumSets; s++)
            for (int k = 0; k < NumKnobs; k++)
                out += static_cast<char>(savedCC[s][k]);
        for (int s = 0; s < NumSets; s++) {
            char buf[9] = {};
            snprintf(buf, sizeof(buf), "%.8s", setNames[s]);
            for (int i = 0; i < 8; i++)
                out += buf[i];
        }
        return out;
    }

    void load_state(std::string_view sv) override {
        if (sv.empty()) return;
        size_t pos = 0;

        if (pos < sv.size() && sv[pos] >= '0' && sv[pos] < '0' + NumSets)
            activeSet = sv[pos] - '0';
        pos++;

        if (pos < sv.size())
            midiCh = static_cast<uint8_t>(sv[pos]) & 0x0F;
        pos++;

        // CC numbers
        for (int s = 0; s < NumSets && pos < sv.size(); s++)
            for (int k = 0; k < NumKnobs && pos < sv.size(); k++, pos++)
                savedCC[s][k] = static_cast<uint8_t>(sv[pos]);

        // Set names
        for (int s = 0; s < NumSets && pos + 8 <= sv.size(); s++, pos += 8) {
            char buf[9] = {};
            for (int i = 0; i < 8; i++)
                buf[i] = sv[pos + i];
            buf[8] = 0;
            // Strip null padding
            snprintf(setNames[s], sizeof(setNames[s]), "%s", buf);
        }

        for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
        snprintf(line1, sizeof(line1), "%s", setNames[activeSet]);
        snprintf(line2, sizeof(line2), "Ch%d  Loaded", midiCh + 1);
        displayTimer = 48000 * 2;
    }

private:
    float   params[Info::NumParams]    = {};
    float   lastVal[NumKnobs]          = {};
    float   lastBtn                    = 0.f;
    int     activeSet                  = 0;
    int     displayTimer               = 0;
    uint8_t midiCh                     = 0;
    uint8_t savedCC[NumSets][NumKnobs] = {};
    char    setNames[NumSets][16]      = {};
    char    line1[32]                  = {};
    char    line2[32]                  = {};
    rack::midi::Output midiOut;
};

} // namespace RytmCC

extern "C" void init() {
    MetaModule::register_module<RytmCC::Module, RytmCC::Info>("RytmCC");
}
