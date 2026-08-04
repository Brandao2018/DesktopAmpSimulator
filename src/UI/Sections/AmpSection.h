#pragma once

#include <QGroupBox>
#include <QVariantMap>

class QComboBox;
class QPushButton;

class AudioEngine;
class CustomKnob;

// AMP section: amp model dropdown (Fender / Marshall / Mesa / Orange — each
// switches the DSP voicing), tube selectors (visual only until Phase 3),
// and a 3x3 knob grid. Gain/Drive/Bass/Mid/Treble drive the real DSP;
// Presence/Depth/Sag/Master are visual-only placeholders for Phase 3.

class AmpSection : public QGroupBox
{
    Q_OBJECT

public:
    explicit AmpSection(AudioEngine* engine, QWidget* parent = nullptr);

    // Preset support: everything user-adjustable in this section.
    QVariantMap captureState() const;
    void applyState(const QVariantMap& state);

private:
    void pushAllToEngine();

    AudioEngine* engine_;
    QComboBox*   modelSelector_;
    QComboBox*   preampTubeSelector_;
    QComboBox*   powerTubeSelector_;
    QPushButton* bypassBtn_;
    CustomKnob*  gainKnob_;
    CustomKnob*  driveKnob_;
    CustomKnob*  bassKnob_;
    CustomKnob*  midKnob_;
    CustomKnob*  trebleKnob_;
    CustomKnob*  presenceKnob_;   // visual only (Phase 3)
    CustomKnob*  depthKnob_;      // visual only (Phase 3)
    CustomKnob*  sagKnob_;        // visual only (Phase 3)
    CustomKnob*  masterKnob_;     // visual only (Phase 3)
};
