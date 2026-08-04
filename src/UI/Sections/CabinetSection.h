#pragma once

#include <QGroupBox>
#include <QVariantMap>

class QComboBox;

class CustomKnob;

// CABINET section: four cabinet slots, each with cabinet/mic/position
// dropdowns and Level/Pan/Angle knobs. Entirely visual for now — the IR
// convolver that makes these audible is Phase 3 — so the controls carry
// tooltips saying so and captureState() keeps their settings in presets.

class CabinetSection : public QGroupBox
{
    Q_OBJECT

public:
    explicit CabinetSection(QWidget* parent = nullptr);

    QVariantMap captureState() const;
    void applyState(const QVariantMap& state);

private:
    static constexpr int kNumSlots = 4;

    struct Slot
    {
        QComboBox*  cabinet;
        QComboBox*  mic;
        QComboBox*  position;
        CustomKnob* level;
        CustomKnob* pan;
        CustomKnob* angle;
    };

    QWidget* buildSlot(int index);

    Slot slots_[kNumSlots] = {};
};
