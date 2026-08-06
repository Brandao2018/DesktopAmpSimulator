#pragma once

#include <QColor>

// Visual catalog of pedalboard stompbox skins, one entry per effect type.
// Index order matches EffectsSection::EffectType (None, Wah, Whammy, Phaser,
// Chorus, Delay) — index 0 is the "empty slot" look, not a real pedal.
// Pure data, no QWidget/QObject dependency, so it is unit-testable exactly
// like AmpCatalog.h without pulling Qt Widgets or moc into the test binary.

namespace pedalcat
{
    struct PedalStyle
    {
        const char* name;       // "MEMORY ECHO"
        const char* subtitle;   // "ANALOG DELAY"
        QColor      body;
        QColor      text;
        QColor      led;
        bool        empty;      // draw as an empty pedalboard slot, no footswitch
    };

    inline const PedalStyle kPedalStyles[] = {
        // None — empty pedalboard slot.
        { "EMPTY", "SELECT AN EFFECT",
          QColor(0x1c, 0x1c, 0x1e), QColor(0x62, 0x62, 0x66), QColor(0x40, 0x40, 0x44), true },
        // Wah — black classic wah brick.
        { "WEEPER WAH", "AUTO WAH",
          QColor(0x26, 0x26, 0x2a), QColor(0xd8, 0xd8, 0xdc), QColor(0xff, 0x38, 0x30), false },
        // Whammy — red pitch shifter.
        { "SKY SHIFTER", "PITCH BENDER",
          QColor(0xb0, 0x24, 0x20), QColor(0xf6, 0xe9, 0xd8), QColor(0xff, 0xd0, 0x40), false },
        // Phaser — script-era orange.
        { "ORBIT 90", "SCRIPT PHASER",
          QColor(0xd9, 0x6b, 0x1f), QColor(0x1e, 0x14, 0x08), QColor(0xff, 0x38, 0x30), false },
        // Chorus — analog blue.
        { "BLUE SWIRL", "ANALOG CHORUS",
          QColor(0x2c, 0x64, 0xa8), QColor(0xf0, 0xf4, 0xfa), QColor(0xff, 0x38, 0x30), false },
        // Delay — cream memory echo.
        { "MEMORY ECHO", "ANALOG DELAY",
          QColor(0xdd, 0xd4, 0xb6), QColor(0x2c, 0x24, 0x14), QColor(0xff, 0x38, 0x30), false },
    };

    inline constexpr int kNumPedalStyles =
        static_cast<int>(sizeof(kPedalStyles) / sizeof(kPedalStyles[0]));

    // Out-of-range indices clamp to index 0 (the empty-slot look) instead of
    // reading past the table.
    inline const PedalStyle& styleForType(int type)
    {
        return kPedalStyles[type >= 0 && type < kNumPedalStyles ? type : 0];
    }
}
