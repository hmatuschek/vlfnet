#include "datasetfile.hh"

/* ********************************************************************************************* *
 * Implementation of DataSetFile
 * ********************************************************************************************* */
DataSetFile::DataSetFile(const QString &filename)
  : _filename(filename), _timestamp(), _numSamples(0), _sampleRate(0), _parents(),
    _datasets()
{
  QFile file(_filename);
  // Try to open file.
  if (! file.open(QIODevice::ReadOnly)) {
    logError() << "Cannot open dataset file for reading: " << file.fileName();
    _reset();
    return;
  }
  // Read dataset file header
  DataSetFileHeader fileheader;
  if (sizeof(DataSetFileHeader) != file.read((char *) &fileheader, sizeof(fileheader))) {
    logError() << "Cannot read dataset file header: Malformed dataset file?";
    _reset();
    return;
  }
  // Get meta data
  _timestamp = QDateTime(QDate(ntohs(fileheader.year), fileheader.month, fileheader.day),
                         QTime(fileheader.hour, fileheader.minute, fileheader.second), Qt::UTC).toLocalTime();
  size_t numDatasets = ntohs(fileheader.datasets);
  _numSamples  = ntohl(fileheader.samples);
  _sampleRate  = ntohl(fileheader.rate);
  // Read parent datasets
  for (uint16_t i=0; i<ntohs(fileheader.parents); i++) {
    char id[OVL_HASH_SIZE];
    if (OVL_HASH_SIZE != file.read(id, OVL_HASH_SIZE)) {
      logError() << "Cannot read parent datasets from file header: Malformed dataset file?";
      _reset();
      return;
    }
    _parents.push_back(Identifier(id));
  }

  // Read all dataset headers;
  size_t offset = sizeof(DataSetFileHeader) + OVL_HASH_SIZE*_parents.size();
  DataSetHeader header;
  for (size_t i=0; i<numDatasets; i++) {
    if(! file.seek(offset)) {
      logError() << "Cannot seek to next dataset header: Malformed dataset file?";
      _reset();
      return;
    }
    if (sizeof(DataSetHeader) != file.read((char *) &header, sizeof(DataSetHeader))) {
      logError() << "Cannot read dataset header: Malformed dataset file`?";
      _reset();
      return;
    }
    _datasets.append(QPair<Location, size_t>(
                       Location(header.latitude, header.latitude, header.height), offset));
    offset += _numSamples;
  }
}

void
DataSetFile::_reset() {
  _filename.clear();
  _timestamp = QDateTime();
  _numSamples = 0;
  _sampleRate = 0;
  _parents.clear();
  _datasets.clear();
}

bool
DataSetFile::isValid() const {
  return _filename.length() && _timestamp.isValid()
      && _numSamples && _sampleRate && _datasets.size();
}

const QString &
DataSetFile::filename() const {
  return _filename;
}

const QDateTime &
DataSetFile::datetime() const {
  return _timestamp;
}

size_t
DataSetFile::samples() const {
  return _numSamples;
}

size_t
DataSetFile::sampleRate() const {
  return _sampleRate;
}

size_t
DataSetFile::numParents() const {
  return _parents.size();
}

const Identifier &
DataSetFile::parent(size_t i) const {
  return _parents[i];
}

const QVector<Identifier> &
DataSetFile::parents() const {
  return _parents;
}

size_t
DataSetFile::numDatasets() const {
  return _datasets.size();
}

const Location &
DataSetFile::datasetLocation(size_t i) const {
  return _datasets[i].first;
}

bool
DataSetFile::readDataset(size_t i, int16_t *data) const {
  QFile file(_filename);
  if (! file.open(QIODevice::ReadOnly)) {
    logDebug() << "Cannot read dataset from " << _filename << ": Cannot open file.";
    return false;
  }
  if (! file.seek(_datasets[i].second+sizeof(DataSetHeader))) {
    logDebug() << "Cannot seek to dataset in " << _filename << ".";
    return false;
  }

  for (size_t i=0; i<_numSamples; i++) {
    int16_t val;
    if (2 != file.read((char *) &val, 2)) {
      logError() << "Cannot read dataset from file " << _filename << ": read returned error.";
      return false;
    }
    data[i] = ntohs(val);
  }

  return true;
}

QJsonObject
DataSetFile::toJson() const {
  QJsonObject res;
  res.insert("timestamp", _timestamp.toUTC().toString("yyyy-MM-DD hh:mm:ss"));
  res.insert("samples", int(_numSamples));
  res.insert("samplerate", int(_sampleRate));
  // assemble parent list
  QJsonArray parents;
  for (size_t i=0; i<_parents.size(); i++) {
    parents.append(_parents[i].toBase32());
  }
  res.insert("parents", parents);
  // Assemble list of datasets
  QJsonArray datasets;
  for (size_t i=0; i<_datasets.size(); i++) {
    datasets.append(_datasets[i].first.toJson());
  }
  res.insert("timeseries", datasets);

  return res;
}

/* ********************************************************************************************* *
 * Implementation of DataSetDir
 * ********************************************************************************************* */
DataSetDir::DataSetDir(const QString &directory)
  : _dir(directory)
{
  reload();
}

bool
DataSetDir::addDataSet(const Identifier &id) {
  DataSetFile file(id.toBase32());
  if (! file.isValid()) { return false; }
  _datasets.insert(id, file);
  return true;
}

bool
DataSetDir::addDataSet(const QString &name) {
  Identifier id = Identifier::fromBase32(name);
  if (id.isValid()) {
    return addDataSet(id);
  }
  return false;
}

void
DataSetDir::reload() {
  _datasets.clear();
  _dir.refresh();
  // Get directory content
  QStringList content = _dir.entryList(QDir::Files|QDir::Readable);
  foreach(QString filename, content) {
    addDataSet(filename);
  }
}

QJsonArray
DataSetDir::toJson() const {
  QJsonArray res;
  QHash<Identifier, DataSetFile>::const_iterator dataset = _datasets.begin();
  for (; dataset != _datasets.end(); dataset++) {
    res.append(dataset->toJson());
  }
  return res;
}
