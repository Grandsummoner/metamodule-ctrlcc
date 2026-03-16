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

static constexpr uint8_t DefaultCh[NumKnobs] = {0, 0, 0, 0, 0, 0};

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

static constexpr MetaModule::AltParamAction makeAlt(std::string_view sn,
                                                     std::string_view ln) {
    MetaModule::AltParamAction act{};
    act.short_name = sn;
    act.long_name  = ln;
    return act;
}

static constexpr MetaModule::DynamicTextDisplay makeDisplay(
    float px, float py, float w, float h, std::string_view sn) {
    MetaModule::DynamicTextDisplay disp{};
    disp.x_mm       = px;
    disp.y_mm       = py;
    disp.width_mm   = w;
    disp.height_mm  = h;
    disp.short_name = sn;
    disp.long_name  = sn;
    disp.wrap_mode  = MetaModule::TextDisplay::WrapMode::Clip;
    return disp;
}

static constexpr MetaModule::MonoLight makeLight(float px, float py,
                                                  uint16_t col,
                                                  std::string_view sn) {
    MetaModule::MonoLight light{};
    light.x_mm       = px;
    light.y_mm       = py;
    light.short_name = sn;
    light.long_name  = sn;
    light.color      = MetaModule::RGB565{col};
    return light;
}

struct Info : MetaModule::ModuleInfoBase {
    static constexpr std::string_view slug         = "RytmCC";
    static constexpr std::string_view description  = "6-knob MIDI CC for Elektron Analog Rytm MkII";
    static constexpr uint32_t         width_hp     = 16;
    static constexpr std::string_view png_filename = "RytmCC/RytmCC.png";
    static constexpr std::string_view svg_filename = "";

    static constexpr float KnobSize = 15.17f;

    // SET dots in PNG at px(38,50) px(52,50) px(66,50) px(80,50)
    // mm: x=20.59, 28.18, 35.77, 43.35  y=26.77
    static constexpr std::array<MetaModule::Element, 13> Elements {{
        makeKnob   (13.55f,  56.22f, KnobSize, "K1", "Knob 1 Red"),
        makeKnob   (40.64f,  56.22f, KnobSize, "K2", "Knob 2 Orange"),
        makeKnob   (67.74f,  56.22f, KnobSize, "K3", "Knob 3 Yellow"),
        makeKnob   (13.55f,  93.70f, KnobSize, "K4", "Knob 4 Green"),
        makeKnob   (40.64f,  93.70f, KnobSize, "K5", "Knob 5 Blue"),
        makeKnob   (67.74f,  93.70f, KnobSize, "K6", "Knob 6 Purple"),
        makeDisplay( 3.25f,  18.00f, 74.78f,   9.64f, "CCDisp"),
        makeDisplay( 3.25f, 115.00f, 74.78f,   7.50f, "SetDisp"),
        makeLight  (20.59f,  26.77f, 0x1BDA, "S1"),  // Blue
        makeLight  (28.18f,  26.77f, 0x2D0A, "S2"),  // Green
        makeLight  (35.77f,  26.77f, 0xE3C4, "S3"),  // Orange
        makeLight  (43.35f,  26.77f, 0x8218, "S4"),  // Violet
        makeAlt    ("NextSet", "Next Set"),
    }};

    enum Params  { K1, K2, K3, K4, K5, K6, NumParams };
    enum Lights  { CCDisplay, SetDisplay, SetLight1, SetLight2, SetLight3, SetLight4 };
};

class Module : public CoreProcessor {
public:
    Module() {
        for (int s = 0; s < NumSets; s++) {
            for (int k = 0; k < NumKnobs; k++) {
                savedCC[s][k] = DefaultCC[s][k];
                savedCh[s][k] = DefaultCh[k];
            }
            snprintf(setNames[s], sizeof(setNames[s]),
                "%s", DefaultSetNames[s]);
        }
        for (int k = 0; k < NumKnobs; k++)
            lastVal[k] = -1.f;
        refreshDisplay(255, 0, 0);
    }

    void set_samplerate(float) override {}

    void refreshDisplay(uint8_t ccNum, uint8_t midiVal, uint8_t ch) {
        if (ccNum == 255) {
            snprintf(ccBuf, sizeof(ccBuf),
                "Ch%-2d  Ready         ", ch + 1);
        } else {
            int filled = (midiVal * 8) / 127;
            char bar[9];
            for (int i = 0; i < 8; i++)
                bar[i] = (i < filled) ? '|' : '.';
            bar[8] = 0;
            snprintf(ccBuf, sizeof(ccBuf),
                "Ch%-2d CC%03d %s      ",
                ch + 1, ccNum, bar);
        }
        snprintf(setNameBuf, sizeof(setNameBuf),
            "%-18s", setNames[activeSet]);
    }

