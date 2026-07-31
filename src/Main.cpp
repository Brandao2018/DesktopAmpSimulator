#include <QApplication>
#include <QFile>
#include <QTimer>

#include <juce_events/juce_events.h>

#include "Audio/AudioEngine.h"
#include "Shared/Logger.h"
#include "UI/MainWindow.h"

// Qt owns the process event loop. JUCE still needs its MessageManager for
// device-change notifications, so we create it on this (main) thread and
// pump it briefly from a Qt timer. The audio callback itself runs on the
// device's own real-time thread and needs neither loop.

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("DesktopAmpSimulator"));
    QApplication::setOrganizationName(QStringLiteral("AmpSim"));

    // Dark theme (default).
    QFile style(QStringLiteral(":/StyleSheet.css"));
    if (style.open(QIODevice::ReadOnly | QIODevice::Text))
        app.setStyleSheet(QString::fromUtf8(style.readAll()));

    ampsim::Logger::info("DesktopAmpSimulator starting");

    int exitCode = 0;
    {
        const juce::ScopedJuceInitialiser_GUI juceInit;

        AudioEngine engine;
        MainWindow window(&engine);
        window.show();

        // Pump the JUCE message queue from the Qt event loop.
        QTimer jucePump;
        QObject::connect(&jucePump, &QTimer::timeout, [] {
            if (auto* mm = juce::MessageManager::getInstanceWithoutCreating())
                mm->runDispatchLoopUntil(1);
        });
        jucePump.start(10);

        exitCode = QApplication::exec();
        // engine/window destruct here, before ScopedJuceInitialiser_GUI.
    }

    ampsim::Logger::info("DesktopAmpSimulator exiting");
    return exitCode;
}
