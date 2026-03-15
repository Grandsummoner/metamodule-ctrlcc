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

static constexpr MetaModule::Knob makeKnob(float x, float y,
                                            std::string_view sname,
                                            std::string_view lname) {
    MetaModule::Knob k{};
    k.x_mm          = x;
    k.y_mm          = y;
    k.short_name    = sname;
    k.long_name     = lname;
    k.min_value     = 0.f;
    k.max_value     = 1.f;
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
    static constexpr uint32_t         width_hp     = 16;
    static constexpr std::string_view png_filename = "RytmCC/RytmCC.png";
    static constexpr std::string_view svg_filename = "";

    // Panel is 81.28mm wide (16HP), 128.5mm tall
    // Row 1: K1 K2 K3 spread across top half
    // Row 2: K4 K5 K6 spread across bottom half
    // Set button centred at bottom
    static constexpr std::array<MetaModule::Element, 7> Elements {{
        makeKnob(14.f,  55.f, "K1", "Knob 1 Red"),
        makeKnob(40.f,  55.f, "K2", "Knob 2 Orange"),
        m
