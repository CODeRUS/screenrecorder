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
#include <QLoggingCategory>

#include "wayland-lipstick-recorder-client-protocol.h"
#include "recorder.h"

#include "QAviWriter.h"

Q_LOGGING_CATEGORY(logrecorder, "screenrecorder.recorder", QtDebugMsg)
Q_LOGGING_CATEGORY(logbuffer, "screenrecorder.recorder.buffer", QtDebugMsg)

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
    bool busy = false;
    int transform = 0;
    uint32_t timestamp = 0;
};

WorkerTask::WorkerTask(QAviWriter *avi, const QImage &image)
    : m_avi(avi)
    , m_image(image)
{
}

void WorkerTask::run()
{
    m_avi->addFrame(m_image, "JPG");
}

static void callback(void *data, wl_callback *cb, uint32_t time)
{
    Q_UNUSED(time)
    wl_callback_destroy(cb);
    QTimer::singleShot(0, static_cast<Recorder *>(data), &Recorder::start);
}

Recorder::Recorder(const QString &destination, int fps, int buffers, QObject *parent)
    : QObject(parent)
    , m_destination(destination)
    , m_fps(fps)
    , m_avi(new QAviWriter(destination, fps, QStringLiteral("MJPG"), this))
    , m_buffersCount(buffers)
    , m_pool(new QThreadPool(this))
    , m_timer(new QTimer(this))
{
    m_pool->setMaxThreadCount(1);

    m_timer->setTimerType(Qt::PreciseTimer);
    m_timer->setInterval(1000 / m_fps);
    m_timer->setSingleShot(true);
    connect(m_timer, &QTimer::timeout, this, &Recorder::saveFrame);
}

Recorder::~Recorder()
{
    lipstick_recorder_destroy(m_recorder);
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

    QPlatformNativeInterface *native = QGuiApplication::platformNativeInterface();
    m_screen = QGuiApplication::screens().first();
    wl_output *output = static_cast<wl_output *>(native->nativeResourceForScreen(QByteArrayLiteral("output"), m_screen));

    m_avi->setSize(m_screen->size());
    m_avi->open();

    m_recorder = lipstick_recorder_manager_create_recorder(m_manager, output);
    static const lipstick_recorder_listener recorderListener = {
        setup,
        frame,
        failed,
        cancel
    };
    lipstick_recorder_add_listener(m_recorder, &recorderListener, this);
}

void Recorder::handleShutDown()
{
    m_shutdown = true;

    qCDebug(logrecorder) << "Saving frames, please wait!";
    m_pool->waitForDone();

    if (m_avi) {
        m_avi->close();
    }

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

    QImage img = m_lastFrame->transform == LIPSTICK_RECORDER_TRANSFORM_Y_INVERTED ? m_lastFrame->image.mirrored(false, true) : m_lastFrame->image;
    m_pool->start(new WorkerTask(m_avi, img));
    m_lastFrame->busy = false;
    if (m_starving)
        recordFrame();
    m_timer->start();
}

void Recorder::setup(void *data, lipstick_recorder *recorder, int width, int height, int stride, int format)
{
    Recorder *rec = static_cast<Recorder *>(data);
    QMutexLocker lock(&rec->m_mutex);

    for (int i = 0; i < rec->m_buffersCount; ++i) {
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
    rec->recordFrame();
    static uint32_t time = 0;

    QMutexLocker lock(&rec->m_mutex);
    Buffer *buf = static_cast<Buffer *>(wl_buffer_get_user_data(buffer));
    buf->transform = transform;
    buf->timestamp = timestamp;
    if (rec->m_lastFrame)
        rec->m_lastFrame->busy = false;
    rec->m_lastFrame = buf;
    rec->saveFrame();

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
