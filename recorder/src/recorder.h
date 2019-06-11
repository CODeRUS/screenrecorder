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

#ifndef LIPSTICKRECORDER_RECORDER_H
#define LIPSTICKRECORDER_RECORDER_H

#include <QObject>
#include <QMutex>
#include <wayland-client.h>
#include <QImage>
#include <QPixmap>

class QScreen;
class QAviWriter;
class QThreadPool;

struct wl_display;
struct wl_registry;
struct lipstick_recorder_manager;
struct lipstick_recorder;

class Buffer;

class Recorder : public QObject
{
    Q_OBJECT
public:
    explicit Recorder(const QString &destination, int fps, int buffers, bool fullMode, QObject *parent = nullptr);
    virtual ~Recorder();

public slots:
    void init();
    void start();
    void handleShutDown();

private slots:
    void recordFrame();
    void saveFrame();

private:
    static void global(void *data, wl_registry *registry, uint32_t id, const char *interface, uint32_t version);
    static void globalRemove(void *data, wl_registry *registry, uint32_t id);
    static void setup(void *data, lipstick_recorder *recorder, int width, int height, int stride, int format);
    static void frame(void *data, lipstick_recorder *recorder, wl_buffer *buffer, uint32_t time, int transform);
    static void failed(void *data, lipstick_recorder *recorder, int result, wl_buffer *buffer);
    static void cancel(void *data, lipstick_recorder *recorder, wl_buffer *buffer);

    wl_display *m_display = nullptr;
    wl_registry *m_registry = nullptr;
    wl_shm *m_shm = nullptr;
    lipstick_recorder_manager *m_manager = nullptr;
    lipstick_recorder *m_recorder = nullptr;
    QScreen *m_screen = nullptr;
    QList<Buffer *> m_buffers;
    Buffer *m_lastFrame = nullptr;
    bool m_starving = false;
    QMutex m_mutex;

    QAviWriter *m_avi = nullptr;
    bool m_shutdown = false;

    QString m_destination;
    int m_fps;
    int m_buffersCount;

    QThreadPool *m_pool;
    QTimer *m_timer;

    bool m_fullMode;

    friend class BuffersHandler;
};

#endif
