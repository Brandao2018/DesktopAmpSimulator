#include "AmpSection.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Audio/AudioEngine.h"
#include "UI/AmpCatalog.h"
#include "UI/Widgets/AmpHeadWidget.h"
#include "UI/Widgets/CustomKnob.h"

namespace
{
    const char* kPhase3Tip = "Visual only for now — becomes active with the Phase 3 power-amp model.";
}

AmpSection::AmpSection(AudioEngine* engine, QWidget* parent)
    : QGroupBox(tr("AMP"), parent),
      engine_(engine)
{
    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(14, 22, 14, 10);
    layout->setSpacing(8);

    // --- Selector row: amp model + tube types + bypass -----------------------
    auto* selectorRow = new QHBoxLayout();
    selectorRow->setSpacing(8);

    modelSelector_ = new QComboBox(this);
    modelSelector_->setAccessibleName(tr("Amp model"));
    for (int i = 0; i < ampcat::kNumAmpModels; ++i)
        modelSelector_->addItem(QStringLiteral("%1 — %2")
                                    .arg(QLatin1String(ampcat::kAmpModels[i].maker),
                                         QLatin1String(ampcat::kAmpModels[i].model)),
                                i);

    preampTubeSelector_ = new QComboBox(this);
    preampTubeSelector_->setAccessibleName(tr("Preamp tube"));
    preampTubeSelector_->addItems({ QStringLiteral("12AX7"), QStringLiteral("12AT7"),
                                    QStringLiteral("12AU7") });
    preampTubeSelector_->setToolTip(tr(kPhase3Tip));

    powerTubeSelector_ = new QComboBox(this);
    powerTubeSelector_->setAccessibleName(tr("Power tube"));
    powerTubeSelector_->addItems({ QStringLiteral("EL34"), QStringLiteral("6L6GC"),
                                   QStringLiteral("EL84") });
    powerTubeSelector_->setToolTip(tr(kPhase3Tip));

    bypassBtn_ = new QPushButton(tr("ACTIVE"), this);
    bypassBtn_->setObjectName(QStringLiteral("bypassBtn"));
    bypassBtn_->setCheckable(true);
    bypassBtn_->setChecked(true);
    bypassBtn_->setAccessibleName(tr("Amp bypass"));
    bypassBtn_->setFixedWidth(80);

    selectorRow->addWidget(new QLabel(tr("Model"), this));
    selectorRow->addWidget(modelSelector_, 1);
    selectorRow->addWidget(new QLabel(tr("Preamp"), this));
    selectorRow->addWidget(preampTubeSelector_);
    selectorRow->addWidget(new QLabel(tr("Power"), this));
    selectorRow->addWidget(powerTubeSelector_);
    selectorRow->addStretch();
    selectorRow->addWidget(bypassBtn_);

    // --- Amp head with a single row of panel knobs ---------------------------
    head_ = new AmpHeadWidget(this);

    gainKnob_     = new CustomKnob(tr("Gain"), -24.0f, 24.0f, 0.0f, tr("dB"), head_);
    driveKnob_    = new CustomKnob(tr("Drive"), 0.0f, 36.0f, 12.0f, tr("dB"), head_);
    bassKnob_     = new CustomKnob(tr("Bass"), -12.0f, 12.0f, 0.0f, tr("dB"), head_);
    midKnob_      = new CustomKnob(tr("Mid"), -12.0f, 12.0f, 0.0f, tr("dB"), head_);
    trebleKnob_   = new CustomKnob(tr("Treble"), -12.0f, 12.0f, 0.0f, tr("dB"), head_);
    presenceKnob_ = new CustomKnob(tr("Presence"), -12.0f, 12.0f, 0.0f, tr("dB"), head_);
    depthKnob_    = new CustomKnob(tr("Depth"), 0.0f, 100.0f, 50.0f, tr("%"), head_);
    sagKnob_      = new CustomKnob(tr("Sag"), 0.0f, 100.0f, 30.0f, tr("%"), head_);
    masterKnob_   = new CustomKnob(tr("Master"), 0.0f, 100.0f, 70.0f, tr("%"), head_);

    presenceKnob_->setToolTip(tr(kPhase3Tip));
    depthKnob_->setToolTip(tr(kPhase3Tip));
    sagKnob_->setToolTip(tr(kPhase3Tip));
    masterKnob_->setToolTip(tr(kPhase3Tip));

    CustomKnob* knobs[9] = { gainKnob_, driveKnob_, bassKnob_,
                             midKnob_, trebleKnob_, presenceKnob_,
                             depthKnob_, sagKnob_, masterKnob_ };
    for (auto* knob : knobs)
    {
        head_->knobRow()->addStretch(1);
        head_->knobRow()->addWidget(knob);
    }
    head_->knobRow()->addStretch(1);

    layout->addLayout(selectorRow);
    layout->addWidget(head_);

    // --- Wiring --------------------------------------------------------------
    connect(modelSelector_, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        const int catalogIndex = modelSelector_->itemData(index).toInt();
        applyModelVisuals(catalogIndex);
        engine_->ampModel().setVoicing(ampcat::kAmpModels[catalogIndex].voicing);
    });
    connect(bypassBtn_, &QPushButton::toggled, this, [this](bool on) {
        engine_->ampModel().setEnabled(on);
        bypassBtn_->setText(on ? tr("ACTIVE") : tr("BYPASS"));
        head_->setLampOn(on);
    });
    connect(gainKnob_, &CustomKnob::valueChanged,
            this, [this](float db) { engine_->setGainDb(db); });
    connect(driveKnob_, &CustomKnob::valueChanged,
            this, [this](float db) { engine_->ampModel().setDriveDb(db); });
    connect(bassKnob_, &CustomKnob::valueChanged,
            this, [this](float db) { engine_->ampModel().setBassDb(db); });
    connect(midKnob_, &CustomKnob::valueChanged,
            this, [this](float db) { engine_->ampModel().setMidDb(db); });
    connect(trebleKnob_, &CustomKnob::valueChanged,
            this, [this](float db) { engine_->ampModel().setTrebleDb(db); });

    applyModelVisuals(0);
    pushAllToEngine();
}

