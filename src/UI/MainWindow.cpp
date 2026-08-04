#include "MainWindow.h"

#include <QCloseEvent>
#include <QComboBox>
#include <QFrame>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QSettings>
#include <QSignalBlocker>
#include <QStatusBar>
#include <QTimer>
#include <QVBoxLayout>

#include "Audio/AudioEngine.h"
#include "Shared/Constants.h"
#include "UI/Panels/PresetPanel.h"
#include "UI/Sections/AmpSection.h"
#include "UI/Sections/CabinetSection.h"
#include "UI/Sections/EffectsSection.h"
#include "UI/Sections/MasterSection.h"

namespace
{
    constexpr int kStatusRefreshMs = 500;
}

MainWindow::MainWindow(AudioEngine* engine, QWidget* parent)
    : QMainWindow(parent),
      audioEngine_(engine),
      ampSection_(nullptr),
      cabinetSection_(nullptr),
      effectsSection_(nullptr),
      masterSection_(nullptr),
      presetPanel_(nullptr),
      deviceSelector_(nullptr),
      bufferSizeSelector_(nullptr),
      powerBtn_(nullptr),
      deviceStatusLabel_(nullptr),
      sampleRateLabel_(nullptr),
      latencyLabel_(nullptr),
      cpuLabel_(nullptr),
      meterTimer_(nullptr),
      statusTimer_(nullptr)
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
    resize(1100, 760);
    setMinimumSize(820, 600);

    // --- Top bar: branding + device controls + power -------------------------
    auto* topBar = new QFrame(this);
    topBar->setObjectName(QStringLiteral("topBar"));
    auto* topLayout = new QHBoxLayout(topBar);
    topLayout->setContentsMargins(14, 6, 14, 6);
    topLayout->setSpacing(8);

    auto* brand = new QLabel(tr("Desktop <b>Amp</b> Simulator"), topBar);
    brand->setObjectName(QStringLiteral("brand"));

    deviceSelector_ = new QComboBox(topBar);
    deviceSelector_->setAccessibleName(tr("Audio device"));
    deviceSelector_->setMinimumWidth(200);

    bufferSizeSelector_ = new QComboBox(topBar);
    bufferSizeSelector_->setAccessibleName(tr("Buffer size"));
    for (int size : ampsim::kBufferSizes)
        bufferSizeSelector_->addItem(tr("%1 smp").arg(size), size);
    bufferSizeSelector_->setCurrentIndex(2);   // 256 by default

    powerBtn_ = new QPushButton(topBar);
    powerBtn_->setObjectName(QStringLiteral("powerBtn"));
    powerBtn_->setCheckable(true);
    powerBtn_->setFixedSize(38, 38);
    powerBtn_->setAccessibleName(tr("Power"));
    powerBtn_->setToolTip(tr("Power on/off"));

    topLayout->addWidget(brand);
    topLayout->addStretch();
    topLayout->addWidget(new QLabel(tr("Device"), topBar));
    topLayout->addWidget(deviceSelector_);
    topLayout->addWidget(new QLabel(tr("Buffer"), topBar));
    topLayout->addWidget(bufferSizeSelector_);
    topLayout->addWidget(powerBtn_);

    // --- Section stack in a vertical scroll area -----------------------------
    ampSection_ = new AmpSection(audioEngine_);
    cabinetSection_ = new CabinetSection();
    effectsSection_ = new EffectsSection(audioEngine_);
    masterSection_ = new MasterSection(audioEngine_);

    auto* sections = new QWidget();
    auto* sectionLayout = new QVBoxLayout(sections);
    sectionLayout->setContentsMargins(10, 8, 10, 8);
    sectionLayout->setSpacing(10);
    sectionLayout->addWidget(ampSection_);
    sectionLayout->addWidget(cabinetSection_);
    sectionLayout->addWidget(effectsSection_);
    sectionLayout->addWidget(masterSection_);
    sectionLayout->addStretch();

    auto* scroll = new QScrollArea(this);
    scroll->setObjectName(QStringLiteral("sectionScroll"));
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    scroll->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    scroll->setWidget(sections);

    auto* central = new QWidget(this);
    central->setObjectName(QStringLiteral("studioBackground"));
    auto* centralLayout = new QVBoxLayout(central);
    centralLayout->setContentsMargins(0, 0, 0, 0);
    centralLayout->setSpacing(0);
    centralLayout->addWidget(topBar);
    centralLayout->addWidget(scroll, 1);
    setCentralWidget(central);

    // --- Preset dock ---------------------------------------------------------
    presetPanel_ = new PresetPanel([this] { return captureState(); },
                                   [this](const QVariantMap& s) { applyState(s); },
                                   this);
    addDockWidget(Qt::RightDockWidgetArea, presetPanel_);

    // --- Status bar ----------------------------------------------------------
    deviceStatusLabel_ = new QLabel(tr("No device"), this);
    sampleRateLabel_ = new QLabel(tr("— Hz"), this);
    latencyLabel_ = new QLabel(tr("— ms"), this);
    cpuLabel_ = new QLabel(tr("CPU —"), this);
    for (auto* label : { deviceStatusLabel_, sampleRateLabel_, latencyLabel_, cpuLabel_ })
        label->setObjectName(QStringLiteral("statusReadout"));

    statusBar()->addPermanentWidget(deviceStatusLabel_);
    statusBar()->addPermanentWidget(sampleRateLabel_);
    statusBar()->addPermanentWidget(latencyLabel_);
    statusBar()->addPermanentWidget(cpuLabel_);
}

