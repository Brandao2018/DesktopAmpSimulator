#pragma once

#include <QGroupBox>
#include <QVariantMap>

class AudioEngine;
class CustomKnob;
class LevelMeter;

// MASTER section: input/output stereo meters (fed from the engine's
// lock-free atomics by MainWindow's refresh timer), the master volume
// (real — engine output level), and noise-gate/transpose knobs that stay
// visual-only until their DSP lands.

class MasterSection : public QGroupBox
{
    Q_OBJECT

public:
    explicit MasterSection(AudioEngine* engine, QWidget* parent = nullptr);

    // Called from MainWindow's ~30 Hz meter timer.
    void updateMeters();

    QVariantMap captureState() const;
    void applyState(const QVariantMap& state);

private:
    AudioEngine* engine_;
    LevelMeter*  inputMeter_;
    LevelMeter*  outputMeter_;
    CustomKnob*  masterKnob_;      // real: engine output volume
    CustomKnob*  gateKnob_;        // visual only (noise gate DSP pending)
    CustomKnob*  transposeKnob_;   // visual only (global transpose pending)
};
