#include "audio.hh"
#include <ovlnet/logger.hh>

Audio::Audio(QObject *parent)
  : QIODevice(parent), _input(0)
{
  QAudioFormat format;
  format.setSampleRate(48000);
  format.setChannelCount(1);
  format.setSampleSize(16);
  format.setCodec("audio/pcm");
  format.setSampleType(QAudioFormat::SignedInt);
  format.setByteOrder(QAudioFormat::Endian(QSysInfo::ByteOrder));

  QAudioDeviceInfo info = QAudioDeviceInfo::defaultInputDevice();
  if (!info.isFormatSupported(format)) {
    QAudioFormat near = info.nearestFormat(format);
    logError() << "Default format not supported try to use nearest: "
               << near.sampleRate() << " Hz, "
               << near.channelCount() << " channels, "
               << near.sampleSize() << "b, "
               << ((QAudioFormat::LittleEndian==near.byteOrder()) ? "little" : "big") << "  endian, "
               << near.sampleType() << " type.";
    return;
  }

  _input = new QAudioInput(format, this);
  //_input->setBufferSize(1024);
}

Audio::Audio(const QAudioDeviceInfo &device, QObject *parent)
  : QIODevice(parent), _input(0)
{
  QAudioFormat format;
  format.setSampleRate(46000);
  format.setChannelCount(1);
  format.setSampleSize(16);
  format.setCodec("audio/pcm");
  format.setSampleType(QAudioFormat::SignedInt);
  format.setByteOrder(QAudioFormat::Endian(QSysInfo::ByteOrder));

  if (! device.isFormatSupported(format)) {
    QAudioFormat near = device.nearestFormat(format);
    logError() << "Default format not supported try to use nearest: "
               << near.sampleRate() << " Hz, "
               << near.channelCount() << " channels, "
               << near.sampleSize() << "b, "
               << ((QAudioFormat::LittleEndian==near.byteOrder()) ? "little" : "big") << "  endian, "
               << near.sampleType() << " type.";
    return;
  }

  _input = new QAudioInput(device, format, this);
}

bool
Audio::ready() const {
  return 0 != _input;
}

bool Audio::start(double mSec) {
  // Check if input device is ready
  if (! ready()) {
    logError() << "Cannot start recording: Input device not initialized.";
    return false;
  }

  // Open this proxy
  if (! this->open(QIODevice::WriteOnly)) {
    logError() << "Cannot open sound proxy device.";
    return false;
  }

  // Compute samples to record
  if (mSec>0) {
    _nSamples = 96e3*mSec/1000;
  } else {
    _nSamples = -1;
  }

  // Go.
  _input->start(this);
  return QAudio::ActiveState == _input->state();
}

void
Audio::stop() {
  if (ready())
    _input->stop();
  this->close();
  emit stopped();
}

qint64
Audio::readData(char *data, qint64 maxlen) {
  // pass...
  return 0;
}

qint64
Audio::writeData(const char *data, qint64 len) {
  // If recording is complete
  if (0 == _nSamples) {
    stop();
    return 0;
  }
  // Otherwise determine the number of samples provided
  size_t nSample = len/2;
  // determine the max. number of samples to process
  if (_nSamples>0) {
    nSample = std::min(int64_t(nSample), _nSamples);
  }
  // If some samples to process are left
  if (nSample) {
    // process samples
    emit stream((const int16_t *)data, nSample);
  }
  // Update samples to process
  if (_nSamples>0) {
    _nSamples -= nSample;
  }
  // done
  return 2*nSample;
}
