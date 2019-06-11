#include "recorder.h"

#include <QCommandLineParser>
#include <QGuiApplication>

#include <QDateTime>
#include <QDebug>
#include <QLoggingCategory>
#include <QStandardPaths>
#include <QTimer>

#include <signal.h>

#include "dbusadaptor.h"

Q_LOGGING_CATEGORY(logmain, "screenrecorder.main", QtDebugMsg)

void initializeDBus()
{
    DBusAdaptor *adaptor = new DBusAdaptor(qApp);
    if (!adaptor->registerService()) {
        qApp->quit();
    }
    Recorder::instance()->init();
}

void handleShutDownSignal(int)
{
    Recorder::instance()->handleShutDown();
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
    const bool daemonize = parser.isSet(daemonOption);
    if (daemonize) {
        qCDebug(logmain) << "Daemonize";
    } else {
        setShutDownSignal(SIGINT); // shut down on ctrl-c
        setShutDownSignal(SIGTERM); // shut down on killall
    }

    const QStringList args = parser.positionalArguments();
    if (args.count() == 0 && !daemonize) {
        parser.showHelp();
    }

    QString dest = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    if (args.count() > 0) {
        dest = args.first();
    }

    const bool fullMode = parser.isSet(fullOption);
    Recorder *recorder = new Recorder({
                                dest,
                                fps,
                                buffers,
                                fullMode,
                                scale,
                                quality,
                                smooth,
                                daemonize,
                            }, qGuiApp);

    if (daemonize) {
        QTimer::singleShot(0, &initializeDBus);
    } else {
        QTimer::singleShot(0, recorder, &Recorder::init);
    }

    return app.exec();
}