void MainWindow::connectSignals()
{
    connect(powerBtn_, &QPushButton::toggled, this, &MainWindow::onPowerToggled);
    connect(deviceSelector_, qOverload<int>(&QComboBox::activated),
            this, &MainWindow::onDeviceSelected);
    connect(bufferSizeSelector_, qOverload<int>(&QComboBox::activated),
            this, &MainWindow::onBufferSizeSelected);
    connect(presetPanel_, &PresetPanel::statusMessage,
            this, [this](const QString& message) { showStatus(message); });

    meterTimer_ = new QTimer(this);
    meterTimer_->setInterval(ampsim::kUiRefreshMs);
    connect(meterTimer_, &QTimer::timeout, this, &MainWindow::updateMeters);
    meterTimer_->start();

    statusTimer_ = new QTimer(this);
    statusTimer_->setInterval(kStatusRefreshMs);
    connect(statusTimer_, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    statusTimer_->start();
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
        showStatus(tr("No audio devices found. Connect an interface and power on again."));
}

void MainWindow::updateMeters()
{
    // Lock-free reads of the audio thread's atomics — never blocks audio.
    masterSection_->updateMeters();

    // Hot-plug: refresh the device list when the OS reports a change.
    if (audioEngine_->consumeDeviceListChanged())
        refreshDeviceList();
}

void MainWindow::updateStatusBar()
{
    const QString device = QString::fromStdString(audioEngine_->getCurrentDeviceName());
    deviceStatusLabel_->setText(device.isEmpty() ? tr("No device") : device);

    const double rate = audioEngine_->getCurrentSampleRate();
    sampleRateLabel_->setText(rate > 0.0 ? tr("%1 kHz").arg(rate / 1000.0, 0, 'f', 1)
                                         : tr("— Hz"));

    const float latency = audioEngine_->getMeasuredLatency();
    latencyLabel_->setText(latency > 0.0f ? tr("%1 ms").arg(latency, 0, 'f', 1)
                                          : tr("— ms"));

    cpuLabel_->setText(audioEngine_->isRunning()
                           ? tr("CPU %1%").arg(audioEngine_->getCpuUsage() * 100.0, 0, 'f', 1)
                           : tr("CPU —"));
}

QVariantMap MainWindow::captureState() const
{
    return {
        { QStringLiteral("amp"), ampSection_->captureState() },
        { QStringLiteral("cabinet"), cabinetSection_->captureState() },
        { QStringLiteral("effects"), effectsSection_->captureState() },
        { QStringLiteral("master"), masterSection_->captureState() },
    };
}

void MainWindow::applyState(const QVariantMap& state)
{
    ampSection_->applyState(state.value(QStringLiteral("amp")).toMap());
    cabinetSection_->applyState(state.value(QStringLiteral("cabinet")).toMap());
    effectsSection_->applyState(state.value(QStringLiteral("effects")).toMap());
    masterSection_->applyState(state.value(QStringLiteral("master")).toMap());
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
        showStatus(QString::fromStdString(audioEngine_->getLastError()));
    }
}

void MainWindow::onDeviceSelected(int index)
{
    const QString name = deviceSelector_->itemText(index);
    if (audioEngine_->setAudioDevice(name.toStdString()))
        showStatus(tr("Switched to: %1").arg(name));
    else
        showStatus(QString::fromStdString(audioEngine_->getLastError()));
}

void MainWindow::onBufferSizeSelected(int index)
{
    const int size = bufferSizeSelector_->itemData(index).toInt();
    if (audioEngine_->setBufferSize(size))
        showStatus(tr("Buffer size set to %1 samples.").arg(size));
    else
        showStatus(QString::fromStdString(audioEngine_->getLastError()));
}

void MainWindow::showStatus(const QString& message)
{
    statusBar()->showMessage(message, 5000);
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
