#include "MainWindow.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QTimer>
#include <QVBoxLayout>

#include "Audio/AudioEngine.h"
#include "Shared/Constants.h"
#include "UI/MeterWidget.h"

MainWindow::MainWindow(AudioEngine* engine, QWidget* parent)
    : QMainWindow(parent),
      audioEngine_(engine),
      inputMeter_(nullptr),
      outputMeter_(nullptr),
      deviceSelector_(nullptr),
      bufferSizeSelector_(nullptr),
      startStopBtn_(nullptr),
      statusLabel_(nullptr),
      sampleRateLabel_(nullptr),
      bufferSizeLabel_(nullptr),
      latencyLabel_(nullptr),
      updateTimer_(nullptr)
{
    setupUI();
    connectSignals();
    refreshDeviceList();
    restoreWindowGeometry();
    showStatus(tr("Ready — press Start to begin audio."));
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    setWindowTitle(tr("Desktop Amp Simulator — Phase 1"));
    resize(600, 400);

    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout(central);

    inputMeter_ = new MeterWidget(tr("Input"), central);
    outputMeter_ = new MeterWidget(tr("Output"), central);

    auto* inputGroup = new QGroupBox(tr("Input"), central);
    auto* inputLayout = new QVBoxLayout(inputGroup);
    inputLayout->addWidget(inputMeter_);

    auto* outputGroup = new QGroupBox(tr("Output"), central);
    auto* outputLayout = new QVBoxLayout(outputGroup);
    outputLayout->addWidget(outputMeter_);

    auto* settingsGroup = new QGroupBox(tr("Audio Device"), central);
    auto* form = new QFormLayout(settingsGroup);

    deviceSelector_ = new QComboBox(settingsGroup);
    deviceSelector_->setAccessibleName(tr("Audio device"));
    form->addRow(tr("Device:"), deviceSelector_);

    bufferSizeSelector_ = new QComboBox(settingsGroup);
    bufferSizeSelector_->setAccessibleName(tr("Buffer size"));
    for (int size : ampsim::kBufferSizes)
        bufferSizeSelector_->addItem(QString::number(size), size);
    bufferSizeSelector_->setCurrentIndex(2);   // 256 by default
    form->addRow(tr("Buffer size:"), bufferSizeSelector_);

    sampleRateLabel_ = new QLabel(tr("—"), settingsGroup);
    form->addRow(tr("Sample rate:"), sampleRateLabel_);

    bufferSizeLabel_ = new QLabel(tr("—"), settingsGroup);
    form->addRow(tr("Actual buffer:"), bufferSizeLabel_);

    latencyLabel_ = new QLabel(tr("—"), settingsGroup);
    form->addRow(tr("Roundtrip latency:"), latencyLabel_);

    startStopBtn_ = new QPushButton(tr("Start"), central);
    startStopBtn_->setMinimumHeight(36);

    statusLabel_ = new QLabel(central);
    statusLabel_->setWordWrap(true);

    layout->addWidget(inputGroup);
    layout->addWidget(outputGroup);
    layout->addWidget(settingsGroup);
    layout->addWidget(startStopBtn_);
    layout->addWidget(statusLabel_);
    layout->addStretch();

    setCentralWidget(central);
}

void MainWindow::connectSignals()
{
    connect(startStopBtn_, &QPushButton::clicked, this, &MainWindow::onStartStopClicked);
    connect(deviceSelector_, qOverload<int>(&QComboBox::activated),
            this, &MainWindow::onDeviceSelected);
    connect(bufferSizeSelector_, qOverload<int>(&QComboBox::activated),
            this, &MainWindow::onBufferSizeSelected);

    updateTimer_ = new QTimer(this);
    updateTimer_->setInterval(ampsim::kUiRefreshMs);
    connect(updateTimer_, &QTimer::timeout, this, &MainWindow::updateMeters);
    updateTimer_->start();
}

