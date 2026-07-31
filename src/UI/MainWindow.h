#pragma once

#include <QMainWindow>

class QComboBox;
class QLabel;
class QPushButton;
class QTimer;

class AudioEngine;
class MeterWidget;

// Main application window. Owns nothing audio-related; it reads the engine's
// lock-free meter atomics on a 30 Hz timer and forwards user actions
// (start/stop, device / buffer-size selection) to the engine on the UI thread.

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(AudioEngine* engine, QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void updateMeters();                // timer-driven UI refresh (~30 Hz)
    void onStartStopClicked();
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
    QPushButton* startStopBtn_;
    QLabel*      statusLabel_;
    QLabel*      sampleRateLabel_;
    QLabel*      bufferSizeLabel_;
    QLabel*      latencyLabel_;
    QTimer*      updateTimer_;
};
