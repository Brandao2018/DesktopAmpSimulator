#include "AmpSection.h"

#include <QComboBox>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

#include "Audio/AudioEngine.h"
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
    modelSelector_->addItem(tr("Fender Deluxe"), static_cast<int>(ampsim::AmpVoicing::Fender));
    modelSelector_->addItem(tr("Marshall JCM"), static_cast<int>(ampsim::AmpVoicing::Marshall));
    modelSelector_->addItem(tr("Mesa Rectifier"), static_cast<int>(ampsim::AmpVoicing::Mesa));
    modelSelector_->addItem(tr("Orange Crush"), static_cast<int>(ampsim::AmpVoicing::Orange));

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

    // --- 3x3 knob grid -------------------------------------------------------
    gainKnob_     = new CustomKnob(tr("Gain"), -24.0f, 24.0f, 0.0f, tr("dB"), this);
    driveKnob_    = new CustomKnob(tr("Drive"), 0.0f, 36.0f, 12.0f, tr("dB"), this);
    bassKnob_     = new CustomKnob(tr("Bass"), -12.0f, 12.0f, 0.0f, tr("dB"), this);
    midKnob_      = new CustomKnob(tr("Mid"), -12.0f, 12.0f, 0.0f, tr("dB"), this);
    trebleKnob_   = new CustomKnob(tr("Treble"), -12.0f, 12.0f, 0.0f, tr("dB"), this);
    presenceKnob_ = new CustomKnob(tr("Presence"), -12.0f, 12.0f, 0.0f, tr("dB"), this);
    depthKnob_    = new CustomKnob(tr("Depth"), 0.0f, 100.0f, 50.0f, tr("%"), this);
    sagKnob_      = new CustomKnob(tr("Sag"), 0.0f, 100.0f, 30.0f, tr("%"), this);
    masterKnob_   = new CustomKnob(tr("Master"), 0.0f, 100.0f, 70.0f, tr("%"), this);

    presenceKnob_->setToolTip(tr(kPhase3Tip));
    depthKnob_->setToolTip(tr(kPhase3Tip));
    sagKnob_->setToolTip(tr(kPhase3Tip));
    masterKnob_->setToolTip(tr(kPhase3Tip));

    auto* grid = new QGridLayout();
    grid->setHorizontalSpacing(10);
    grid->setVerticalSpacing(4);
    CustomKnob* knobs[9] = { gainKnob_, driveKnob_, bassKnob_,
                             midKnob_, trebleKnob_, presenceKnob_,
                             depthKnob_, sagKnob_, masterKnob_ };
    for (int i = 0; i < 9; ++i)
        grid->addWidget(knobs[i], i / 3, i % 3, Qt::AlignCenter);

    auto* gridRow = new QHBoxLayout();
    gridRow->addStretch();
    gridRow->addLayout(grid);
    gridRow->addStretch();

    layout->addLayout(selectorRow);
    layout->addLayout(gridRow);

    // --- Wiring --------------------------------------------------------------
    connect(modelSelector_, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        engine_->ampModel().setVoicing(
            static_cast<ampsim::AmpVoicing>(modelSelector_->itemData(index).toInt()));
    });
    connect(bypassBtn_, &QPushButton::toggled, this, [this](bool on) {
        engine_->ampModel().setEnabled(on);
        bypassBtn_->setText(on ? tr("ACTIVE") : tr("BYPASS"));
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

    pushAllToEngine();
}

void AmpSection::pushAllToEngine()
{
    engine_->setGainDb(gainKnob_->value());
    engine_->ampModel().setDriveDb(driveKnob_->value());
    engine_->ampModel().setBassDb(bassKnob_->value());
    engine_->ampModel().setMidDb(midKnob_->value());
    engine_->ampModel().setTrebleDb(trebleKnob_->value());
    engine_->ampModel().setVoicing(
        static_cast<ampsim::AmpVoicing>(modelSelector_->currentData().toInt()));
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
    modelSelector_->setCurrentIndex(state.value(QStringLiteral("model"), 0).toInt());
    preampTubeSelector_->setCurrentIndex(state.value(QStringLiteral("preampTube"), 0).toInt());
    powerTubeSelector_->setCurrentIndex(state.value(QStringLiteral("powerTube"), 0).toInt());
    bypassBtn_->setChecked(state.value(QStringLiteral("active"), true).toBool());
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
