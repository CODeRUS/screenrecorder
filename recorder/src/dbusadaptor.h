#ifndef DBUSADAPTOR_H
#define DBUSADAPTOR_H

#include <QDBusAbstractAdaptor>
#include "recorder.h"

class DBusAdaptor : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.coderus.screenrecorder")
public:
    explicit DBusAdaptor(QObject *parent = nullptr);
    virtual ~DBusAdaptor();

    bool registerService();
    void unregisterService();

    Q_PROPERTY(int State READ State NOTIFY StateChanged)
    Q_PROPERTY(QString Destination READ Destination WRITE SetDestination FINAL)
    Q_PROPERTY(int Fps READ Fps WRITE SetFps FINAL)
    Q_PROPERTY(int Buffers READ Buffers WRITE SetBuffers FINAL)
    Q_PROPERTY(bool FullMode READ FullMode WRITE SetFullMode FINAL)
    Q_PROPERTY(double Scale READ Scale WRITE SetScale FINAL)
    Q_PROPERTY(int Quality READ Quality WRITE SetQuality FINAL)
    Q_PROPERTY(bool Smooth READ Smooth WRITE SetSmooth FINAL)

public slots:
    Q_NOREPLY void Quit();
    void Start();
    Q_NOREPLY void Stop();

    int State() const;

    QString Destination() const;
    void SetDestination(const QString &destination);

    int Fps() const;
    void SetFps(int fps);

    int Buffers() const;
    void SetBuffers(int buffers);

    bool FullMode() const;
    void SetFullMode(bool fullMode);

    double Scale() const;
    void SetScale(double scale);

    int Quality() const;
    void SetQuality(int quality);

    bool Smooth() const;
    void SetSmooth(bool smooth);

signals:
    void StateChanged(int state);
    void RecordingFinished(const QString &fileName);

};

#endif // DBUSADAPTOR_H
