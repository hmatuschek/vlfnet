#include "receiver.hh"
#include <ovlnet/crypto.hh>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonParseError>
#include "datasetfile.hh"
#include <netinet/in.h>
#include "station.hh"


/* ********************************************************************************************* *
 * Implementation of ReceiverConfig
 * ********************************************************************************************* */
ReceiverConfig::ReceiverConfig()
  : _device()
{
  // pass...
}

ReceiverConfig::ReceiverConfig(const QString &filename)
  : _device()
{
  QFile file(filename);
  if (! file.open(QIODevice::ReadOnly)) {
    logError() << "Cannot read receiver config from " << filename << ".";
    return;
  }
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
  if (QJsonParseError::NoError != err.error) {
    logError() << "Cannot parse receiver config from " << filename
               << ": " << err.errorString() << ".";
    return;
  }
  if (! doc.isObject()) {
    logError() << "Cannot process reciever config from " << filename
               << ": Not an object." << doc.toJson();
    return;
  }
  QJsonObject obj = doc.object();
  if (! obj.contains("device")) {
    logError() << "No input device specified in receiver config " << filename << ".";
    return;
  }
  QString deviceName = obj.value("device").toString();
  if (deviceName.isEmpty()) { return; }
  logDebug() << "Use device " << deviceName << " as receiver input device.";
  QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
  foreach(QAudioDeviceInfo info, devices) {
    if (info.deviceName() == deviceName) {
      _device = info;
      break;
    }
  }
}

ReceiverConfig::ReceiverConfig(const QJsonObject &obj)
  : _device()
{
  if (! obj.contains("device")) {
    logError() << "No input device specified in receiver config.";
    return;
  }
  QString deviceName = obj.value("device").toString();
  if (deviceName.isEmpty()) { return; }
  QList<QAudioDeviceInfo> devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
  foreach(QAudioDeviceInfo info, devices) {
    if (info.deviceName() == deviceName) {
      _device = info;
      break;
    }
  }
}

ReceiverConfig::ReceiverConfig(const ReceiverConfig &other)
  : _device(other._device)
{
  // pass...
}

ReceiverConfig &
ReceiverConfig::operator =(const ReceiverConfig &other) {
  _device = other._device;
  return *this;
}

QJsonObject
ReceiverConfig::toJson() const {
  QJsonObject res;
  res.insert("device", _device.deviceName());
  return res;
}

bool
ReceiverConfig::save(const QString &filename) const {
  QJsonObject obj = toJson();
  QFile file(filename);
  if (! file.open(QIODevice::WriteOnly)) {
    logError() << "Cannot save receiver config to " << filename << ".";
    return false;
  }
  file.write(QJsonDocument(obj).toJson());
  file.flush();
  file.close();
  logDebug() << "Saved receiver config in " << filename << ".";
  return true;
}

const QAudioDeviceInfo &
ReceiverConfig::device() const {
  return _device;
}

void
ReceiverConfig::setDevice(const QAudioDeviceInfo &device) {
  _device = device;
}


/* ********************************************************************************************* *
 * Implementation of Receiver
 * ********************************************************************************************* */
Receiver::Receiver(Station &station, const ReceiverConfig &config, QObject *parent)
  : Audio(config.device(), parent), _station(station), _tmpFile(), _samples(0)
{
  // pass...
}

Receiver::~Receiver()
{
  // pass...
}

bool
Receiver::start(double mSec) {
  if (_tmpFile.isOpen()) {
    logError() << "Cannot start reception: Still receiving.";
    return false;
  }

  if (! Audio::start(mSec)) {
    logError() << "Cannot start reception: Audio device returned error.";
    return false;
  }

  _startTime = QDateTime::currentDateTimeUtc();
  return _tmpFile.open();
}

void
Receiver::stop() {
  logDebug() << "Stop reception. Store data.";
  Audio::stop();
  if (_tmpFile.open()) {
    save();
  }
  _tmpFile.close();
}

