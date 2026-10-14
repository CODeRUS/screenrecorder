/***************************************************************************
**
** Copyright (C) 2014 Jolla Ltd.
** Contact: Giulio Camuffo <giulio.camuffo@jollamobile.com>
**
** This file is part of lipstick-recorder.
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation
** and appearing in the file LICENSE.LGPL included in the packaging
** of this file.
**
****************************************************************************/

#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include <QGuiApplication>
#include <QScreen>
#include <qpa/qplatformnativeinterface.h>
#include <QDebug>
#include <QThread>
#include <QMutexLocker>
#include <QElapsedTimer>
#include <QTimer>
#include <QThreadPool>
#include <QtConcurrent>
#include <QLoggingCategory>
#include <QDateTime>

#include <MDConfGroup>

#include "wayland-lipstick-recorder-client-protocol.h"
#include "recorder.h"

#include "QAviWriter.h"

Q_LOGGING_CATEGORY(logrecorder, "screenrecorder.recorder", QtDebugMsg)
Q_LOGGING_CATEGORY(logbuffer, "screenrecorder.recorder.buffer", QtDebugMsg)

static Recorder *s_instance = nullptr;

class Buffer
{
public:
    static Buffer *create(wl_shm *shm, int width, int height, int stride, int format)
    {
        int size = stride * height;

        char filename[] = "/tmp/lipstick-recorder-shm-XXXXXX";
        int fd = mkstemp(filename);
        if (fd < 0) {
            qCWarning(logbuffer) << "creating a buffer file for" << size << "B failed";
            return nullptr;
        }
        int flags = fcntl(fd, F_GETFD);
        if (flags != -1)
            fcntl(fd, F_SETFD, flags | FD_CLOEXEC);

        if (ftruncate(fd, size) < 0) {
            qCWarning(logbuffer) << "ftruncate failed:" << strerror(errno);
            close(fd);
            return nullptr;
        }

        uchar *data = (uchar *)mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        unlink(filename);
        if (data == (uchar *)MAP_FAILED) {
            qCWarning(logbuffer) << "mmap failed";
            close(fd);
            return nullptr;
        }

        Buffer *buf = new Buffer;

        wl_shm_pool *pool = wl_shm_create_pool(shm, fd, size);
        buf->buffer = wl_shm_pool_create_buffer(pool, 0, width, height, stride, WL_SHM_FORMAT_ARGB8888);
        wl_buffer_set_user_data(buf->buffer, buf);
        wl_shm_pool_destroy(pool);
        buf->data = data;
        buf->image = QImage(data, width, height, stride, QImage::Format_RGBA8888);
        close(fd);
        return buf;
    }

    wl_buffer *buffer;
    uchar *data;
    QImage image;
    QPixmap pixmap;
    bool busy = false;
};

Recorder::Recorder(const Options &options, QObject *parent)
    : QObject(parent)
    , m_avi(new QAviWriter(QStringLiteral("MJPG"), this))
    , m_options(options)
    , m_pool(new QThreadPool(this))
    , m_timer(new QTimer(this))
{
    qCDebug(logrecorder) << "Writing to" << options.destination;
    qCDebug(logrecorder) << "Fps:" << options.fps;
    qCDebug(logrecorder) << "Buffers:" << options.buffers;
    qCDebug(logrecorder) << "Scale:" << options.scale;
    qCDebug(logrecorder) << "Smooth:" << options.smooth;
    qCDebug(logrecorder) << "Quality:" << options.quality;
    if (options.fullMode) {
        qCDebug(logrecorder) << "Writing full fps frames.";
    } else {
        qCDebug(logrecorder) << "Writing only changed frames.";
    }

    m_screen = QGuiApplication::screens().first();

    m_pool->setMaxThreadCount(1);

    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setInterval(1000 / m_options.fps);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &Recorder::saveFrame);

    s_instance = this;
}

Recorder *Recorder::instance()
{
    return s_instance;
}

Recorder::~Recorder()
{
}

Recorder::Status Recorder::status() const
{
    return m_status;
}

void Recorder::setStatus(Recorder::Status status)
{
    qCDebug(logrecorder) << Q_FUNC_INFO << status;
    m_status = status;
    emit statusChanged(m_status);
}

Recorder::Options Recorder::readOptions()
{
    MDConfGroup dconf(QStringLiteral("/org/coderus/screenrecorder"));
    return {
        dconf.value(QStringLiteral("destination"), QStandardPaths::writableLocation(QStandardPaths::MoviesLocation)).toString(),
        dconf.value(QStringLiteral("fps"), 24).toInt(),
        dconf.value(QStringLiteral("buffers"), 48).toInt(),
        dconf.value(QStringLiteral("fullmode"), false).toBool(),
        dconf.value(QStringLiteral("scale"), 1.0f).toDouble(),
        dconf.value(QStringLiteral("quality"), 100).toInt(),
        dconf.value(QStringLiteral("smooth"), false).toBool(),
        dconf.value(QStringLiteral("convert"), true).toBool(),
        false,
    };
}