void AmpSection::applyModelVisuals(int catalogIndex)
{
    const auto& spec = ampcat::kAmpModels[catalogIndex];
    head_->setSpec(&spec);

    CustomKnob* knobs[9] = { gainKnob_, driveKnob_, bassKnob_,
                             midKnob_, trebleKnob_, presenceKnob_,
                             depthKnob_, sagKnob_, masterKnob_ };
    for (auto* knob : knobs)
    {
        knob->setKnobStyle(spec.knobStyle);
        knob->setTextColors(spec.panelText, spec.panelText);
    }
}

void AmpSection::pushAllToEngine()
{
    engine_->setGainDb(gainKnob_->value());
    engine_->ampModel().setDriveDb(driveKnob_->value());
    engine_->ampModel().setBassDb(bassKnob_->value());
    engine_->ampModel().setMidDb(midKnob_->value());
    engine_->ampModel().setTrebleDb(trebleKnob_->value());
    engine_->ampModel().setVoicing(
        ampcat::kAmpModels[modelSelector_->currentData().toInt()].voicing);
    engine_->ampModel().setEnabled(bypassBtn_->isChecked());
}

QVariantMap AmpSection::captureState() const
{
    return {
        { QStringLiteral("model"), modelSelector_->currentIndex() },
        { QStringLiteral("preampTube"), preampTubeSelector_->currentIndex() },
        { QStringLiteral("powerTube"), powerTubeSelector_->currentIndex() },
        { QStringLiteral("active"), bypassBtn_->isChecked() },
        { QStringLiteral("gain"), gainKnob_->value() },
        { QStringLiteral("drive"), driveKnob_->value() },
        { QStringLiteral("bass"), bassKnob_->value() },
        { QStringLiteral("mid"), midKnob_->value() },
        { QStringLiteral("treble"), trebleKnob_->value() },
        { QStringLiteral("presence"), presenceKnob_->value() },
        { QStringLiteral("depth"), depthKnob_->value() },
        { QStringLiteral("sag"), sagKnob_->value() },
        { QStringLiteral("master"), masterKnob_->value() },
    };
}

void AmpSection::applyState(const QVariantMap& state)
{
    const int model = qBound(0, state.value(QStringLiteral("model"), 0).toInt(),
                             ampcat::kNumAmpModels - 1);
    modelSelector_->setCurrentIndex(model);
    applyModelVisuals(modelSelector_->currentData().toInt());
    preampTubeSelector_->setCurrentIndex(state.value(QStringLiteral("preampTube"), 0).toInt());
    powerTubeSelector_->setCurrentIndex(state.value(QStringLiteral("powerTube"), 0).toInt());
    bypassBtn_->setChecked(state.value(QStringLiteral("active"), true).toBool());
    head_->setLampOn(bypassBtn_->isChecked());
    gainKnob_->setValue(state.value(QStringLiteral("gain"), 0.0f).toFloat());
    driveKnob_->setValue(state.value(QStringLiteral("drive"), 12.0f).toFloat());
    bassKnob_->setValue(state.value(QStringLiteral("bass"), 0.0f).toFloat());
    midKnob_->setValue(state.value(QStringLiteral("mid"), 0.0f).toFloat());
    trebleKnob_->setValue(state.value(QStringLiteral("treble"), 0.0f).toFloat());
    presenceKnob_->setValue(state.value(QStringLiteral("presence"), 0.0f).toFloat());
    depthKnob_->setValue(state.value(QStringLiteral("depth"), 50.0f).toFloat());
    sagKnob_->setValue(state.value(QStringLiteral("sag"), 30.0f).toFloat());
    masterKnob_->setValue(state.value(QStringLiteral("master"), 70.0f).toFloat());

    pushAllToEngine();   // knobs whose value didn't change emit no signal
}
