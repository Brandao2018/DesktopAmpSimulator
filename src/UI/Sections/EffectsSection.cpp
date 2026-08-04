#include "EffectsSection.h"

#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSignalBlocker>
#include <QStackedWidget>
#include <QVBoxLayout>

#include "Audio/AudioEngine.h"
#include "UI/Widgets/CustomKnob.h"

EffectsSection::EffectsSection(AudioEngine* engine, QWidget* parent)
    : QGroupBox(tr("EFFECTS"), parent),
      engine_(engine)
{
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 22, 14, 10);
    layout->setSpacing(10);

    for (int i = 0; i < kNumSlots; ++i)
        layout->addWidget(buildSlot(i), 1);

    // Default board: Wah / Whammy / Phaser / Delay (Chorus via any dropdown),
    // everything bypassed until the player stomps it on.
    slots_[0].type->setCurrentIndex(Wah);
    slots_[1].type->setCurrentIndex(Whammy);
    slots_[2].type->setCurrentIndex(Phaser);
    slots_[3].type->setCurrentIndex(Delay);
    for (auto& slot : slots_)
        slot.pages->setCurrentIndex(slot.type->currentIndex());

    applyToEngine();
}

QWidget* EffectsSection::buildSlot(int index)
{
    auto* frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("fxSlot"));
    auto* slotLayout = new QVBoxLayout(frame);
    slotLayout->setContentsMargins(8, 6, 8, 6);
    slotLayout->setSpacing(4);

    auto* title = new QLabel(tr("FX %1").arg(index + 1), frame);
    title->setObjectName(QStringLiteral("slotTitle"));
    title->setAlignment(Qt::AlignCenter);

    Slot& slot = slots_[index];

    slot.type = new QComboBox(frame);
    slot.type->setAccessibleName(tr("Effect %1 type").arg(index + 1));
    slot.type->addItems({ tr("None"), tr("Wah"), tr("Whammy"), tr("Phaser"),
                          tr("Chorus"), tr("Delay") });

    slot.active = new QPushButton(tr("BYPASS"), frame);
    slot.active->setObjectName(QStringLiteral("bypassBtn"));
    slot.active->setCheckable(true);
    slot.active->setChecked(false);
    slot.active->setAccessibleName(tr("Effect %1 bypass").arg(index + 1));

    // One page of knobs per effect type (page index == combo index).
    slot.pages = new QStackedWidget(frame);

    auto makePage = [frame, &slot](std::initializer_list<CustomKnob*> knobs) {
        auto* page = new QWidget(frame);
        auto* row = new QHBoxLayout(page);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(2);
        row->addStretch();
        for (auto* knob : knobs)
            row->addWidget(knob);
        row->addStretch();
        slot.pages->addWidget(page);
    };

    // Page 0: None.
    makePage({});

    slot.wahPosition = new CustomKnob(tr("Position"), 0.0f, 100.0f, 50.0f, tr("%"), frame);
    makePage({ slot.wahPosition });

    slot.whammyPitch = new CustomKnob(tr("Pitch"), -12.0f, 24.0f, 0.0f, tr("st"), frame);
    makePage({ slot.whammyPitch });

    slot.phaserRate = new CustomKnob(tr("Rate"), 0.1f, 6.0f, 0.8f, tr("Hz"), frame);
    makePage({ slot.phaserRate });

    slot.chorusRate = new CustomKnob(tr("Rate"), 0.1f, 5.0f, 0.8f, tr("Hz"), frame);
    slot.chorusDepth = new CustomKnob(tr("Depth"), 0.0f, 100.0f, 50.0f, tr("%"), frame);
    makePage({ slot.chorusRate, slot.chorusDepth });

    slot.delayTime = new CustomKnob(tr("Time"), 50.0f, 800.0f, 350.0f, tr("ms"), frame);
    slot.delayTime->setDecimals(0);
    slot.delayFeedback = new CustomKnob(tr("Fdbk"), 0.0f, 90.0f, 35.0f, tr("%"), frame);
    slot.delayMix = new CustomKnob(tr("Mix"), 0.0f, 100.0f, 30.0f, tr("%"), frame);
    makePage({ slot.delayTime, slot.delayFeedback, slot.delayMix });

    slotLayout->addWidget(title);
    slotLayout->addWidget(slot.type);
    slotLayout->addWidget(slot.pages, 1);
    slotLayout->addWidget(slot.active);

    connect(slot.type, qOverload<int>(&QComboBox::activated),
            this, [this, index](int) { onTypeSelected(index); });
    connect(slot.active, &QPushButton::toggled, this, [this, index](bool on) {
        slots_[index].active->setText(on ? tr("ACTIVE") : tr("BYPASS"));
        applyToEngine();
    });
    for (auto* knob : { slot.wahPosition, slot.whammyPitch, slot.phaserRate,
                        slot.chorusRate, slot.chorusDepth,
                        slot.delayTime, slot.delayFeedback, slot.delayMix })
        connect(knob, &CustomKnob::valueChanged, this, [this](float) { applyToEngine(); });

    return frame;
}

