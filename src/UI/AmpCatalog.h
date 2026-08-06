#pragma once

#include <QColor>

#include "DSP/AmpModel.h"
#include "UI/Widgets/KnobStyle.h"

// Visual catalog of amp models. Every entry belongs to one of the four DSP
// voicings (the "maker" axis the audio engine knows about) and adds the
// cosmetics that make the faceplate look like that maker's classic heads:
// tolex texture and color, control-panel finish, logo treatment and knob
// hardware. Inspired by the classics, drawn from scratch — nothing here is
// a circuit model or an official reproduction of any real product.

namespace ampcat
{
    enum class TolexStyle { Smooth, Levant, Tweed };

    enum class PanelStyle
    {
        Blackface,        // black panel, aluminum trim strips (US clean heads)
        ChromeStrip,      // polished chrome band on a tweed box
        GoldBrushed,      // brushed gold with engraved black text (UK stacks)
        DiamondPlate,     // stamped diamond tread steel (US high-gain)
        BrushedSteel,     // fine-brushed steel
        CreamHieroglyph,  // warm cream panel in a bright picture frame
    };

    enum class LogoStyle { Script, BlockSerif, MetalPlate };

    struct AmpModelSpec
    {
        const char*        maker;
        const char*        model;
        ampsim::AmpVoicing voicing;

        TolexStyle tolexStyle;
        QColor     tolex;
        QColor     tolexDark;

        PanelStyle panelStyle;
        QColor     panel;
        QColor     panelText;   // knob labels / panel lettering
        QColor     piping;      // trim line around the panel

        LogoStyle  logoStyle;
        QColor     logoColor;

        KnobStyle  knobStyle;
        QColor     jewel;       // pilot-lamp color when powered
    };

