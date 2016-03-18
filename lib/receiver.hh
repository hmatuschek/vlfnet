#ifndef RECEIVER_HH
#define RECEIVER_HH

#include <QIODevice>
#include <QTemporaryFile>
#include <QDateTime>
#include "audio.hh"
#include "location.hh"
#include "datasetfile.hh"
#include <fftw3.h>


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
  Receiver(Station &station, const ReceiverConfig &config, QObject *parent=0);
  virtual ~Receiver();

public slots:
  bool start(double mSec=-1);
  void stop();

protected:
  qint64 writeData(const char *data, qint64 len);
  bool save();

protected:
  Station &_station;
  QTemporaryFile _tmpFile;
  size_t _samples;
  QDateTime _startTime;
};


class Beacon
{
public:
  Beacon();
  Beacon(const QString &name, double fmin, double fmax);
  Beacon(const Beacon &other);

  Beacon &operator=(const Beacon &other);

  const QString &name() const;
  double fmin() const;
  double fmax() const;

protected:
  QString _name;
  double _fmin;
  double _fmax;
};


class BeaconReceiver: public Audio
{
  Q_OBJECT

public:
  BeaconReceiver(const QVector<Beacon> &beacons, double tau, const ReceiverConfig &config, Station &station);
  virtual ~BeaconReceiver();

  const QVector<Beacon> &beacons() const;
  const QVector<double> &averages() const;

protected:
  qint64 writeData(const char *data, qint64 len);
  void _doFFT();

protected:
  Station &_station;

  fftw_plan _fft;
  size_t _nFFTBuffer;
  double *_fftInBuffer;
  double *_fftOutBuffer;
  double _lambda;
  QVector<Beacon> _beacons;
  QVector<double> _averages;
};

#endif // RECEIVER_HH
