#ifndef AUDIO_HH
#define AUDIO_HH

#include <QIODevice>
#include <QAudioInput>

class Audio: public QIODevice
{
  Q_OBJECT

public:
  explicit Audio(QObject *parent=0);
  Audio(const QAudioDeviceInfo &device, QObject *parent=0);

  QAudioDeviceInfo device() const;
  bool setDevice(const QAudioDeviceInfo &device);

  bool ready() const;

public slots:
  bool start(double mSec=-1);
  void stop();

signals:
  void stream(const int16_t *data, size_t len);
  void stopped();

protected:
  qint64 readData(char *data, qint64 maxlen);
  qint64 writeData(const char *data, qint64 len);

protected:
  QAudioDeviceInfo _device;
  QAudioInput *_input;
  int64_t _nSamples;
};

#endif // AUDIO_HH
