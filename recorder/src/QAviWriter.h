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

#include <QObject>
#include <QSize>

extern "C" {
#include <gwavi.h>
#include <avi-utils.h>
}

class QPixmap;
class QImage;

class QAviWriter : public QObject
{
public:
    QAviWriter(const QString &fileName, unsigned int fps = 24, const QString &codec = QStringLiteral("MJPG"), QObject *parent = nullptr);
	~QAviWriter();

	bool open();
	bool close();

	//! Returns the name of the output file.
	QString fileName(){return d_file_name;}

	//! Returns the name of the file used to set the audio track.
	QString audioFileName(){return d_audio_file_name;}
	void setAudioFileName(const QString&);

	//! Returns the FourCC representing the codec of the encoded video.
	QString codec(){return d_codec;}
	//! This function allows you to set the video codec.
	bool setCodec(const QString&);
	//! This function allows you to check if a video codec is supported.
	static bool isCodecSupported(const QString&);

	//! Returns the size in pixels of each encoded video frame
	QSize size(){return d_size;}
	//! This function allows you to reset the video size.
    bool resetSize(const QSize&);
    void setSize(const QSize&);

	//! Returns the number of frames in the output video file
	unsigned int count(){return d_frame_count;}
	//! Returns the framerate: number of frames per second of the output video file
	unsigned int framerate(){return d_fps;}

	//! This function allows you to add an encoded video frame to the AVI file.
	bool addFrame(unsigned char*, size_t);
	//! This function allows you to add an encoded video frame to the AVI file.
	bool addFrame(QPixmap pix, const char* format = "JPG", int quality = -1);
	//! This function allows you to add an encoded video frame to the AVI file.
	bool addFrame(QImage img, const char* format = "JPG", int quality = -1);

	unsigned int audioChannelCount();
	unsigned int audioSampleRate();
	unsigned int audioBitsPerSample();
	unsigned int audioBitRate(){return d_bit_rate;}

	//! Returns the list of audio formats supported by QAviWriter (only .wav files for the moment).
	static QList<QByteArray> supportedAudioFormats();
	//! Returns the list of video formats supported by QAviWriter (only .avi files for the moment).
	static QList<QByteArray> supportedVideoFormats();

private:
	//! Tries to guess the format of the audio track, based on the assumption that it has a PCM format.
	bool parseAudioFileFormat();
	//! Actually adds the audio track to the output AVI file.
	bool addAudioFile(const QString&);

	//! Name of the output .avi file
	QString d_file_name;
	//! Name of the input audio file
	QString d_audio_file_name;
	//! FourCC representing the codec of the video encoded stream
	QString d_codec;
	//! Size in pixels
	QSize d_size;
	//! Framerate: number of frames per second of the output video file
    unsigned int d_fps = 0;
	//! The number of frames in the output video file
    unsigned int d_frame_count = 0;
	//! The bit rate of the audio track, that is the number of bits encoded per second
    unsigned int d_bit_rate = 0;
    struct gwavi_t *gwavi = nullptr;
	struct gwavi_audio_t d_audio_format;
};
#endif
