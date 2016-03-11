#ifndef RECEIVER_HH
#define RECEIVER_HH

#include <QIODevice>
#include <QTemporaryFile>
#include <QDateTime>
#include "audio.hh"
#include "location.hh"


class Receiver: public Audio
{
  Q_OBJECT

public:
  Receiver(const Location &location, const QString &dataDir, const QAudioDeviceInfo &device, QObject *parent=0);
  virtual ~Receiver();

public slots:
  bool start(double mSec=-1);
  void stop();

protected:
  qint64 writeData(const char *data, qint64 len);
  bool save();

protected:
  QTemporaryFile _tmpFile;
  size_t _samples;
  QString _dataDir;
  QDateTime _startTime;
  Location _location;
};

#endif // RECEIVER_HH
