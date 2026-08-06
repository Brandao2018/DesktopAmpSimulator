// Tests for the amp and pedal visual catalogs (UI/AmpCatalog.h,
// UI/PedalCatalog.h). These are pure data tables — no QWidget, QObject, or
// moc involved — so they're testable the same way as the DSP: headless,
// just linked against Qt6::Gui for QColor.

#include <catch2/catch_test_macros.hpp>

#include <cstring>
#include <set>
#include <string>

#include "UI/AmpCatalog.h"
#include "UI/PedalCatalog.h"

namespace
{
    bool isValidColor(const QColor& c)
    {
        // A default-constructed QColor is invalid(); every catalog entry
        // should have explicitly set colors.
        return c.isValid();
    }
}

//==============================================================================
// AmpCatalog
//==============================================================================

TEST_CASE("AmpCatalog is non-empty and matches its declared count", "[catalog][amp]")
{
    REQUIRE(ampcat::kNumAmpModels > 0);
    REQUIRE(static_cast<size_t>(ampcat::kNumAmpModels)
            == sizeof(ampcat::kAmpModels) / sizeof(ampcat::kAmpModels[0]));
}

TEST_CASE("Every AmpCatalog entry has a maker and model name", "[catalog][amp]")
{
    for (int i = 0; i < ampcat::kNumAmpModels; ++i)
    {
        const auto& spec = ampcat::kAmpModels[i];
        INFO("index " << i);
        REQUIRE(spec.maker != nullptr);
        REQUIRE(std::strlen(spec.maker) > 0);
        REQUIRE(spec.model != nullptr);
        REQUIRE(std::strlen(spec.model) > 0);
    }
}

TEST_CASE("AmpCatalog entries are unique maker/model pairs", "[catalog][amp]")
{
    std::set<std::string> seen;
    for (int i = 0; i < ampcat::kNumAmpModels; ++i)
    {
        const auto& spec = ampcat::kAmpModels[i];
        const std::string key = std::string(spec.maker) + " — " + spec.model;
        INFO("duplicate: " << key);
        REQUIRE(seen.insert(key).second);
    }
}

TEST_CASE("Every AmpCatalog entry has a valid voicing and valid colors", "[catalog][amp]")
{
    for (int i = 0; i < ampcat::kNumAmpModels; ++i)
    {
        const auto& spec = ampcat::kAmpModels[i];
        INFO("index " << i << " (" << spec.maker << " " << spec.model << ")");

        const int voicing = static_cast<int>(spec.voicing);
        REQUIRE(voicing >= 0);
        REQUIRE(voicing < ampsim::kNumAmpVoicings);

        REQUIRE(isValidColor(spec.tolex));
        REQUIRE(isValidColor(spec.tolexDark));
        REQUIRE(isValidColor(spec.panel));
        REQUIRE(isValidColor(spec.panelText));
        REQUIRE(isValidColor(spec.piping));
        REQUIRE(isValidColor(spec.logoColor));
        REQUIRE(isValidColor(spec.jewel));
    }
}

TEST_CASE("AmpCatalog covers all four DSP voicings", "[catalog][amp]")
{
    // The UI catalog is a superset of the DSP voicings (several faceplates
    // per voicing); every voicing must still be reachable from the menu.
    bool seen[ampsim::kNumAmpVoicings] = {};
    for (int i = 0; i < ampcat::kNumAmpModels; ++i)
        seen[static_cast<int>(ampcat::kAmpModels[i].voicing)] = true;

    for (int v = 0; v < ampsim::kNumAmpVoicings; ++v)
    {
        INFO("voicing index " << v);
        REQUIRE(seen[v]);
    }
}

TEST_CASE("AmpCatalog panel text contrasts against its panel color", "[catalog][amp]")
{
    // Not a color-science check — just guards against a copy/paste that
    // leaves label text the same color as the panel behind it (invisible).
    for (int i = 0; i < ampcat::kNumAmpModels; ++i)
    {
        const auto& spec = ampcat::kAmpModels[i];
        INFO("index " << i << " (" << spec.maker << " " << spec.model << ")");
        REQUIRE(spec.panelText != spec.panel);
    }
}

//==============================================================================
// PedalCatalog
//==============================================================================

TEST_CASE("PedalCatalog is non-empty and matches its declared count", "[catalog][pedal]")
{
    REQUIRE(pedalcat::kNumPedalStyles > 0);
    REQUIRE(static_cast<size_t>(pedalcat::kNumPedalStyles)
            == sizeof(pedalcat::kPedalStyles) / sizeof(pedalcat::kPedalStyles[0]));
}

TEST_CASE("PedalCatalog index 0 is the empty-slot style, others are real pedals", "[catalog][pedal]")
{
    REQUIRE(pedalcat::kPedalStyles[0].empty);
    for (int i = 1; i < pedalcat::kNumPedalStyles; ++i)
    {
        INFO("index " << i);
        REQUIRE_FALSE(pedalcat::kPedalStyles[i].empty);
    }
}

TEST_CASE("Every PedalCatalog entry has a name, subtitle, and valid colors", "[catalog][pedal]")
{
    for (int i = 0; i < pedalcat::kNumPedalStyles; ++i)
    {
        const auto& style = pedalcat::kPedalStyles[i];
        INFO("index " << i);
        REQUIRE(style.name != nullptr);
        REQUIRE(std::strlen(style.name) > 0);
        REQUIRE(style.subtitle != nullptr);
        REQUIRE(std::strlen(style.subtitle) > 0);
        REQUIRE(isValidColor(style.body));
        REQUIRE(isValidColor(style.text));
        REQUIRE(isValidColor(style.led));
    }
}

TEST_CASE("PedalCatalog names are unique", "[catalog][pedal]")
{
    std::set<std::string> seen;
    for (int i = 0; i < pedalcat::kNumPedalStyles; ++i)
    {
        INFO("duplicate: " << pedalcat::kPedalStyles[i].name);
        REQUIRE(seen.insert(pedalcat::kPedalStyles[i].name).second);
    }
}

TEST_CASE("styleForType returns the matching catalog entry for valid indices", "[catalog][pedal]")
{
    for (int i = 0; i < pedalcat::kNumPedalStyles; ++i)
        REQUIRE(&pedalcat::styleForType(i) == &pedalcat::kPedalStyles[i]);
}

TEST_CASE("styleForType clamps out-of-range indices to the empty-slot style", "[catalog][pedal]")
{
    REQUIRE(&pedalcat::styleForType(-1) == &pedalcat::kPedalStyles[0]);
    REQUIRE(&pedalcat::styleForType(pedalcat::kNumPedalStyles) == &pedalcat::kPedalStyles[0]);
    REQUIRE(&pedalcat::styleForType(9999) == &pedalcat::kPedalStyles[0]);
}

TEST_CASE("PedalCatalog has exactly one entry per EffectsSection effect type", "[catalog][pedal]")
{
    // EffectsSection::EffectType is None, Wah, Whammy, Phaser, Chorus,
    // Delay, kNumTypes = 6. Kept in sync manually (private enum, not
    // reachable from here without Qt Widgets) — this pins the count so a
    // future added effect fails loudly here instead of silently drawing
    // the wrong pedal skin.
    REQUIRE(pedalcat::kNumPedalStyles == 6);
}
