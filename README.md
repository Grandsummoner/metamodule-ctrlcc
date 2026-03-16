# CtrlCC

A MIDI CC controller plugin for the [4ms MetaModule](https://4mscompany.com/metamodule). Turn MetaModule's knobs into a hands-on controller for any USB MIDI device.

📖 **[User Guide](https://grandsummoner.github.io/metamodule-ctrlcc)**

---

## What it does

CtrlCC gives you **6 knobs × 4 switchable sets = 24 MIDI CC assignments** on a single MetaModule plugin. Each knob can send a different CC number on a different MIDI channel, so you can control multiple devices or parameters at once.

The companion **[Patch Builder](https://grandsummoner.github.io/metamodule-ctrlcc)** is a standalone HTML tool that lets you design your assignments visually — pick parameters by name from a built-in database of 17 devices — then export a ready-to-load `.yml` patch file.

## Supported devices

Elektron Analog Rytm MKII · Analog Four MKII · Digitakt II · Digitone II · Tonverk · Korg minilogue xd · Audiothingies Micromonsta 2 · Make Noise 0-Coast · Novation Circuit Rhythm · Roland TR-1000 · Sonicware ELZ_1 play V2 · Dirtywave M8 · Synthstrom Deluge · OXI Coral · Knobula Pianophonic · Modbap Trinity · Supercritical Redshift 6

Any class-compliant USB MIDI device works even if not listed — just assign CC numbers manually.

## Installation

1. Download `CtrlCC.mmplugin` from the [latest release](https://github.com/Grandsummoner/metamodule-ctrlcc/releases)
2. Copy it to the `plugins/` folder on your MetaModule via WiFi file browser or USB
3. Download `ctrlcc-patch-builder.html` and open it in any browser — no internet required
4. Build your patch, export `.yml`, upload to MetaModule's `patches/` folder and load

## Plugin overview

| | |
|---|---|
| **Knobs** | 6 per set |
| **Sets** | 4 (switch via module menu → Next Set) |
| **MIDI output** | Standard 7-bit CC (0–127) |
| **MIDI channels** | Per-knob, 1–16 |
| **USB MIDI** | Class-compliant devices via MetaModule USB-C |
| **Firmware** | MetaModule v2.1+ |

## Building from source

Requires the [MetaModule Plugin SDK](https://github.com/4ms/metamodule-plugin-sdk). A GitHub Actions workflow is included that cross-compiles for ARM on push.

```bash
git clone https://github.com/Grandsummoner/metamodule-ctrlcc
cd metamodule-ctrlcc
# Follow SDK setup, then:
make
```

## License

MIT