void EffectsSection::onTypeSelected(int slotIndex)
{
    const int chosen = slots_[slotIndex].type->currentIndex();

    // Each effect exists once in the DSP chain: claiming a type used by
    // another slot empties that slot.
    if (chosen != None)
        for (int i = 0; i < kNumSlots; ++i)
            if (i != slotIndex && slots_[i].type->currentIndex() == chosen)
            {
                const QSignalBlocker blocker(slots_[i].type);
                slots_[i].type->setCurrentIndex(None);
                slots_[i].pages->setCurrentIndex(None);
            }

    slots_[slotIndex].pages->setCurrentIndex(chosen);
    applyToEngine();
}

void EffectsSection::applyToEngine()
{
    // Re-derive the whole effect state from the slots. All setters are
    // atomics, so a full rewrite per UI change is cheap and keeps this
    // logic impossible to get out of sync.
    const Slot* hosts[kNumTypes] = {};
    for (const auto& slot : slots_)
        hosts[slot.type->currentIndex()] = &slot;

    auto hostedAndActive = [&](EffectType type) {
        return hosts[type] != nullptr && hosts[type]->active->isChecked();
    };

    engine_->wah().setEnabled(hostedAndActive(Wah));
    if (const Slot* s = hosts[Wah])
        engine_->wah().setPositionPct(s->wahPosition->value());

    engine_->whammy().setEnabled(hostedAndActive(Whammy));
    if (const Slot* s = hosts[Whammy])
        engine_->whammy().setSemitones(s->whammyPitch->value());

    engine_->phaser().setEnabled(hostedAndActive(Phaser));
    if (const Slot* s = hosts[Phaser])
        engine_->phaser().setRateHz(s->phaserRate->value());

    engine_->chorus().setEnabled(hostedAndActive(Chorus));
    if (const Slot* s = hosts[Chorus])
    {
        engine_->chorus().setRateHz(s->chorusRate->value());
        engine_->chorus().setDepthPct(s->chorusDepth->value());
    }

    engine_->delay().setEnabled(hostedAndActive(Delay));
    if (const Slot* s = hosts[Delay])
    {
        engine_->delay().setTimeMs(s->delayTime->value());
        engine_->delay().setFeedbackPct(s->delayFeedback->value());
        engine_->delay().setMixPct(s->delayMix->value());
    }
}

QVariantMap EffectsSection::captureState() const
{
    QVariantMap state;
    for (int i = 0; i < kNumSlots; ++i)
    {
        const QString prefix = QStringLiteral("slot%1.").arg(i);
        const Slot& slot = slots_[i];
        state.insert(prefix + QStringLiteral("type"), slot.type->currentIndex());
        state.insert(prefix + QStringLiteral("active"), slot.active->isChecked());
        state.insert(prefix + QStringLiteral("wahPosition"), slot.wahPosition->value());
        state.insert(prefix + QStringLiteral("whammyPitch"), slot.whammyPitch->value());
        state.insert(prefix + QStringLiteral("phaserRate"), slot.phaserRate->value());
        state.insert(prefix + QStringLiteral("chorusRate"), slot.chorusRate->value());
        state.insert(prefix + QStringLiteral("chorusDepth"), slot.chorusDepth->value());
        state.insert(prefix + QStringLiteral("delayTime"), slot.delayTime->value());
        state.insert(prefix + QStringLiteral("delayFeedback"), slot.delayFeedback->value());
        state.insert(prefix + QStringLiteral("delayMix"), slot.delayMix->value());
    }
    return state;
}

void EffectsSection::applyState(const QVariantMap& state)
{
    for (int i = 0; i < kNumSlots; ++i)
    {
        const QString prefix = QStringLiteral("slot%1.").arg(i);
        Slot& slot = slots_[i];

        const QSignalBlocker typeBlocker(slot.type);
        const QSignalBlocker activeBlocker(slot.active);
        slot.type->setCurrentIndex(state.value(prefix + QStringLiteral("type"), None).toInt());
        slot.pages->setCurrentIndex(slot.type->currentIndex());
        const bool active = state.value(prefix + QStringLiteral("active"), false).toBool();
        slot.active->setChecked(active);
        slot.active->setText(active ? tr("ACTIVE") : tr("BYPASS"));

        slot.wahPosition->setValue(state.value(prefix + QStringLiteral("wahPosition"), 50.0f).toFloat());
        slot.whammyPitch->setValue(state.value(prefix + QStringLiteral("whammyPitch"), 0.0f).toFloat());
        slot.phaserRate->setValue(state.value(prefix + QStringLiteral("phaserRate"), 0.8f).toFloat());
        slot.chorusRate->setValue(state.value(prefix + QStringLiteral("chorusRate"), 0.8f).toFloat());
        slot.chorusDepth->setValue(state.value(prefix + QStringLiteral("chorusDepth"), 50.0f).toFloat());
        slot.delayTime->setValue(state.value(prefix + QStringLiteral("delayTime"), 350.0f).toFloat());
        slot.delayFeedback->setValue(state.value(prefix + QStringLiteral("delayFeedback"), 35.0f).toFloat());
        slot.delayMix->setValue(state.value(prefix + QStringLiteral("delayMix"), 30.0f).toFloat());
    }
    applyToEngine();
}