bool
Receiver::save() {
  DataSetFileHeader fileHeader;
  fileHeader.year = htons(_startTime.date().year());
  fileHeader.month = _startTime.date().month();
  fileHeader.day = _startTime.date().day();
  fileHeader.hour = _startTime.time().hour();
  fileHeader.minute = _startTime.time().minute();
  fileHeader.second = _startTime.time().second();
  fileHeader.datasets = htons(1);
  fileHeader.samples = htonl(_samples);
  fileHeader.rate = htonl(_input->format().sampleRate());
  fileHeader.parents = 0;

  DataSetHeader header;
  header.longitude = _station.location().longitude();
  header.latitude = _station.location().latitude();
  header.height = _station.location().height();

  // Create a new temp file to assemble datafile in
  QTemporaryFile tmpFile;
  if (! tmpFile.open()) { return false; }
  EVP_MD_CTX mdctx; OVLHashInit(&mdctx);

  // Write file header
  tmpFile.write((const char *) &fileHeader, sizeof(DataSetFileHeader));
  OVLHashUpdate((const unsigned char *) &fileHeader, sizeof(DataSetFileHeader), &mdctx);
  // write dataset header
  tmpFile.write((const char *) &header, sizeof(DataSetHeader));
  OVLHashUpdate((const unsigned char *) &header, sizeof(DataSetHeader), &mdctx);

  // copy data from _tmpFile
  _tmpFile.seek(0);
  for (size_t i=0; i<_nSamples; i++) {
    int16_t value;
    _tmpFile.read((char *) &value, 2);
    tmpFile.write((const char *) &value, 2);
    OVLHashUpdate((const unsigned char *) &value, 2, &mdctx);
  }

  // Get hash of file
  char hash[OVL_HASH_SIZE];
  OVLHashFinal(&mdctx, (uint8_t *)hash);

  // Move to destination
  tmpFile.copy(_station.datasets().path()+Identifier(hash).toBase32());
  logDebug() << "Added dataset at " << _station.datasets().path()
             << "/" << Identifier(hash).toBase32();
  return true;
}

qint64
Receiver::writeData(const char *data, qint64 len) {
  // Forward to default implementation Audio::writeData
  qint64 nbytes = Audio::writeData(data, len);

  // Save data into temp file in network byte-order
  size_t offset = 0;
  while (nbytes>0) {
    int16_t value = htons(*(int16_t *)(data+offset));
    _tmpFile.write((const char *) &value, 2);
    nbytes -= 2; _samples += 1;
  }

  return nbytes;
}



/* ********************************************************************************************* *
 * Implementation of Beacon
 * ********************************************************************************************* */
Beacon::Beacon()
  : _name(), _fmin(0), _fmax(0)
{
  // pass...
}

Beacon::Beacon(const QString &name, double fmin, double fmax)
  : _name(name), _fmin(fmin), _fmax(fmax)
{
  // pass...
}

Beacon::Beacon(const Beacon &other)
  : _name(other._name), _fmin(other._fmin), _fmax(other._fmax)
{
  // pass...
}

const QString &
Beacon::name() const {
  return _name;
}

double
Beacon::fmin() const {
  return _fmin;
}

double
Beacon::fmax() const {
  return _fmax;
}


/* ********************************************************************************************* *
 * Implementation of BeaconReceiver
 * ********************************************************************************************* */
BeaconReceiver::BeaconReceiver(const QVector<Beacon> &beacons, double tau,
                               const ReceiverConfig &config, Station &station)
  : Audio(config.device(), &station), _station(station), _nFFTBuffer(4096), _beacons(beacons)
{
  _fftInBuffer = new double[4096];
  _fftOutBuffer = new double[4096];
  _fft = fftw_plan_r2r_1d(4096, _fftInBuffer, _fftOutBuffer, FFTW_R2HC, FFTW_ESTIMATE);
  // resize and initialize signal averages
  _averages.fill(0, _beacons.size());
  // damping factor for the averaging
  _lambda = std::min(1., 4096./48e3/tau);
}

BeaconReceiver::~BeaconReceiver() {
  fftw_destroy_plan(_fft);
  delete[] _fftInBuffer;
  delete[] _fftOutBuffer;
}

const QVector<Beacon> &
BeaconReceiver::beacons() const {
  return _beacons;
}

const QVector<double> &
BeaconReceiver::averages() const {
  return _averages;
}

qint64
BeaconReceiver::writeData(const char *data, qint64 len) {
  if (len <= 0) { return 0; }
  // Store samples in FFT input buffer
  size_t nSamples = std::min(size_t(len/2), _nFFTBuffer);
  for (size_t i=0; i<nSamples; i++) {
    _fftInBuffer[4096-_nFFTBuffer] = ntohs(* (int16_t *)(data+i));
  }
  // update samples left
  _nFFTBuffer -= nSamples;
  // If inbuffer is full -> do FFT
  if (0 == _nFFTBuffer) {
    _doFFT();
    _nFFTBuffer = 4096;
  }

  // Return the number of bytes processed
  return nSamples*2;
}

void
BeaconReceiver::_doFFT() {
  // Perform FFT
  fftw_execute(_fft);
  // Update signal estimates
  for (size_t i=0; i<_beacons.size(); i++) {
    int a = 4096*_beacons[i].fmin()/48e3;
    int b = 4096*_beacons[i].fmax()/48e3;
    // limit a & b
    a = std::max(1, std::min(a, 2048));
    b = std::max(1, std::min(b, 2048));
    // get max signal
    double sig = 0;
    for (int j=a; j<b; j++) {
      double v = _fftOutBuffer[j]*_fftOutBuffer[j] +
          _fftOutBuffer[4096-j]*_fftOutBuffer[4096-j];
      v = std::sqrt(v/4096);
      sig = std::max(sig, v);
    }
    // peform averaging
    _averages[i] = (1-_lambda)*_averages[i] + _lambda*sig;
  }
}
