#ifndef RECEIVER_HH
#define RECEIVER_HH

#include <QIODevice>
#include <QTemporaryFile>
#include <QDateTime>
#include "audio.hh"
#include "location.hh"
#include "datasetfile.hh"


class ReceiverConfig
{
public:
  ReceiverConfig();
  explicit ReceiverConfig(const QString &filename);
  explicit ReceiverConfig(const QJsonObject &obj);
  ReceiverConfig(const ReceiverConfig &other);

  ReceiverConfig &operator =(const ReceiverConfig &other);

  QJsonObject toJson() const;
  bool save(const QString &filename) const;

  const QAudioDeviceInfo &device() const;
  void setDevice(const QAudioDeviceInfo &device);

protected:
  QAudioDeviceInfo _device;
};


class Receiver: public Audio
{
  Q_OBJECT

public:
  Receiver(const Location &location, DataSetDir &dataDir, const ReceiverConfig &config, QObject *parent=0);
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
  DataSetDir &_dataDir;
  QDateTime _startTime;
  Location _location;
};

#endif // RECEIVER_HH
