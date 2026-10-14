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

    Q_PROPERTY(int State READ GetState NOTIFY StateChanged)
    Q_PROPERTY(QString Destination READ GetDestination WRITE SetDestination FINAL)
    Q_PROPERTY(int Fps READ GetFps WRITE SetFps FINAL)
    Q_PROPERTY(int Buffers READ GetBuffers WRITE SetBuffers FINAL)
    Q_PROPERTY(bool FullMode READ GetFullMode WRITE SetFullMode FINAL)
    Q_PROPERTY(double Scale READ GetScale WRITE SetScale FINAL)
    Q_PROPERTY(int Quality READ GetQuality WRITE SetQuality FINAL)
    Q_PROPERTY(bool Smooth READ GetSmooth WRITE SetSmooth FINAL)
    Q_PROPERTY(bool Convert READ GetConvert WRITE SetConvert FINAL)

public slots:
    Q_NOREPLY void Quit();
    void Start();
    Q_NOREPLY void Stop();

    int GetState() const;

    QString GetDestination() const;
    void SetDestination(const QString &destination);

    int GetFps() const;
    void SetFps(int fps);

    int GetBuffers() const;
    void SetBuffers(int buffers);

    bool GetFullMode() const;
    void SetFullMode(bool fullMode);

    double GetScale() const;
    void SetScale(double scale);

    int GetQuality() const;
    void SetQuality(int quality);

    bool GetSmooth() const;
    void SetSmooth(bool smooth);

    bool GetConvert() const;
    void SetConvert(bool convert);

signals:
    void StateChanged(int state);
    void RecordingFinished(const QString &fileName);

};

#endif // DBUSADAPTOR_H
