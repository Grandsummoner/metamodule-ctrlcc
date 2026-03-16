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
#include <cstdlib>

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
    float px, float py, float w, float h, std::string_view sn,
    RGB565 col = Colors565::White) {
    MetaModule::DynamicTextDisplay disp{};
    disp.x_mm       = px;
    disp.y_mm       = py;
    disp.width_mm   = w;
    disp.height_mm  = h;
    disp.short_name = sn;
    disp.long_name  = sn;
    disp.wrap_mode  = MetaModule::TextDisplay::WrapMode::Clip;
    disp.color      = col;
    return disp;
}

static constexpr MetaModule::MonoLight makeLight(float px, float py,
                                                  RGB565 col,
                                                  std::string_view sn,
                                                  std::string_view img) {
    MetaModule::MonoLight light{};
    light.x_mm       = px;
    light.y_mm       = py;
    light.short_name = sn;
    light.long_name  = sn;
    light.color      = col;
    light.image      = img;
    return light;
}

struct Info : MetaModule::ModuleInfoBase {
    static constexpr std::string_view slug         = "RytmCC";
    static constexpr std::string_view description  = "6-knob MIDI CC for Elektron Analog Rytm MkII";
    static constexpr uint32_t         width_hp     = 16;
    static constexpr std::string_view png_filename = "RytmCC/RytmCC.png";
    static constexpr std::string_view svg_filename = "";

    static constexpr float KnobSize = 15.17f;

    static constexpr std::array<MetaModule::Element, 13> Elements {{
        makeKnob   (13.55f,  56.22f, KnobSize, "K1", "Knob 1 Red"),
        makeKnob   (40.64f,  56.22f, KnobSize, "K2", "Knob 2 Orange"),
        makeKnob   (67.74f,  56.22f, KnobSize, "K3", "Knob 3 Yellow"),
        makeKnob   (13.55f,  93.70f, KnobSize, "K4", "Knob 4 Green"),
        makeKnob   (40.64f,  93.70f, KnobSize, "K5", "Knob 5 Blue"),
        makeKnob   (67.74f,  93.70f, KnobSize, "K6", "Knob 6 Purple"),
        makeDisplay(39.00f,  18.00f, 35.00f,   9.64f, "CCDisp",
                    Colors565::White),
        makeDisplay(30.00f, 115.00f, 44.00f,   7.50f, "SetDisp",
                    Colors565::Cyan),
        makeLight  (20.59f,  26.77f, Colors565::Blue,   "S1", "RytmCC/led_ring.png"),
        makeLight  (28.18f,  26.77f, Colors565::Green,  "S2", "RytmCC/led_ring.png"),
        makeLight  (35.77f,  26.77f, Colors565::Orange, "S3", "RytmCC/led_ring.png"),
        makeLight  (43.35f,  26.77f, Colors565::Purple, "S4", "RytmCC/led_ring.png"),
        makeAlt    ("NextSet", "Next Set"),
    }};

