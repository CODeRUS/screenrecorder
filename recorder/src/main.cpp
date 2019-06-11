#include "recorder.h"

#include <QCommandLineParser>
#include <QGuiApplication>

#include <QDebug>
#include <QLoggingCategory>
#include <QTimer>

#include <signal.h>

Q_LOGGING_CATEGORY(logmain, "screenrecorder.main", QtDebugMsg)

static Recorder *recorder = nullptr;

void handleShutDownSignal(int)
{
    if (!recorder) {
        exit(0);
    }
    recorder->handleShutDown();
}

void setShutDownSignal(int signalId)
{
    struct sigaction sa;
    sa.sa_flags = 0;
    sigemptyset(&sa.sa_mask);
    sa.sa_handler = handleShutDownSignal;
    if (sigaction(signalId, &sa, NULL) == -1) {
        perror("setting up termination signal");
        exit(1);
    }
}

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    app.setApplicationVersion(QStringLiteral(PROJECT_PACKAGE_VERSION));

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Screen recorder"));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(QStringLiteral("destination"),
                                 app.translate("main", "Destination file. Example: record.avi."));
    QCommandLineOption framerateOption(
            QStringLiteral("fps"),
            app.translate("main", "Framerate. Default is 24."),
            app.translate("main", "framerate"),
            QStringLiteral("24"));
    parser.addOption(framerateOption);

    QCommandLineOption buffersOption(
            QStringLiteral("buffers"),
            app.translate("main", "Amount of buffers to store received frames. Default is framerate * 2."),
            app.translate("main", "buffers"));
    parser.addOption(buffersOption);

    QCommandLineOption scaleOption(
            QStringLiteral("scale"),
            app.translate("main", "Scale frames ratio."),
            app.translate("main", "scale"));
    parser.addOption(scaleOption);

    QCommandLineOption smoothOption(
            {QStringLiteral("s"), QStringLiteral("smooth")},
            app.translate("main", "Scale frames using SmoothTransformation."));
    parser.addOption(smoothOption);

    QCommandLineOption qualityOption(
            QStringLiteral("quality"),
            app.translate("main", "Frame JPEG compression quality."),
            app.translate("main", "quality"));
    parser.addOption(qualityOption);

    QCommandLineOption daemonOption(
            {QStringLiteral("d"), QStringLiteral("daemon")},
            app.translate("main", "Daemonize recorder. Will create D-Bus service org.coderus.screenrecorder on system bus."));
    parser.addOption(daemonOption);

    QCommandLineOption fullOption(
            {QStringLiteral("f"), QStringLiteral("full")},
            app.translate("main", "Write full frames. Including frames when idle. By default only changed frames are recorded."));
    parser.addOption(fullOption);

    parser.process(app);

    const QStringList args = parser.positionalArguments();
    if (args.count() == 0) {
        parser.showHelp();
    }

    const QString dest = args.first();

    int fps = 24;
    if (parser.isSet(framerateOption)) {
        fps = parser.value(framerateOption).toInt();
    }
    int buffers = fps * 2;
    if (parser.isSet(buffersOption)) {
        buffers = parser.value(buffersOption).toInt();
    }
    float scale = 1.0f;
    if (parser.isSet(scaleOption)) {
        scale = parser.value(scaleOption).toFloat();
    }
    int quality = 100;
    if (parser.isSet(qualityOption)) {
        quality = parser.value(qualityOption).toInt();
    }
    const bool smooth = parser.isSet(fullOption);
    if (parser.isSet(daemonOption)) {
        qCDebug(logmain) << "Daemonize";
    } else {
        setShutDownSignal(SIGINT); // shut down on ctrl-c
        setShutDownSignal(SIGTERM); // shut down on killall
    }

    const bool fullMode = parser.isSet(fullOption);
    recorder = new Recorder({
                                dest,
                                fps,
                                buffers,
                                fullMode,
                                scale,
                                quality,
                                smooth,
                            }, qGuiApp);
    QTimer::singleShot(0, recorder, &Recorder::init);

    return app.exec();
}
