#include "dbusadaptor.h"

#include <QCoreApplication>
#include <QDebug>
#include <QDBusConnection>
#include <QDBusError>
#include <QDBusMetaType>
#include <QLoggingCategory>

static const QString s_dbusObject = QStringLiteral("/org/coderus/screenrecorder");
static const QString s_dbusService = QStringLiteral("org.coderus.screenrecorder");
static const QString s_dbusInterface = QStringLiteral("org.coderus.screenrecorder");

Q_LOGGING_CATEGORY(logadaptor, "screenrecorder.adaptor", QtDebugMsg)

DBusAdaptor::DBusAdaptor(QObject *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(true);

    connect(Recorder::instance(), &Recorder::statusChanged, this, &DBusAdaptor::StateChanged);
}

DBusAdaptor::~DBusAdaptor()
{
    unregisterService();
}

int DBusAdaptor::State() const
{
    return Recorder::instance()->status();
}

QString DBusAdaptor::Destination() const
{
    return Recorder::instance()->m_options.destination;
}

void DBusAdaptor::SetDestination(const QString &destination)
{
    Recorder::instance()->m_options.destination = destination;
    qCDebug(logadaptor) << Q_FUNC_INFO << destination;
}

int DBusAdaptor::Fps() const
{
    return Recorder::instance()->m_options.fps;
}

void DBusAdaptor::SetFps(int fps)
{
    Recorder::instance()->m_options.fps = fps;
    qCDebug(logadaptor) << Q_FUNC_INFO << fps;
}

int DBusAdaptor::Buffers() const
{
    return Recorder::instance()->m_options.buffers;
}

void DBusAdaptor::SetBuffers(int buffers)
{
    Recorder::instance()->m_options.buffers = buffers;
    qCDebug(logadaptor) << Q_FUNC_INFO << buffers;
}

bool DBusAdaptor::FullMode() const
{
    return Recorder::instance()->m_options.fullMode;
}

void DBusAdaptor::SetFullMode(bool fullMode)
{
    Recorder::instance()->m_options.fullMode = fullMode;
    qCDebug(logadaptor) << Q_FUNC_INFO << fullMode;
}

double DBusAdaptor::Scale() const
{
    return Recorder::instance()->m_options.scale;
}

void DBusAdaptor::SetScale(double scale)
{
    Recorder::instance()->m_options.scale = scale;
    qCDebug(logadaptor) << Q_FUNC_INFO << scale;
}

int DBusAdaptor::Quality() const
{
    return Recorder::instance()->m_options.quality;
}

void DBusAdaptor::SetQuality(int quality)
{
    Recorder::instance()->m_options.quality = quality;
    qCDebug(logadaptor) << Q_FUNC_INFO << quality;
}

bool DBusAdaptor::Smooth() const
{
    return Recorder::instance()->m_options.smooth;
}

void DBusAdaptor::SetSmooth(bool smooth)
{
    Recorder::instance()->m_options.smooth = smooth;
    qCDebug(logadaptor) << Q_FUNC_INFO << smooth;
}

bool DBusAdaptor::registerService()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    const bool registerObjectSuccess = bus.registerObject(s_dbusObject, s_dbusInterface, qApp);
    qCDebug(logadaptor) << Q_FUNC_INFO << "Object registered:" << registerObjectSuccess;
    if (!registerObjectSuccess) {
        qCWarning(logadaptor) << Q_FUNC_INFO << bus.lastError().message();
        return false;
    }
    const bool registerServiceSuccess = bus.registerService(s_dbusService);
    qCDebug(logadaptor) << Q_FUNC_INFO << "Service registered:" << registerServiceSuccess;
    if (!registerServiceSuccess) {
        qCWarning(logadaptor) << Q_FUNC_INFO << bus.lastError().message();
        return false;
    }
    return true;
}

void DBusAdaptor::unregisterService()
{
    QDBusConnection bus = QDBusConnection::systemBus();
    const bool unregisterServiceSuccess = bus.unregisterService(s_dbusService);
    qCDebug(logadaptor) << Q_FUNC_INFO << "Service unregistered:" << unregisterServiceSuccess;
    if (!unregisterServiceSuccess) {
        qCWarning(logadaptor) << Q_FUNC_INFO << bus.lastError().message();
        return;
    }
    bus.unregisterObject(s_dbusObject);
}

void DBusAdaptor::Quit()
{
    qCDebug(logadaptor) << Q_FUNC_INFO;
    Recorder::instance()->handleShutDown();
}

void DBusAdaptor::Start()
{
    qCDebug(logadaptor) << Q_FUNC_INFO ;
    Recorder::instance()->start();
}

void DBusAdaptor::Stop()
{
    const QString filename = Recorder::instance()->stop();
    emit RecordingFinished(filename);
}