    void update() override {
        if (displayTimer > 0) {
            displayTimer--;
            if (displayTimer == 0)
                refreshDisplay(255, 0, savedCh[activeSet][0]);
        }

        for (int k = 0; k < NumKnobs; k++) {
            float v = params[k];
            if (v != lastVal[k]) {
                lastVal[k] = v;
                uint8_t ccNum   = savedCC[activeSet][k];
                uint8_t ch      = savedCh[activeSet][k];
                uint8_t midiVal = static_cast<uint8_t>(v * 127.f) & 0x7F;

                rack::midi::Message msg;
                msg.bytes[0] = static_cast<uint8_t>(
                    0xB0 | (ch & 0x0F));
                msg.bytes[1] = ccNum & 0x7F;
                msg.bytes[2] = midiVal;
                midiOut.sendMessage(msg);

                refreshDisplay(ccNum, midiVal, ch);
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
                refreshDisplay(255, 0, savedCh[activeSet][0]);
                displayTimer = 48000 * 2;
            }
        }
    }

    float get_param(int id) const override {
        if (id >= 0 && id < Info::NumParams) return params[id];
        return 0.f;
    }

    float get_led_brightness(int led_id) const override {
        if (led_id == Info::SetLight1) return activeSet == 0 ? 1.f : 0.f;
        if (led_id == Info::SetLight2) return activeSet == 1 ? 1.f : 0.f;
        if (led_id == Info::SetLight3) return activeSet == 2 ? 1.f : 0.f;
        if (led_id == Info::SetLight4) return activeSet == 3 ? 1.f : 0.f;
        return 0.f;
    }

    size_t get_display_text(int display_id,
                             std::span<char> buf) override {
        int len = 0;
        if (display_id == Info::CCDisplay)
            len = snprintf(buf.data(), buf.size(), "%s", ccBuf);
        else if (display_id == Info::SetDisplay)
            len = snprintf(buf.data(), buf.size(), "%s", setNameBuf);
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
        for (int s = 0; s < NumSets; s++)
            for (int k = 0; k < NumKnobs; k++)
                out += static_cast<char>(savedCh[s][k]);
        for (int s = 0; s < NumSets; s++) {
            char buf[9] = {};
            snprintf(buf, sizeof(buf), "%.8s", setNames[s]);
            for (int i = 0; i < 8; i++) out += buf[i];
        }
        return out;
    }

    void load_state(std::string_view sv) override {
        if (sv.empty()) return;
        size_t pos = 0;

        if (pos < sv.size() &&
            sv[pos] >= '0' && sv[pos] < '0' + NumSets)
            activeSet = sv[pos] - '0';
        pos++;

        for (int s = 0; s < NumSets && pos < sv.size(); s++)
            for (int k = 0; k < NumKnobs && pos < sv.size(); k++, pos++)
                savedCC[s][k] = static_cast<uint8_t>(sv[pos]);

        for (int s = 0; s < NumSets && pos < sv.size(); s++)
            for (int k = 0; k < NumKnobs && pos < sv.size(); k++, pos++)
                savedCh[s][k] = static_cast<uint8_t>(sv[pos]) & 0x0F;

        for (int s = 0; s < NumSets && pos + 8 <= sv.size();
             s++, pos += 8) {
            char buf[9] = {};
            for (int i = 0; i < 8; i++) buf[i] = sv[pos + i];
            buf[8] = 0;
            bool hasContent = false;
            for (int i = 0; i < 8; i++)
                if (buf[i] > 0x20) { hasContent = true; break; }
            if (hasContent)
                snprintf(setNames[s], sizeof(setNames[s]), "%s", buf);
        }

        for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
        refreshDisplay(255, 0, savedCh[activeSet][0]);
        displayTimer = 48000 * 2;
    }

private:
    float   params[Info::NumParams]      = {};
    float   lastVal[NumKnobs]            = {};
    int     activeSet                    = 0;
    int     displayTimer                 = 0;
    uint8_t savedCC[NumSets][NumKnobs]   = {};
    uint8_t savedCh[NumSets][NumKnobs]   = {};
    char    setNames[NumSets][16]        = {};
    char    ccBuf[32]                    = {};
    char    setNameBuf[32]               = {};
    rack::midi::Output midiOut;
};

} // namespace RytmCC

extern "C" void init() {
    MetaModule::register_module<RytmCC::Module, RytmCC::Info>("RytmCC");
}
