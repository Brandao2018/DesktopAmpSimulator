#pragma once

#include <QDockWidget>
#include <QVariantMap>

#include <functional>

class QLineEdit;
class QListWidget;
class QPushButton;

// Preset dock: named snapshots of the whole rig (amp, cabinet, effects,
// master), stored in QSettings. The panel owns storage and the list UI;
// MainWindow supplies capture/apply callbacks so the panel needs no
// knowledge of the sections themselves.

class PresetPanel : public QDockWidget
{
    Q_OBJECT

public:
    using CaptureFn = std::function<QVariantMap()>;
    using ApplyFn   = std::function<void(const QVariantMap&)>;

    PresetPanel(CaptureFn capture, ApplyFn apply, QWidget* parent = nullptr);

signals:
    void statusMessage(const QString& message);

private:
    void refreshList();
    void savePreset();
    void loadSelected();
    void deleteSelected();

    CaptureFn    capture_;
    ApplyFn      apply_;
    QLineEdit*   searchField_;
    QLineEdit*   nameField_;
    QListWidget* list_;
    QPushButton* saveBtn_;
    QPushButton* loadBtn_;
    QPushButton* deleteBtn_;
};
