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
#include <QFile>
#include <QPixmap>

#include <fstream>
using namespace std;

QAviWriter::QAviWriter(const QString &fileName, unsigned int fps, const QString &codec, QObject *parent)
    : QObject(parent)
    , d_file_name(fileName)
    , d_fps(fps)
{
	setCodec(codec);
}

/**
 * This is the first function you should call when using QAviWriter.
 * It allocates memory and takes care of initializing the AVI header with the provided information.
 *
 * @return true on success or false if an error occured.
 */
bool QAviWriter::open()
{
	gwavi = gwavi_open(d_file_name.toUtf8().constData(), (unsigned int)d_size.width(), (unsigned int)d_size.height(),
            d_codec.toLatin1().constData(), d_fps, ((!d_audio_file_name.isEmpty() && parseAudioFileFormat()) ? &d_audio_format : nullptr));

    return (gwavi != nullptr);
}

/**
 * This function should be called when the program is done adding video frames to the AVI file.
 * It adds the audio track if a valid audio file name was defined, frees the memory allocated and properly closes the output file.
 *
 * @return true on success or false if an error occured.
 */
bool QAviWriter::close()
{
	if (!d_audio_file_name.isEmpty())
		addAudioFile(d_audio_file_name);

	int error = gwavi_close(gwavi);
	if (!error)
        gwavi = nullptr;

	return (error == 0);
}

//! This function allows you to set an audio track to your AVI file. It should be called before calling the open() function.
void QAviWriter::setAudioFileName(const QString &audioFileName)
{
	d_audio_file_name = audioFileName;
}

bool QAviWriter::parseAudioFileFormat()
{
	std::ifstream file(d_audio_file_name.toUtf8().constData(), std::ifstream::in | std::ifstream::binary);
	if (!file.is_open()){
		printf("Error opening the input audio file!\n");
		return false;
	}

	file.seekg(20, ios::beg);
	short format;
	file.read((char*)&format, sizeof(short)); // read the file format. This should be 1 for PCM
	if (format != 1){
		printf("The input audio file is not a PCM type file!\n");
		file.close();
		return false;
	}

	short channels;
	file.read((char*)&channels, sizeof(short));// read the number of channels (1 or 2)

	d_audio_format.channels = (unsigned int)channels;
	//printf("Audio channels: %d\n", channels);

	int sampleRate;
	file.read((char*)&sampleRate, sizeof(int));
	d_audio_format.samples_per_second = (unsigned int)sampleRate;
	//printf("Audio sample rate: %d\n", sampleRate);

	file.read((char*)&d_bit_rate, sizeof(unsigned int));
	//printf("Audio bit rate: %d\n", d_bit_rate);

	short blockAlign;
	file.read((char*)&blockAlign, sizeof(short));

	short bitsPerSample;
	file.read((char*)&bitsPerSample, sizeof(short));
	d_audio_format.bits = (unsigned int)bitsPerSample;
	//printf("Audio bits per sample: %d\n", bitsPerSample);

	file.close();
	return true;
}

/**
 * This function allows you to reset the video size. In a standard use case, you
 * should not need to call it. However, if you need to, you can call it to reset
 * the video height and width set in the AVI file after you are done adding
 * frames to your AVI file and before you call close().
 *
 * @return true on success, false on error.
 */
bool QAviWriter::resetSize(const QSize &size)
{
    if (d_size == size)
        return false;

    int error = gwavi_set_size(gwavi, (unsigned int)size.width(), (unsigned int)size.height());
    if (!error)
        d_size = size;

    return (error == 0);
}

void QAviWriter::setSize(const QSize &size)
{
    d_size = size;
}

//! This function allows you to set the video codec.
bool QAviWriter::setCodec(const QString &codec)
{
	if (d_codec == codec)
		return false;

	if (isCodecSupported(codec)){
		d_codec = codec;
		return true;
	}

	return false;
}

bool QAviWriter::isCodecSupported(const QString &codec)
{
    return (check_fourcc(codec.toLatin1().constData()) == 0);
}

//! This function allows you to add an encoded video frame to the AVI file.
bool QAviWriter::addFrame(QImage img, const char* format, int quality)
{
	if (!gwavi)
		return false;

	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);
	img.save(&buffer, format, quality);

	int error = gwavi_add_frame(gwavi, (unsigned char *)ba.data(), (size_t)ba.size());
	if (!error)
		++d_frame_count;

	return (error == 0);
}

//! This function allows you to add an encoded video frame to the AVI file.
bool QAviWriter::addFrame(QPixmap pix, const char* format, int quality)
{
	if (!gwavi)
		return false;

	QByteArray ba;
	QBuffer buffer(&ba);
	buffer.open(QIODevice::WriteOnly);
	pix.save(&buffer, format, quality);

	int error = gwavi_add_frame(gwavi, (unsigned char *)ba.data(), (size_t)ba.size());
	if (!error)
		++d_frame_count;

	return (error == 0);
}

//! This function allows you to add an encoded video frame to the AVI file.
bool QAviWriter::addFrame(unsigned char* buffer, size_t length)
{
	if (!gwavi)
		return false;

	int error = gwavi_add_frame(gwavi, buffer, length);
	if (!error)
		++d_frame_count;

	return (error == 0);
}

bool QAviWriter::addAudioFile(const QString &fileName)
{
	if (!gwavi || fileName.isEmpty())
		return false;

	QFile file(fileName);
	if (!file.open(QIODevice::ReadOnly))
		return false;

	QByteArray ba = file.readAll();
	return (gwavi_add_audio(gwavi, (unsigned char *)ba.data(), (size_t)ba.size()) == 0);
}

/**
 * Returns the number of channels in the audio file.
 * @return 0 if there's no audio track set or 1 for mono and 2 for stereo.
 */
unsigned int QAviWriter::audioChannelCount()
{
	return d_audio_file_name.isEmpty() ? 0 : d_audio_format.channels;
}

unsigned int QAviWriter::audioSampleRate()
{
	return d_audio_file_name.isEmpty() ? 0 : d_audio_format.samples_per_second;
}

unsigned int QAviWriter::audioBitsPerSample()
{
	return d_audio_file_name.isEmpty() ? 0 : d_audio_format.bits;
}

QList<QByteArray> QAviWriter::supportedAudioFormats()
{
	QList<QByteArray> lst;
	lst.append(QByteArray("WAV", 3));
	return lst;
}

QList<QByteArray> QAviWriter::supportedVideoFormats()
{
	QList<QByteArray> lst;
	lst.append(QByteArray("AVI", 3));
	return lst;
}

QAviWriter::~QAviWriter()
{
	if (gwavi)
		close();
}
