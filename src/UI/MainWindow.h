#pragma once

#include <QMainWindow>

class QComboBox;
class QLabel;
class QPushButton;
class QTimer;

class AudioEngine;
class KnobWidget;
class MeterWidget;

// Main application window, styled after classic studio/pedalboard software:
// a top toolbar (device, buffer, readouts), a "board" area holding the rack
// meters (pedals arrive in Phase 2), and an amp panel with rotary knobs and
// a round power switch.
//
// It reads the engine's lock-free meter atomics on a 30 Hz timer and forwards
// user actions to the engine on the UI thread.

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AudioEngine* engine, QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateMeters();                // timer-driven UI refresh (~30 Hz)
    void onPowerToggled(bool on);
    void onDeviceSelected(int index);
    void onBufferSizeSelected(int index);

private:
    void setupUI();
    void connectSignals();
    void refreshDeviceList();
    void showStatus(const QString& message, bool isError = false);
    void restoreWindowGeometry();
    void saveWindowGeometry();

    void closeEvent(QCloseEvent* event) override;

    AudioEngine* audioEngine_;
    MeterWidget* inputMeter_;
    MeterWidget* outputMeter_;
    QComboBox*   deviceSelector_;
    QComboBox*   bufferSizeSelector_;
    KnobWidget*  gainKnob_;
    KnobWidget*  volumeKnob_;
    QPushButton* powerBtn_;
    QLabel*      statusLabel_;
    QLabel*      sampleRateLabel_;
    QLabel*      latencyLabel_;
    QTimer*      updateTimer_;
};
