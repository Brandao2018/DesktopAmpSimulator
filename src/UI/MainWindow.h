#pragma once

#include <QMainWindow>
#include <QVariantMap>

class QComboBox;
class QLabel;
class QPushButton;
class QTimer;

class AudioEngine;
class AmpSection;
class CabinetSection;
class EffectsSection;
class MasterSection;
class PresetPanel;

// Main window in the professional dark studio style: a slim top bar with
// device controls and the power switch, a vertically scrolling stack of
// sections (AMP -> CABINET -> EFFECTS -> MASTER), a preset dock on the
// right, and a status bar reporting device / sample rate / latency / CPU.
//
// Meters refresh at ~30 Hz from the engine's lock-free atomics; the status
// bar refreshes every 500 ms. All engine calls happen on the UI thread.

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AudioEngine* engine, QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateMeters();                // ~30 Hz
    void updateStatusBar();             // every 500 ms
    void onPowerToggled(bool on);
    void onDeviceSelected(int index);
    void onBufferSizeSelected(int index);

private:
    void setupUI();
    void connectSignals();
    void refreshDeviceList();
    QVariantMap captureState() const;
    void applyState(const QVariantMap& state);
    void showStatus(const QString& message);
    void restoreWindowGeometry();
    void saveWindowGeometry();

    void closeEvent(QCloseEvent* event) override;

    AudioEngine*    audioEngine_;
    AmpSection*     ampSection_;
    CabinetSection* cabinetSection_;
    EffectsSection* effectsSection_;
    MasterSection*  masterSection_;
    PresetPanel*    presetPanel_;
    QComboBox*      deviceSelector_;
    QComboBox*      bufferSizeSelector_;
    QPushButton*    powerBtn_;
    QLabel*         deviceStatusLabel_;
    QLabel*         sampleRateLabel_;
    QLabel*         latencyLabel_;
    QLabel*         cpuLabel_;
    QTimer*         meterTimer_;
    QTimer*         statusTimer_;
};
