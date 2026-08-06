#pragma once

#include <QGroupBox>
#include <QVariantMap>

class QComboBox;
class QPushButton;

class AmpHeadWidget;
class AudioEngine;
class CustomKnob;

// AMP section: a model dropdown spanning the whole catalog (each entry maps
// to one of the four DSP voicings and its own faceplate cosmetics), tube
// selectors (visual only until Phase 3), and a skeuomorphic amp head whose
// panel carries the knobs. Gain/Drive/Bass/Mid/Treble drive the real DSP;
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
    void applyModelVisuals(int catalogIndex);

    AudioEngine*   engine_;
    QComboBox*     modelSelector_;
    QComboBox*     preampTubeSelector_;
    QComboBox*     powerTubeSelector_;
    QPushButton*   bypassBtn_;
    AmpHeadWidget* head_;
    CustomKnob*    gainKnob_;
    CustomKnob*    driveKnob_;
    CustomKnob*    bassKnob_;
    CustomKnob*    midKnob_;
    CustomKnob*    trebleKnob_;
    CustomKnob*    presenceKnob_;   // visual only (Phase 3)
    CustomKnob*    depthKnob_;      // visual only (Phase 3)
    CustomKnob*    sagKnob_;        // visual only (Phase 3)
    CustomKnob*    masterKnob_;     // visual only (Phase 3)
};