void MainWindow::refreshDeviceList()
{
    const QSignalBlocker blocker(deviceSelector_);
    deviceSelector_->clear();

    const auto names = audioEngine_->getAvailableDeviceNames();
    for (const auto& name : names)
        deviceSelector_->addItem(QString::fromStdString(name));

    const QString current = QString::fromStdString(audioEngine_->getCurrentDeviceName());
    const int idx = deviceSelector_->findText(current);
    if (idx >= 0)
        deviceSelector_->setCurrentIndex(idx);

    if (names.empty())
        showStatus(tr("No audio devices found. Connect an interface and restart audio."), true);
}

void MainWindow::updateMeters()
{
    // Lock-free reads of the audio thread's atomics — never blocks audio.
    inputMeter_->setLevels(audioEngine_->getInputLevelL(),
                           audioEngine_->getInputLevelR());
    outputMeter_->setLevels(audioEngine_->getOutputLevelL(),
                            audioEngine_->getOutputLevelR());

    const double rate = audioEngine_->getCurrentSampleRate();
    sampleRateLabel_->setText(rate > 0.0 ? tr("%1 Hz").arg(rate, 0, 'f', 0) : tr("—"));

    const int buffer = audioEngine_->getCurrentBufferSize();
    bufferSizeLabel_->setText(buffer > 0 ? tr("%1 samples").arg(buffer) : tr("—"));

    const float latency = audioEngine_->getMeasuredLatency();
    latencyLabel_->setText(latency > 0.0f ? tr("%1 ms").arg(latency, 0, 'f', 1) : tr("—"));

    // Hot-plug: refresh the device list when the OS reports a change.
    if (audioEngine_->consumeDeviceListChanged())
        refreshDeviceList();
}

void MainWindow::onStartStopClicked()
{
    if (audioEngine_->isRunning())
    {
        audioEngine_->stop();
        startStopBtn_->setText(tr("Start"));
        showStatus(tr("Stopped."));
        return;
    }

    if (audioEngine_->start())
    {
        startStopBtn_->setText(tr("Stop"));
        showStatus(tr("Running on: %1")
                       .arg(QString::fromStdString(audioEngine_->getCurrentDeviceName())));
        refreshDeviceList();
    }
    else
    {
        showStatus(QString::fromStdString(audioEngine_->getLastError()), true);
    }
}

void MainWindow::onDeviceSelected(int index)
{
    const QString name = deviceSelector_->itemText(index);
    if (audioEngine_->setAudioDevice(name))
        showStatus(tr("Switched to: %1").arg(name));
    else
        showStatus(QString::fromStdString(audioEngine_->getLastError()), true);
}

void MainWindow::onBufferSizeSelected(int index)
{
    const int size = bufferSizeSelector_->itemData(index).toInt();
    if (audioEngine_->setBufferSize(size))
        showStatus(tr("Buffer size set to %1 samples.").arg(size));
    else
        showStatus(QString::fromStdString(audioEngine_->getLastError()), true);
}

void MainWindow::showStatus(const QString& message, bool isError)
{
    statusLabel_->setText(message);
    statusLabel_->setStyleSheet(isError ? QStringLiteral("color: #e65a50;")
                                        : QString());
}

void MainWindow::restoreWindowGeometry()
{
    QSettings settings(QStringLiteral("AmpSim"), QStringLiteral("DesktopAmpSimulator"));
    const QByteArray geometry = settings.value(QStringLiteral("geometry")).toByteArray();
    if (!geometry.isEmpty())
        restoreGeometry(geometry);
}

void MainWindow::saveWindowGeometry()
{
    QSettings settings(QStringLiteral("AmpSim"), QStringLiteral("DesktopAmpSimulator"));
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
}

void MainWindow::closeEvent(QCloseEvent* event)
{
    saveWindowGeometry();
    audioEngine_->stop();
    QMainWindow::closeEvent(event);
    event->accept();
}
