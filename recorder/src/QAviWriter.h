/***************************************************************************
File                 : QAviWriter.h
Project              : libqtavi
--------------------------------------------------------------------
Copyright            : (C) 2017 by Ion Vasilief
Email (use @ for *)  : ion_vasilief*yahoo.fr
Description          : Easy creation of AVI video files for Qt-based applications
***************************************************************************/
#ifndef Q_AVI_WRITER_H
#define Q_AVI_WRITER_H

#include <QImage>
#include <QObject>
#include <QPixmap>
#include <QSize>

#include "gwavi.h"

class QAviWriter : public QObject
{
public:
    QAviWriter(const QString &codec = QStringLiteral("MJPG"), QObject *parent = nullptr);
	~QAviWriter();

	bool open();
	bool close();

    void setFileName(const QString &fileName);
    QString fileName() const {return d_file_name;}

    void setFps(unsigned int fps);

    void setSize(const QSize &size);
    QSize size() const {return d_size;}

	//! Returns the number of frames in the output video file
    unsigned int count() const {return d_frame_count;}
	//! This function allows you to add an encoded video frame to the AVI file.
    bool addFrame(const QImage &img, const char* format = "JPG", int quality = -1);
    bool addFrame(const QPixmap &img, const char* format = "JPG", int quality = -1);

private:
	//! Name of the output .avi file
	QString d_file_name;
	//! FourCC representing the codec of the video encoded stream
	QString d_codec;
	//! Size in pixels
	QSize d_size;
	//! Framerate: number of frames per second of the output video file
    unsigned int d_fps = 0;
	//! The number of frames in the output video file
    unsigned int d_frame_count = 0;

    GWAVI *d_gwavi = nullptr;

};
#endif