    inline const AmpModelSpec kAmpModels[] = {
        // --- Fender-voiced: bright, scooped, soft breakup --------------------
        { "Fender", "'65 Twin Reverb", ampsim::AmpVoicing::Fender,
          TolexStyle::Smooth, QColor(0x1e, 0x1e, 0x21), QColor(0x0d, 0x0d, 0x0f),
          PanelStyle::Blackface, QColor(0x16, 0x16, 0x19), QColor(0xe9, 0xe4, 0xd4), QColor(0xd8, 0xd2, 0xc0),
          LogoStyle::Script, QColor(0xd4, 0xd4, 0xda),
          KnobStyle::FenderSkirted, QColor(0xd8, 0x30, 0x28) },

        { "Fender", "'64 Deluxe Reverb", ampsim::AmpVoicing::Fender,
          TolexStyle::Smooth, QColor(0x22, 0x22, 0x25), QColor(0x0f, 0x0f, 0x12),
          PanelStyle::Blackface, QColor(0x17, 0x17, 0x1a), QColor(0xe9, 0xe4, 0xd4), QColor(0xd8, 0xd2, 0xc0),
          LogoStyle::Script, QColor(0xd4, 0xd4, 0xda),
          KnobStyle::FenderSkirted, QColor(0xd8, 0x30, 0x28) },

        { "Fender", "'59 Bassman Tweed", ampsim::AmpVoicing::Fender,
          TolexStyle::Tweed, QColor(0xc7, 0xa2, 0x58), QColor(0x8a, 0x68, 0x2e),
          PanelStyle::ChromeStrip, QColor(0xc9, 0xc9, 0xce), QColor(0x2a, 0x22, 0x12), QColor(0x6a, 0x52, 0x24),
          LogoStyle::Script, QColor(0x3a, 0x2c, 0x14),
          KnobStyle::TweedChicken, QColor(0xe8, 0x9a, 0x2a) },

        // --- Marshall-voiced: mid-forward British crunch ---------------------
        { "Marshall", "JCM800 2203", ampsim::AmpVoicing::Marshall,
          TolexStyle::Levant, QColor(0x24, 0x24, 0x27), QColor(0x10, 0x10, 0x12),
          PanelStyle::GoldBrushed, QColor(0xc4, 0x9e, 0x3e), QColor(0x21, 0x18, 0x06), QColor(0xd9, 0xb8, 0x5a),
          LogoStyle::Script, QColor(0xf2, 0xf2, 0xf2),
          KnobStyle::MarshallGold, QColor(0xd8, 0x30, 0x28) },

        { "Marshall", "1959 Super Lead Plexi", ampsim::AmpVoicing::Marshall,
          TolexStyle::Levant, QColor(0x26, 0x26, 0x29), QColor(0x11, 0x11, 0x13),
          PanelStyle::GoldBrushed, QColor(0xcf, 0xa8, 0x46), QColor(0x24, 0x1a, 0x07), QColor(0xe2, 0xc2, 0x64),
          LogoStyle::Script, QColor(0xf2, 0xf2, 0xf2),
          KnobStyle::MarshallGold, QColor(0xd8, 0x30, 0x28) },

        { "Marshall", "JVM410H", ampsim::AmpVoicing::Marshall,
          TolexStyle::Levant, QColor(0x1f, 0x1f, 0x22), QColor(0x0e, 0x0e, 0x10),
          PanelStyle::GoldBrushed, QColor(0xb5, 0x91, 0x38), QColor(0x1e, 0x16, 0x05), QColor(0xc9, 0xa8, 0x4e),
          LogoStyle::Script, QColor(0xf2, 0xf2, 0xf2),
          KnobStyle::MarshallGold, QColor(0xd8, 0x30, 0x28) },

        // --- Mesa-voiced: tight, aggressive high gain ------------------------
        { "Mesa", "Dual Rectifier", ampsim::AmpVoicing::Mesa,
          TolexStyle::Smooth, QColor(0x1c, 0x1c, 0x1e), QColor(0x0c, 0x0c, 0x0e),
          PanelStyle::DiamondPlate, QColor(0x8e, 0x90, 0x96), QColor(0x14, 0x14, 0x16), QColor(0x54, 0x56, 0x5c),
          LogoStyle::MetalPlate, QColor(0xd9, 0xd9, 0xde),
          KnobStyle::MesaMetal, QColor(0xd8, 0x30, 0x28) },

        { "Mesa", "Mark V", ampsim::AmpVoicing::Mesa,
          TolexStyle::Smooth, QColor(0x20, 0x20, 0x22), QColor(0x0e, 0x0e, 0x10),
          PanelStyle::BrushedSteel, QColor(0x9a, 0x9c, 0xa2), QColor(0x18, 0x18, 0x1a), QColor(0x5e, 0x60, 0x66),
          LogoStyle::MetalPlate, QColor(0xd9, 0xd9, 0xde),
          KnobStyle::MesaMetal, QColor(0xd8, 0x30, 0x28) },

        // --- Orange-voiced: thick, warm, compressed --------------------------
        { "Orange", "OR50 Custom", ampsim::AmpVoicing::Orange,
          TolexStyle::Levant, QColor(0xd9, 0x6b, 0x1f), QColor(0xa2, 0x49, 0x0f),
          PanelStyle::CreamHieroglyph, QColor(0xef, 0xe7, 0xd0), QColor(0x22, 0x1c, 0x12), QColor(0xf6, 0xef, 0xdc),
          LogoStyle::BlockSerif, QColor(0x1c, 0x16, 0x0e),
          KnobStyle::OrangeChicken, QColor(0xe8, 0x9a, 0x2a) },

        { "Orange", "Rockerverb 100", ampsim::AmpVoicing::Orange,
          TolexStyle::Levant, QColor(0xd0, 0x62, 0x1a), QColor(0x99, 0x42, 0x0c),
          PanelStyle::CreamHieroglyph, QColor(0xec, 0xe3, 0xca), QColor(0x22, 0x1c, 0x12), QColor(0xf4, 0xec, 0xd6),
          LogoStyle::BlockSerif, QColor(0x1c, 0x16, 0x0e),
          KnobStyle::OrangeChicken, QColor(0xe8, 0x9a, 0x2a) },
    };

    inline constexpr int kNumAmpModels =
        static_cast<int>(sizeof(kAmpModels) / sizeof(kAmpModels[0]));
}
