#include "MasterSection.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "Audio/AudioEngine.h"
#include "UI/Widgets/CustomKnob.h"
#include "UI/Widgets/LevelMeter.h"

MasterSection::MasterSection(AudioEngine* engine, QWidget* parent)
    : QGroupBox(tr("MASTER"), parent),
      engine_(engine)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 22, 14, 10);
    layout->setSpacing(16);

    auto* meters = new QVBoxLayout();
    meters->setSpacing(6);
    auto* inputLabel = new QLabel(tr("INPUT"), this);
    inputLabel->setObjectName(QStringLiteral("slotTitle"));
    inputMeter_ = new LevelMeter(tr("Input"), this);
    auto* outputLabel = new QLabel(tr("OUTPUT"), this);
    outputLabel->setObjectName(QStringLiteral("slotTitle"));
    outputMeter_ = new LevelMeter(tr("Output"), this);
    meters->addWidget(inputLabel);
    meters->addWidget(inputMeter_);
    meters->addWidget(outputLabel);
    meters->addWidget(outputMeter_);
    meters->addStretch();

    masterKnob_ = new CustomKnob(tr("Master"), -60.0f, 6.0f, 0.0f, tr("dB"), this);
    gateKnob_ = new CustomKnob(tr("Gate"), -90.0f, -30.0f, -70.0f, tr("dB"), this);
    gateKnob_->setToolTip(tr("Visual only for now — the noise gate DSP is coming later."));
    transposeKnob_ = new CustomKnob(tr("Transpose"), -12.0f, 12.0f, 0.0f, tr("st"), this);
    transposeKnob_->setDecimals(0);
    transposeKnob_->setToolTip(tr("Visual only for now — global transpose is coming later."));

    auto* knobs = new QHBoxLayout();
    knobs->setSpacing(10);
    knobs->addWidget(masterKnob_);
    knobs->addWidget(gateKnob_);
    knobs->addWidget(transposeKnob_);

    layout->addLayout(meters, 1);
    layout->addLayout(knobs);

    connect(masterKnob_, &CustomKnob::valueChanged,
            this, [this](float db) { engine_->setVolumeDb(db); });
    engine_->setVolumeDb(masterKnob_->value());
}

void MasterSection::updateMeters()
{
    inputMeter_->setLevels(engine_->getInputLevelL(), engine_->getInputLevelR());
    outputMeter_->setLevels(engine_->getOutputLevelL(), engine_->getOutputLevelR());
}

QVariantMap MasterSection::captureState() const
{
    return {
        { QStringLiteral("master"), masterKnob_->value() },
        { QStringLiteral("gate"), gateKnob_->value() },
        { QStringLiteral("transpose"), transposeKnob_->value() },
    };
}

void MasterSection::applyState(const QVariantMap& state)
{
    masterKnob_->setValue(state.value(QStringLiteral("master"), 0.0f).toFloat());
    gateKnob_->setValue(state.value(QStringLiteral("gate"), -70.0f).toFloat());
    transposeKnob_->setValue(state.value(QStringLiteral("transpose"), 0.0f).toFloat());
    engine_->setVolumeDb(masterKnob_->value());
}