void Recorder::init()
{
    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    m_display = static_cast<wl_display *>(native->nativeResourceForIntegration("display"));
    m_registry = wl_display_get_registry(m_display);

    static const wl_registry_listener registryListener = {
        global,
        globalRemove
    };
    wl_registry_add_listener(m_registry, &registryListener, this);

    wl_callback *cb = wl_display_sync(m_display);
    static const wl_callback_listener callbackListener = {
        callback
    };
    wl_callback_add_listener(cb, &callbackListener, this);
}

void Recorder::start()
{
    if (!m_manager)
        qFatal("The lipstick_recorder_manager global is not available.");

    if (m_status != StatusReady) {
        qCWarning(logrecorder) << Q_FUNC_INFO << "Recorder not ready or busy!";
        return;
    }

    m_size = m_screen->size();
    m_size.setWidth(qRound(m_size.width() * m_options.scale));
    m_size.setHeight(qRound(m_size.height() * m_options.scale));


    const QString dateString = QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    const QString filename = QStringLiteral("/screenrecorder-%1.avi").arg(dateString);

    m_avi->setFileName(m_options.destination + filename);
    m_avi->setFps(m_options.fps);
    m_avi->setSize(m_size);
    m_avi->open();

    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    wl_output *output = static_cast<wl_output *>(native->nativeResourceForScreen(QByteArrayLiteral("output"), m_screen));
    m_recorder = lipstick_recorder_manager_create_recorder(m_manager, output);
    static const lipstick_recorder_listener recorderListener = {
        setup,
        frame,
        failed,
        cancel
    };
    lipstick_recorder_add_listener(m_recorder, &recorderListener, this);

    setStatus(StatusRecording);
}

QString Recorder::stop()
{
    if (m_status != StatusRecording) {
        qCWarning(logrecorder) << Q_FUNC_INFO << "Not recording!";
        return QString();
    }

    setStatus(StatusSaving);

    qCDebug(logrecorder) << "Saving frames, please wait!";
    m_pool->waitForDone();
    m_avi->close();

    // Convert the file to MP4 if requested

    QString file(m_avi->fileName());

    // this should not be neccessary, we have a bug in options init?
    MDConfGroup dconf(QStringLiteral("/org/coderus/screenrecorder"));
    bool conv = dconf.value(QStringLiteral("convert"), true).toBool();
    if (conv) {
        file = convert(m_avi->fileName());
    }
    else {
        qCDebug(logrecorder) << "No converstion, leaving as AVI";
    }

    setStatus(StatusReady);

    lipstick_recorder_destroy(m_recorder);

    return file;
}

QString Recorder::convert(const QString & filein)
{
    setStatus(StatusConverting);
    qCDebug(logrecorder) << "Converting to MP4";


    ///usr/bin/ffmpeg,-progress,-,-nostats,-hide_banner,-y,-i,
    //home/defaultuser/Videos/screenrecorder-20261010_181758.avi,
    //-c:v,vp9,-filter:v,tblend,scale=iw/2:ih/2,-c:a,copy,-r,10,
    //home/defaultuser/Videos/screenrecorder-20261010_181758_edit.mkv

    QString fileout(filein);
    fileout.truncate(filein.lastIndexOf(QStringLiteral(".")));
    fileout.append(QStringLiteral(".mp4"));
    // mark's vp9
    /*
    QStringList arguments = {QStringLiteral("-y"),
                             QStringLiteral("-i"),
                             m_avi->fileName(),
                             QStringLiteral("-c:v"),
                             QStringLiteral("vp9"),
                             QStringLiteral("-crf"),
                             QStringLiteral("28"),
                             QStringLiteral("-b:v"),
                             QStringLiteral("0"),
                             QStringLiteral("-row-mt"),
                             QStringLiteral("1"),
                             QStringLiteral("-an"),
                             fileout};
    */
    /* david's original*/
    QStringList arguments = {QStringLiteral("-i"),
                             m_avi->fileName(),
                             QStringLiteral("-y"),
                             QStringLiteral("-q:v"),
                             QStringLiteral("2"),
                             QStringLiteral("-pix_fmt"),
                             QStringLiteral("yuv420p"),
                             QStringLiteral("-preset"),
                             QStringLiteral("slow"),
                             QStringLiteral("-movflags"),
                             QStringLiteral("+faststart"),
                             fileout};
    int result = QProcess::execute(QStringLiteral("/usr/bin/ffmpeg"), arguments);
    if (result == 0) {
        printf("wrote");
        qCDebug(logrecorder) << "Converted as " << fileout;
        QFile::remove(filein);
    }
    else {
        printf("failed");
        qCDebug(logrecorder) << "Converted failed with code: " << result;
        fileout = filein;
    }

    return fileout;
}

void Recorder::handleShutDown()
{
    if (m_shutdown) {
        return;
    }
    m_shutdown = true;

    qCDebug(logrecorder) << "File saved to:" << stop();
    qGuiApp->sendEvent(qGuiApp, new QEvent(QEvent::Quit));
}

