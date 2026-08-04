#include "CabinetSection.h"

#include <QComboBox>
#include <QFormLayout>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QVBoxLayout>

#include "UI/Widgets/CustomKnob.h"

namespace
{
    const char* kPhase3Tip = "Visual only for now — becomes audible with the Phase 3 IR convolver.";
}

CabinetSection::CabinetSection(QWidget* parent)
    : QGroupBox(tr("CABINET"), parent)
{
    setToolTip(tr(kPhase3Tip));

    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(14, 22, 14, 10);
    layout->setSpacing(10);

    for (int i = 0; i < kNumSlots; ++i)
        layout->addWidget(buildSlot(i), 1);
}

QWidget* CabinetSection::buildSlot(int index)
{
    auto* frame = new QFrame(this);
    frame->setObjectName(QStringLiteral("cabSlot"));
    auto* slotLayout = new QVBoxLayout(frame);
    slotLayout->setContentsMargins(8, 6, 8, 6);
    slotLayout->setSpacing(4);

    auto* title = new QLabel(tr("CAB %1").arg(index + 1), frame);
    title->setObjectName(QStringLiteral("slotTitle"));
    title->setAlignment(Qt::AlignCenter);

    Slot& slot = slots_[index];

    slot.cabinet = new QComboBox(frame);
    slot.cabinet->setAccessibleName(tr("Cabinet %1 model").arg(index + 1));
    slot.cabinet->addItems({ tr("Off"), QStringLiteral("1x12 Tweed"),
                             QStringLiteral("2x12 Classic"), QStringLiteral("4x12 Modern"),
                             QStringLiteral("4x10 Vintage") });
    slot.cabinet->setCurrentIndex(index == 0 ? 1 : 0);   // slot 1 starts populated

    slot.mic = new QComboBox(frame);
    slot.mic->setAccessibleName(tr("Cabinet %1 microphone").arg(index + 1));
    slot.mic->addItems({ QStringLiteral("SM57"), QStringLiteral("R121"),
                         QStringLiteral("U87"), QStringLiteral("MD421") });

    slot.position = new QComboBox(frame);
    slot.position->setAccessibleName(tr("Cabinet %1 mic position").arg(index + 1));
    slot.position->addItems({ tr("Cap Edge"), tr("Cap"), tr("Cone"), tr("Room") });

    auto* form = new QFormLayout();
    form->setContentsMargins(0, 0, 0, 0);
    form->setSpacing(3);
    form->addRow(tr("Cab"), slot.cabinet);
    form->addRow(tr("Mic"), slot.mic);
    form->addRow(tr("Pos"), slot.position);

    slot.level = new CustomKnob(tr("Level"), -24.0f, 6.0f, 0.0f, tr("dB"), frame);
    slot.pan   = new CustomKnob(tr("Pan"), -100.0f, 100.0f, 0.0f, QString(), frame);
    slot.angle = new CustomKnob(tr("Angle"), 0.0f, 90.0f, 0.0f, tr("°"), frame);
    slot.pan->setDecimals(0);
    slot.angle->setDecimals(0);
    for (auto* knob : { slot.level, slot.pan, slot.angle })
        knob->setToolTip(tr(kPhase3Tip));

    auto* knobRow = new QHBoxLayout();
    knobRow->setSpacing(2);
    knobRow->addWidget(slot.level);
    knobRow->addWidget(slot.pan);
    knobRow->addWidget(slot.angle);

    slotLayout->addWidget(title);
    slotLayout->addLayout(form);
    slotLayout->addLayout(knobRow);

    return frame;
}

QVariantMap CabinetSection::captureState() const
{
    QVariantMap state;
    for (int i = 0; i < kNumSlots; ++i)
    {
        const QString prefix = QStringLiteral("slot%1.").arg(i);
        const Slot& slot = slots_[i];
        state.insert(prefix + QStringLiteral("cabinet"), slot.cabinet->currentIndex());
        state.insert(prefix + QStringLiteral("mic"), slot.mic->currentIndex());
        state.insert(prefix + QStringLiteral("position"), slot.position->currentIndex());
        state.insert(prefix + QStringLiteral("level"), slot.level->value());
        state.insert(prefix + QStringLiteral("pan"), slot.pan->value());
        state.insert(prefix + QStringLiteral("angle"), slot.angle->value());
    }
    return state;
}

void CabinetSection::applyState(const QVariantMap& state)
{
    for (int i = 0; i < kNumSlots; ++i)
    {
        const QString prefix = QStringLiteral("slot%1.").arg(i);
        Slot& slot = slots_[i];
        slot.cabinet->setCurrentIndex(state.value(prefix + QStringLiteral("cabinet"),
                                                  i == 0 ? 1 : 0).toInt());
        slot.mic->setCurrentIndex(state.value(prefix + QStringLiteral("mic"), 0).toInt());
        slot.position->setCurrentIndex(state.value(prefix + QStringLiteral("position"), 0).toInt());
        slot.level->setValue(state.value(prefix + QStringLiteral("level"), 0.0f).toFloat());
        slot.pan->setValue(state.value(prefix + QStringLiteral("pan"), 0.0f).toFloat());
        slot.angle->setValue(state.value(prefix + QStringLiteral("angle"), 0.0f).toFloat());
    }
}
