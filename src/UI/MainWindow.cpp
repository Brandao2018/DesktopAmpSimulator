#include "MainWindow.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QSettings>
#include <QSignalBlocker>
#include <QStyle>
#include <QTimer>
#include <QVBoxLayout>

#include "Audio/AudioEngine.h"
#include "Shared/Constants.h"
#include "UI/KnobWidget.h"
#include "UI/MeterWidget.h"

MainWindow::MainWindow(AudioEngine* engine, QWidget* parent)
    : QMainWindow(parent),
      audioEngine_(engine),
      inputMeter_(nullptr),
      outputMeter_(nullptr),
      deviceSelector_(nullptr),
      bufferSizeSelector_(nullptr),
      gainKnob_(nullptr),
      volumeKnob_(nullptr),
      powerBtn_(nullptr),
      statusLabel_(nullptr),
      sampleRateLabel_(nullptr),
      latencyLabel_(nullptr),
      updateTimer_(nullptr)
{
    setupUI();
    connectSignals();
    refreshDeviceList();
    restoreWindowGeometry();
    showStatus(tr("Ready — hit the power switch."));
}

MainWindow::~MainWindow() = default;

void MainWindow::setupUI()
{
    setWindowTitle(tr("Desktop Amp Simulator"));
    resize(760, 560);

    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("studioBackground"));
    auto* layout = new QVBoxLayout(central);
    layout->setContentsMargins(14, 12, 14, 12);
    layout->setSpacing(10);

    // --- Top toolbar: branding + device settings + readouts -----------------
    auto* toolbar = new QFrame(central);
    toolbar->setObjectName(QStringLiteral("toolbar"));
    auto* toolbarLayout = new QHBoxLayout(toolbar);
    toolbarLayout->setContentsMargins(14, 8, 14, 8);

    auto* brand = new QLabel(tr("Desktop <b>Amp</b> Simulator"), toolbar);
    brand->setObjectName(QStringLiteral("brand"));

    deviceSelector_ = new QComboBox(toolbar);
    deviceSelector_->setAccessibleName(tr("Audio device"));
    deviceSelector_->setMinimumWidth(200);

    bufferSizeSelector_ = new QComboBox(toolbar);
    bufferSizeSelector_->setAccessibleName(tr("Buffer size"));
    for (int size : ampsim::kBufferSizes)
        bufferSizeSelector_->addItem(tr("%1 smp").arg(size), size);
    bufferSizeSelector_->setCurrentIndex(2);   // 256 by default

    sampleRateLabel_ = new QLabel(tr("— Hz"), toolbar);
    sampleRateLabel_->setObjectName(QStringLiteral("readout"));
    latencyLabel_ = new QLabel(tr("— ms"), toolbar);
    latencyLabel_->setObjectName(QStringLiteral("readout"));

    toolbarLayout->addWidget(brand);
    toolbarLayout->addStretch();
    toolbarLayout->addWidget(new QLabel(tr("Device"), toolbar));
    toolbarLayout->addWidget(deviceSelector_);
    toolbarLayout->addWidget(new QLabel(tr("Buffer"), toolbar));
    toolbarLayout->addWidget(bufferSizeSelector_);
    toolbarLayout->addWidget(sampleRateLabel_);
    toolbarLayout->addWidget(latencyLabel_);

    // --- Board area: rack meters now, pedals in Phase 2 ---------------------
    auto* board = new QFrame(central);
    board->setObjectName(QStringLiteral("board"));
    auto* boardLayout = new QVBoxLayout(board);
    boardLayout->setContentsMargins(18, 14, 18, 14);
    boardLayout->setSpacing(10);

    auto* inputRack = new QFrame(board);
    inputRack->setObjectName(QStringLiteral("rackModule"));
    auto* inputRackLayout = new QVBoxLayout(inputRack);
    auto* inputTitle = new QLabel(tr("INPUT"), inputRack);
    inputTitle->setObjectName(QStringLiteral("rackTitle"));
    inputMeter_ = new MeterWidget(tr("Input"), inputRack);
    inputRackLayout->addWidget(inputTitle);
    inputRackLayout->addWidget(inputMeter_);

    auto* outputRack = new QFrame(board);
    outputRack->setObjectName(QStringLiteral("rackModule"));
    auto* outputRackLayout = new QVBoxLayout(outputRack);
    auto* outputTitle = new QLabel(tr("OUTPUT"), outputRack);
    outputTitle->setObjectName(QStringLiteral("rackTitle"));
    outputMeter_ = new MeterWidget(tr("Output"), outputRack);
    outputRackLayout->addWidget(outputTitle);
    outputRackLayout->addWidget(outputMeter_);

    auto* pedalSlot = new QLabel(tr("Pedalboard — amp models and effects arrive in Phase 2"), board);
    pedalSlot->setObjectName(QStringLiteral("pedalSlot"));
    pedalSlot->setAlignment(Qt::AlignCenter);
    pedalSlot->setMinimumHeight(72);

    boardLayout->addWidget(inputRack);
    boardLayout->addWidget(pedalSlot, 1);
    boardLayout->addWidget(outputRack);

    // --- Amp panel: knobs + power switch ------------------------------------
    auto* ampPanel = new QFrame(central);
    ampPanel->setObjectName(QStringLiteral("ampPanel"));
    auto* ampLayout = new QHBoxLayout(ampPanel);
    ampLayout->setContentsMargins(22, 10, 22, 10);
    ampLayout->setSpacing(18);

    auto* logo = new QLabel(tr("<i>Dream</i>"), ampPanel);
    logo->setObjectName(QStringLiteral("ampLogo"));

    gainKnob_ = new KnobWidget(tr("Gain"), -24.0f, 24.0f, 0.0f, tr("dB"), ampPanel);
    volumeKnob_ = new KnobWidget(tr("Volume"), -60.0f, 6.0f, 0.0f, tr("dB"), ampPanel);

    powerBtn_ = new QPushButton(ampPanel);
    powerBtn_->setObjectName(QStringLiteral("powerBtn"));
    powerBtn_->setCheckable(true);
    powerBtn_->setFixedSize(46, 46);
    powerBtn_->setAccessibleName(tr("Power"));
    powerBtn_->setToolTip(tr("Power on/off"));

    auto* powerLabel = new QLabel(tr("ON"), ampPanel);
    powerLabel->setObjectName(QStringLiteral("powerLabel"));

    ampLayout->addWidget(logo);
    ampLayout->addStretch();
    ampLayout->addWidget(gainKnob_);
    ampLayout->addWidget(volumeKnob_);
    ampLayout->addStretch();
    ampLayout->addWidget(powerLabel);
    ampLayout->addWidget(powerBtn_);

    // --- Status line --------------------------------------------------------
    statusLabel_ = new QLabel(central);
    statusLabel_->setObjectName(QStringLiteral("statusLabel"));
    statusLabel_->setWordWrap(true);

    layout->addWidget(toolbar);
    layout->addWidget(board, 1);
    layout->addWidget(ampPanel);
    layout->addWidget(statusLabel_);

    setCentralWidget(central);
}

