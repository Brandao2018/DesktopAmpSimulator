#pragma once

#include <QGroupBox>
#include <QVariantMap>

class QComboBox;
class QPushButton;
class QStackedWidget;

class AudioEngine;
class CustomKnob;

// EFFECTS section: four effect slots. Each slot picks one of the real DSP
// effects (Wah, Whammy, Phaser, Chorus, Delay) from a type dropdown, has
// its own ACTIVE toggle, and shows that effect's parameter knobs.
//
// The DSP processing order is fixed (Wah -> Whammy -> amp -> Phaser ->
// Chorus -> Delay, see AudioEngine); the slots only choose which effects
// are available and enabled, not their order. An effect not hosted by any
// slot is disabled. Selecting a type already used by another slot clears
// that other slot (each effect exists once).

class EffectsSection : public QGroupBox
{
    Q_OBJECT

public:
    explicit EffectsSection(AudioEngine* engine, QWidget* parent = nullptr);

    QVariantMap captureState() const;
    void applyState(const QVariantMap& state);

private:
    static constexpr int kNumSlots = 4;

    // Combo/page index; order matters and is shared with the .cpp tables.
    enum EffectType { None = 0, Wah, Whammy, Phaser, Chorus, Delay, kNumTypes };

    struct Slot
    {
        QComboBox*      type;
        QPushButton*    active;
        QStackedWidget* pages;
        CustomKnob*     wahPosition;
        CustomKnob*     whammyPitch;
        CustomKnob*     phaserRate;
        CustomKnob*     chorusRate;
        CustomKnob*     chorusDepth;
        CustomKnob*     delayTime;
        CustomKnob*     delayFeedback;
        CustomKnob*     delayMix;
    };

    QWidget* buildSlot(int index);
    void onTypeSelected(int slotIndex);
    void applyToEngine();

    AudioEngine* engine_;
    Slot slots_[kNumSlots] = {};
};