void Recorder::recordFrame()
{
    Buffer *buf = nullptr;
    for (Buffer *b : m_buffers) {
        if (!b->busy) {
            buf = b;
            break;
        }
    }
    if (buf) {
        lipstick_recorder_record_frame(m_recorder, buf->buffer);
        wl_display_flush(m_display);
        buf->busy = true;
        m_starving = false;
    } else {
        qCWarning(logrecorder) << "No free buffers.";
        m_starving = true;
    }
}

void Recorder::saveFrame()
{
    if (!m_lastFrame) {
        return;
    }
    if (m_shutdown) {
        return;
    }
    QAviWriter *avi = m_avi;
    QPixmap pix = m_lastFrame->pixmap;
    int quality = m_options.quality;
    QtConcurrent::run(m_pool, [avi, pix, quality] {
        avi->addFrame(pix, "JPG", quality);
    });
    if (m_starving)
        recordFrame();
    m_timer->start();
}

void Recorder::callback(void *data, wl_callback *cb, uint32_t time)
{
    Q_UNUSED(time)
    wl_callback_destroy(cb);

    Recorder *rec = static_cast<Recorder *>(data);
    QMutexLocker lock(&rec->m_mutex);

    rec->setStatus(StatusReady);

    if (!rec->m_options.daemonize) {
        QTimer::singleShot(0, rec, &Recorder::start);
    }
}

void Recorder::setup(void *data, lipstick_recorder *recorder, int width, int height, int stride, int format)
{
    Recorder *rec = static_cast<Recorder *>(data);
    QMutexLocker lock(&rec->m_mutex);

    for (int i = 0; i < rec->m_options.buffers; ++i) {
        Buffer *buffer = Buffer::create(rec->m_shm, width, height, stride, format);
        if (!buffer)
            qFatal("Failed to create a buffer.");
        rec->m_buffers << buffer;
    }
    rec->recordFrame();
}

void Recorder::frame(void *data, lipstick_recorder *recorder, wl_buffer *buffer, uint32_t timestamp, int transform)
{
    Q_UNUSED(recorder)

    Recorder *rec = static_cast<Recorder *>(data);
    if (rec->m_shutdown) {
        return;
    }

    rec->recordFrame();
    static uint32_t time = 0;

    QMutexLocker lock(&rec->m_mutex);
    Buffer *buf = static_cast<Buffer *>(wl_buffer_get_user_data(buffer));
    QImage img = transform == LIPSTICK_RECORDER_TRANSFORM_Y_INVERTED ? buf->image.mirrored(false, true) : buf->image;
    if (rec->m_options.scale != 1.0f) {
        img = img.scaled(rec->m_size, Qt::KeepAspectRatio, rec->m_options.smooth ? Qt::SmoothTransformation : Qt::FastTransformation).convertToFormat(QImage::Format_RGBA8888);
    }
    buf->pixmap = QPixmap::fromImage(img);;
    QAviWriter *avi = rec->m_avi;
    QPixmap pix = buf->pixmap;
    int quality = rec->m_options.quality;
    if (rec->m_options.fullMode) {
        // Constant-fps mode: only the fps timer writes frames (see saveFrame()).
        // Here we just record the latest frame and ensure the timer is running so
        // the output is exactly `fps` frames/second with no duplicate writes.
        rec->m_lastFrame = buf;
        if (!rec->m_timer->isActive())
            rec->m_timer->start();
    } else {
        // Changed-frames-only mode: write each delivered frame immediately.
        QtConcurrent::run(rec->m_pool, [avi, pix, quality] {
            avi->addFrame(pix, "JPG", quality);
        });
    }
    buf->busy = false;

    time = timestamp;
}

void Recorder::failed(void *data, lipstick_recorder *recorder, int result, wl_buffer *buffer)
{
    Q_UNUSED(data)
    Q_UNUSED(recorder)
    Q_UNUSED(buffer)

    qFatal("Failed to record a frame, result %d.", result);
}

void Recorder::cancel(void *data, lipstick_recorder *recorder, wl_buffer *buffer)
{
    Q_UNUSED(recorder)

    Recorder *rec = static_cast<Recorder *>(data);

    QMutexLocker lock(&rec->m_mutex);
    Buffer *buf = static_cast<Buffer *>(wl_buffer_get_user_data(buffer));
    buf->busy = false;
}

void Recorder::global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version)
{
    Q_UNUSED(registry)

    Recorder *rec = static_cast<Recorder *>(data);
    if (strcmp(interface, "lipstick_recorder_manager") == 0) {
        rec->m_manager = static_cast<lipstick_recorder_manager *>(wl_registry_bind(registry, id, &lipstick_recorder_manager_interface, qMin(version, 1u)));
    } else if (strcmp(interface, "wl_shm") == 0) {
        rec->m_shm = static_cast<wl_shm *>(wl_registry_bind(registry, id, &wl_shm_interface, qMin(version, 1u)));
    }
}

void Recorder::globalRemove(void *data, wl_registry *registry, uint32_t id)
{
    Q_UNUSED(data)
    Q_UNUSED(registry)
    Q_UNUSED(id)
}