    enum Params { K1, K2, K3, K4, K5, K6, NumParams };
    enum Lights {
        CCDisplay  = 0,
        SetDisplay = 1,
        SetLight1  = 2,
        SetLight2  = 3,
        SetLight3  = 4,
        SetLight4  = 5
    };
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
        midiOut.setChannel(0);
        refreshDisplay(255, 0, 0);
    }

    void set_samplerate(float) override {}

    void refreshDisplay(uint8_t ccNum, uint8_t midiVal, uint8_t ch) {
        if (ccNum == 255) {
            snprintf(ccBuf, sizeof(ccBuf), "Ready!        ");
        } else {
            int filled = (int)((midiVal * 6.0f) / 127.0f + 0.5f);
            if (filled > 6) filled = 6;
            char bar[7];
            for (int i = 0; i < 6; i++)
                bar[i] = (i < filled) ? '|' : '.';
            bar[6] = 0;
            snprintf(ccBuf, sizeof(ccBuf),
                "C%02d %03d %s",
                ch + 1, ccNum, bar);
        }
        snprintf(setNameBuf, sizeof(setNameBuf),
            "%-14s", setNames[activeSet]);
    }

    void update() override {
        if (displayTimer > 0) {
            displayTimer--;
            if (displayTimer == 0)
                refreshDisplay(255, 0, 0);
        }

        for (int k = 0; k < NumKnobs; k++) {
            float v = params[k];
            if (v != lastVal[k]) {
                lastVal[k] = v;
                uint8_t ccNum   = savedCC[activeSet][k];
                uint8_t ch      = savedCh[activeSet][k];
                uint8_t midiVal = static_cast<uint8_t>(v * 127.f) & 0x7F;

                rack::midi::Message msg;
                msg.bytes[0] = 0xB0; // CC status, channel set via setChannel
                msg.bytes[1] = ccNum & 0x7F;
                msg.bytes[2] = midiVal;
                midiOut.setChannel(ch); // set correct channel before each send
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
                refreshDisplay(255, 0, 0);
                displayTimer = 48000 * 2;
            }
        }
    }

    float get_param(int id) const override {
        if (id >= 0 && id < Info::NumParams) return params[id];
        return 0.f;
    }

    float get_led_brightness(int led_id) const override {
        if (led_id == Info::SetLight1) return activeSet == 0 ? 1.0f : 0.0f;
        if (led_id == Info::SetLight2) return activeSet == 1 ? 1.0f : 0.0f;
        if (led_id == Info::SetLight3) return activeSet == 2 ? 1.0f : 0.0f;
        if (led_id == Info::SetLight4) return activeSet == 3 ? 1.0f : 0.0f;
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
        char buf[512] = {};
        int pos = 0;

        pos += snprintf(buf + pos, sizeof(buf) - pos, "%d", activeSet);

        for (int s = 0; s < NumSets; s++) {
            buf[pos++] = '|';
            for (int k = 0; k < NumKnobs; k++) {
                pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "%s%d", k > 0 ? "," : "", savedCC[s][k]);
            }
        }

        for (int s = 0; s < NumSets; s++) {
            buf[pos++] = '|';
            for (int k = 0; k < NumKnobs; k++) {
                pos += snprintf(buf + pos, sizeof(buf) - pos,
                    "%s%d", k > 0 ? "," : "", savedCh[s][k]);
            }
        }

        for (int s = 0; s < NumSets; s++) {
            buf[pos++] = '|';
            for (int i = 0; i < 8 && setNames[s][i]; i++) {
                char c = setNames[s][i];
                if (c != '|' && c != ',') buf[pos++] = c;
            }
        }

        return std::string(buf, pos);
    }

    void load_state(std::string_view sv) override {
        if (sv.empty()) return;

        char fields[13][64] = {};
        int fi = 0, ci = 0;
        for (size_t i = 0; i <= sv.size() && fi < 13; i++) {
            char c = (i < sv.size()) ? sv[i] : '|';
            if (c == '|') {
                fields[fi][ci] = 0;
                fi++;
                ci = 0;
            } else if (ci < 63) {
                fields[fi][ci++] = c;
            }
        }
        if (fi < 9) return;

        int as = fields[0][0] - '0';
        if (as >= 0 && as < NumSets) activeSet = as;

        for (int s = 0; s < NumSets && (s + 1) < fi; s++) {
            const char* p = fields[s + 1];
            for (int k = 0; k < NumKnobs; k++) {
                savedCC[s][k] = (uint8_t)(atoi(p) & 0x7F);
                p = strchr(p, ',');
                if (!p) break;
                p++;
            }
        }

        for (int s = 0; s < NumSets && (s + 5) < fi; s++) {
            const char* p = fields[s + 5];
            for (int k = 0; k < NumKnobs; k++) {
                savedCh[s][k] = (uint8_t)(atoi(p) & 0x0F);
                p = strchr(p, ',');
                if (!p) break;
                p++;
            }
        }

        for (int s = 0; s < NumSets && (s + 9) < fi; s++) {
            if (fields[s + 9][0]) {
                snprintf(setNames[s], sizeof(setNames[s]),
                    "%s", fields[s + 9]);
            }
        }

        for (int k = 0; k < NumKnobs; k++) lastVal[k] = -1.f;
        midiOut.setChannel(0);
        refreshDisplay(255, 0, 0);
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