void MainWindow::connectSignals()
{
    connect(powerBtn_, &QPushButton::toggled, this, &MainWindow::onPowerToggled);
    connect(deviceSelector_, qOverload<int>(&QComboBox::activated),
            this, &MainWindow::onDeviceSelected);
    connect(bufferSizeSelector_, qOverload<int>(&QComboBox::activated),
            this, &MainWindow::onBufferSizeSelected);

    connect(gainKnob_, &KnobWidget::valueChanged,
            this, [this](float db) { audioEngine_->setGainDb(db); });
    connect(volumeKnob_, &KnobWidget::valueChanged,
            this, [this](float db) { audioEngine_->setVolumeDb(db); });

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
        showStatus(tr("No audio devices found. Connect an interface and power on again."), true);
}

void MainWindow::updateMeters()
{
    // Lock-free reads of the audio thread's atomics — never blocks audio.
    inputMeter_->setLevels(audioEngine_->getInputLevelL(),
                           audioEngine_->getInputLevelR());
    outputMeter_->setLevels(audioEngine_->getOutputLevelL(),
                            audioEngine_->getOutputLevelR());

    const double rate = audioEngine_->getCurrentSampleRate();
    sampleRateLabel_->setText(rate > 0.0 ? tr("%1 kHz").arg(rate / 1000.0, 0, 'f', 1)
                                         : tr("— Hz"));

    const float latency = audioEngine_->getMeasuredLatency();
    latencyLabel_->setText(latency > 0.0f ? tr("%1 ms").arg(latency, 0, 'f', 1)
                                          : tr("— ms"));

    // Hot-plug: refresh the device list when the OS reports a change.
    if (audioEngine_->consumeDeviceListChanged())
        refreshDeviceList();
}

void MainWindow::onPowerToggled(bool on)
{
    if (!on)
    {
        audioEngine_->stop();
        showStatus(tr("Powered off."));
        return;
    }

    if (audioEngine_->start())
    {
        showStatus(tr("Running on: %1")
                       .arg(QString::fromStdString(audioEngine_->getCurrentDeviceName())));
        refreshDeviceList();
    }
    else
    {
        const QSignalBlocker blocker(powerBtn_);
        powerBtn_->setChecked(false);
        showStatus(QString::fromStdString(audioEngine_->getLastError()), true);
    }
}

void MainWindow::onDeviceSelected(int index)
{
    const QString name = deviceSelector_->itemText(index);
    if (audioEngine_->setAudioDevice(name.toStdString()))
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
    statusLabel_->setProperty("error", isError);
    statusLabel_->style()->unpolish(statusLabel_);
    statusLabel_->style()->polish(statusLabel_);
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
