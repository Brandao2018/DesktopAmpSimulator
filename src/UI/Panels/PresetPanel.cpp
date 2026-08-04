#include "PresetPanel.h"

#include <QHBoxLayout>
#include <QLineEdit>
#include <QListWidget>
#include <QPushButton>
#include <QSettings>
#include <QVBoxLayout>

namespace
{
    const QString kGroup = QStringLiteral("presets");

}

PresetPanel::PresetPanel(CaptureFn capture, ApplyFn apply, QWidget* parent)
    : QDockWidget(tr("PRESETS"), parent),
      capture_(std::move(capture)),
      apply_(std::move(apply))
{
    setObjectName(QStringLiteral("presetPanel"));
    setFeatures(QDockWidget::DockWidgetMovable | QDockWidget::DockWidgetClosable);
    setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);

    auto* body = new QWidget(this);
    auto* layout = new QVBoxLayout(body);
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    searchField_ = new QLineEdit(body);
    searchField_->setPlaceholderText(tr("Search presets…"));
    searchField_->setClearButtonEnabled(true);
    searchField_->setAccessibleName(tr("Search presets"));

    list_ = new QListWidget(body);
    list_->setAccessibleName(tr("Preset list"));

    nameField_ = new QLineEdit(body);
    nameField_->setPlaceholderText(tr("Preset name"));
    nameField_->setAccessibleName(tr("Preset name"));

    saveBtn_ = new QPushButton(tr("Save"), body);
    loadBtn_ = new QPushButton(tr("Load"), body);
    deleteBtn_ = new QPushButton(tr("Delete"), body);

    auto* buttonRow = new QHBoxLayout();
    buttonRow->addWidget(saveBtn_);
    buttonRow->addWidget(loadBtn_);
    buttonRow->addWidget(deleteBtn_);

    layout->addWidget(searchField_);
    layout->addWidget(list_, 1);
    layout->addWidget(nameField_);
    layout->addLayout(buttonRow);
    setWidget(body);

    connect(searchField_, &QLineEdit::textChanged, this, [this](const QString& text) {
        for (int i = 0; i < list_->count(); ++i)
        {
            auto* item = list_->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    });
    connect(list_, &QListWidget::currentTextChanged,
            this, [this](const QString& name) { nameField_->setText(name); });
    connect(list_, &QListWidget::itemDoubleClicked,
            this, [this](QListWidgetItem*) { loadSelected(); });
    connect(saveBtn_, &QPushButton::clicked, this, &PresetPanel::savePreset);
    connect(loadBtn_, &QPushButton::clicked, this, &PresetPanel::loadSelected);
    connect(deleteBtn_, &QPushButton::clicked, this, &PresetPanel::deleteSelected);

    refreshList();
}

void PresetPanel::refreshList()
{
    list_->clear();
    QSettings settings(QStringLiteral("AmpSim"), QStringLiteral("DesktopAmpSimulator"));
    settings.beginGroup(kGroup);
    QStringList names = settings.childKeys();
    names.sort(Qt::CaseInsensitive);
    list_->addItems(names);
    settings.endGroup();
}

void PresetPanel::savePreset()
{
    const QString name = nameField_->text().trimmed();
    if (name.isEmpty())
    {
        emit statusMessage(tr("Type a preset name before saving."));
        return;
    }

    QSettings settings(QStringLiteral("AmpSim"), QStringLiteral("DesktopAmpSimulator"));
    settings.beginGroup(kGroup);
    settings.setValue(name, capture_());
    settings.endGroup();

    refreshList();
    const auto matches = list_->findItems(name, Qt::MatchExactly);
    if (!matches.isEmpty())
        list_->setCurrentItem(matches.first());
    emit statusMessage(tr("Preset \"%1\" saved.").arg(name));
}

void PresetPanel::loadSelected()
{
    auto* item = list_->currentItem();
    if (item == nullptr)
    {
        emit statusMessage(tr("Select a preset to load."));
        return;
    }

    QSettings settings(QStringLiteral("AmpSim"), QStringLiteral("DesktopAmpSimulator"));
    settings.beginGroup(kGroup);
    const QVariantMap state = settings.value(item->text()).toMap();
    settings.endGroup();

    apply_(state);
    emit statusMessage(tr("Preset \"%1\" loaded.").arg(item->text()));
}

void PresetPanel::deleteSelected()
{
    auto* item = list_->currentItem();
    if (item == nullptr)
    {
        emit statusMessage(tr("Select a preset to delete."));
        return;
    }

    QSettings settings(QStringLiteral("AmpSim"), QStringLiteral("DesktopAmpSimulator"));
    settings.beginGroup(kGroup);
    settings.remove(item->text());
    settings.endGroup();

    emit statusMessage(tr("Preset \"%1\" deleted.").arg(item->text()));
    refreshList();
}
