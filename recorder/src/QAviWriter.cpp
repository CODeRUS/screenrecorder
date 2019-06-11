/***************************************************************************
File                 : QAviWriter.cpp
Project              : libqtavi
--------------------------------------------------------------------
Copyright            : (C) 2017 by Ion Vasilief
Email (use @ for *)  : ion_vasilief*yahoo.fr
Description          : Easy creation of AVI video files for Qt-based applications
***************************************************************************/
#include "QAviWriter.h"

#include <QBuffer>

QAviWriter::QAviWriter(const QString &codec, QObject *parent)
    : QObject(parent)
    , d_codec(codec)
{
}

/**
 * This is the first function you should call when using QAviWriter.
 * It allocates memory and takes care of initializing the AVI header with the provided information.
 *
 * @return true on success or false if an error occured.
 */
bool QAviWriter::open()
{
    d_gwavi = new GWAVI(d_file_name.toUtf8().constData(),
                        (unsigned int)d_size.width(),
                        (unsigned int)d_size.height(),
                        24,
                        d_codec.toLatin1().constData(),
                        d_fps,
                        nullptr);

    return (d_gwavi != nullptr);
}

/**
 * This function should be called when the program is done adding video frames to the AVI file.
 * It adds the audio track if a valid audio file name was defined, frees the memory allocated and properly closes the output file.
 *
 * @return true on success or false if an error occured.
 */
bool QAviWriter::close()
{
    int error = d_gwavi->Finalize();
	if (!error)
        d_gwavi = nullptr;

    return (error == 0);
}

void QAviWriter::setFileName(const QString &fileName)
{
    d_file_name = fileName;
}

void QAviWriter::setFps(unsigned int fps)
{
    d_fps = fps;
}

void QAviWriter::setSize(const QSize &size)
{
    d_size = size;
}

//! This function allows you to add an encoded video frame to the AVI file.
bool QAviWriter::addFrame(const QImage &img, const char* format, int quality)
{
    if (!d_gwavi)
		return false;

	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);
	img.save(&buffer, format, quality);

    int error = d_gwavi->AddVideoFrame(ba.constData(), (size_t)ba.size());
	if (!error)
		++d_frame_count;

    return (error == 0);
}

bool QAviWriter::addFrame(const QPixmap &img, const char *format, int quality)
{
    if (!d_gwavi)
        return false;

    QByteArray ba;
    QBuffer buffer(&ba);
    buffer.open(QIODevice::WriteOnly);
    img.save(&buffer, format, quality);

    int error = d_gwavi->AddVideoFrame(ba.constData(), (size_t)ba.size());
    if (!error)
        ++d_frame_count;

    return (error == 0);
}

QAviWriter::~QAviWriter()
{
    if (d_gwavi)
		close();
}
