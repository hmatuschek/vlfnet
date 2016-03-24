#include "datasetfile.hh"
#include <netinet/in.h>
#include "query.hh"
#include "station.hh"


/* ********************************************************************************************* *
 * Implementation of Timeseries
 * ********************************************************************************************* */
Timeseries::Timeseries()
  : _offset(0), _identifier(), _location()
{
  // pass...
}

Timeseries::Timeseries(size_t offset, const Timeseries::Header *header)
  : _offset(offset), _identifier(header->identifier),
    _location(header->longitude, header->latitude,header->height)
{
  // pass...
}

Timeseries::Timeseries(const Timeseries &other)
  : _offset(other._offset), _identifier(other._identifier), _location(other._location)
{
  // pass...
}

Timeseries &
Timeseries::operator =(const Timeseries &other) {
  _offset = other._offset;
  _identifier = other._identifier;
  _location = other._location;
  return *this;
}

size_t
Timeseries::offset() const {
  return _offset;
}

const Identifier &
Timeseries::identifier() const {
  return _identifier;
}

const Location &
Timeseries::location() const {
  return _location;
}

QJsonObject
Timeseries::toJson() const {
  QJsonObject obj;
  if (_identifier.isValid()) {
    obj.insert("id", _identifier.toBase32());
  }
  obj.insert("location", _location.toJson());
  return obj;
}


/* ********************************************************************************************* *
 * Implementation of DataSetFile
 * ********************************************************************************************* */
DataSetFile::DataSetFile()
  : _filename(), _timestamp(), _numSamples(0), _sampleRate(0), _datasets()
{
  // pass...
}

DataSetFile::DataSetFile(const QString &filename)
  : _filename(filename), _timestamp(), _numSamples(0), _sampleRate(0), _datasets()
{
  QFile file(_filename);
  // Try to open file.
  if (! file.open(QIODevice::ReadOnly)) {
    logError() << "Cannot open dataset file for reading: " << file.fileName();
    _reset();
    return;
  }
  // Read dataset file header
  DataSetFile::Header fileheader;
  if (sizeof(DataSetFile::Header) != file.read((char *) &fileheader, sizeof(DataSetFile::Header))) {
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

  // Read all dataset headers;
  size_t offset = sizeof(DataSetFile::Header);
  Timeseries::Header header;
  for (size_t i=0; i<numDatasets; i++) {
    if(! file.seek(offset)) {
      logError() << "Cannot seek to next timeseries header: Malformed dataset file?";
      _reset();
      return;
    }
    if (sizeof(Timeseries::Header) != file.read((char *) &header, sizeof(Timeseries::Header))) {
      logError() << "Cannot read timeseries header: Malformed dataset file`?";
      _reset();
      return;
    }
    _datasets.append(Timeseries(offset, &header));
    offset += _numSamples;
  }
}

DataSetFile::DataSetFile(const DataSetFile &other)
  : _filename(other._filename), _timestamp(other._timestamp), _numSamples(other._numSamples),
    _sampleRate(other._sampleRate), _datasets(other._datasets)
{
  // pass...
}

DataSetFile &
DataSetFile::operator =(const DataSetFile &other) {
  _filename = other._filename;
  _timestamp = other._timestamp;
  _numSamples = other._numSamples;
  _sampleRate = other._sampleRate;
  _datasets = other._datasets;
  return *this;
}

void
DataSetFile::_reset() {
  _filename.clear();
  _timestamp = QDateTime();
  _numSamples = 0;
  _sampleRate = 0;
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
DataSetFile::numTimeseries() const {
  return _datasets.size();
}

const Timeseries &
DataSetFile::timeseries(size_t i) const {
  return _datasets[i];
}

bool
DataSetFile::readTimeseries(size_t i, int16_t *data) const {
  QFile file(_filename);
  if (! file.open(QIODevice::ReadOnly)) {
    logDebug() << "Cannot read dataset from " << _filename << ": Cannot open file.";
    return false;
  }
  if (! file.seek(_datasets[i].offset()+sizeof(Timeseries::Header))) {
    logDebug() << "Cannot seek to timeseries in " << _filename << ".";
    return false;
  }

  for (size_t i=0; i<_numSamples; i++) {
    int16_t val;
    if (2 != file.read((char *) &val, 2)) {
      logError() << "Cannot read timeseries from file " << _filename << ": read returned error.";
      return false;
    }
    data[i] = ntohs(val);
  }

  return true;
}

QJsonObject
DataSetFile::toJson() const {
  QJsonObject res;
  res.insert("timestamp", _timestamp.toUTC().toString("yyyy-MM-dd hh:mm:ss"));
  res.insert("samples", int(_numSamples));
  res.insert("samplerate", int(_sampleRate));
  // Assemble list of datasets
  QJsonArray datasets;
  for (int i=0; i<_datasets.size(); i++) {
    datasets.append(_datasets[i].toJson());
  }
  res.insert("timeseries", datasets);

  return res;
}


/* ********************************************************************************************* *
 * Implementation of DataSetDir
 * ********************************************************************************************* */
DataSetDir::DataSetDir(const QString &directory)
  : _dir(directory), _datasetOrder(), _datasets(), _parents()
{
  reload();
}

QString DataSetDir::path() const {
  return _dir.absolutePath();
}

size_t
DataSetDir::numDatasets() const {
  return _datasets.size();
}

bool
DataSetDir::contains(const Identifier &id) const {
  return _datasets.contains(id);
}

bool
DataSetDir::containsImplicitly(const Identifier &id) const {
  if (_datasets.contains(id)) { return true; }
  return _parents.contains(id);
}

DataSetFile
DataSetDir::dataset(const Identifier &id) const {
  return _datasets[id];
}

bool
DataSetDir::addDataset(const Identifier &id) {
  DataSetFile file(id.toBase32());
  if (! file.isValid()) { return false; }
  if (_datasets.contains(id)) { return true; }
  beginInsertRows(QModelIndex(), _datasetOrder.size(), _datasetOrder.size());
  _datasets.insert(id, file);
  _datasetOrder.append(id);
  endInsertRows();
  return true;
}

bool
DataSetDir::addDataset(const QString &name) {
  Identifier id = Identifier::fromBase32(name);
  if (id.isValid()) {
    return addDataset(id);
  }
  return false;
}

void
DataSetDir::reload() {
  // ensure data dir exists
  if ( (! _dir.exists()) && (! _dir.mkpath(_dir.absolutePath())) ){
    logError() << "Cannot create data directory: " << _dir.absolutePath() << ".";
    return;
  }
  beginResetModel();
  // Reload datasets
  _datasets.clear();
  _datasetOrder.clear();
  _dir.refresh();
  // Get directory content
  QStringList content = _dir.entryList(QDir::Files|QDir::Readable);
  foreach(QString filename, content) {
    addDataset(filename);
  }
  endResetModel();
}

QJsonObject
DataSetDir::toJson() const {
  QJsonObject res;
  QHash<Identifier, DataSetFile>::const_iterator dataset = _datasets.begin();
  for (; dataset != _datasets.end(); dataset++) {
    res.insert(dataset.key().toBase32(), dataset->toJson());
  }
  return res;
}

int
DataSetDir::rowCount(const QModelIndex &parent) const {
  return _datasets.size();
}

int
DataSetDir::columnCount(const QModelIndex &parent) const {
  return 4;
}

QVariant
DataSetDir::data(const QModelIndex &index, int role) const {
  if (index.row() >= _datasets.size()) { return QVariant(); }
  if (index.column() >= columnCount(index)) { return QVariant(); }
  if (Qt::DisplayRole != role) { return QVariant(); }

  if (0 == index.column()) {
    return _datasets[_datasetOrder[index.row()]].datetime().toString();
  } else if (1 == index.column()) {
    return _datasetOrder[index.row()].toBase32();
  } else if (2 == index.column()) {
    return QString::number(_datasets[_datasetOrder[index.row()]].samples() /
        _datasets[_datasetOrder[index.row()]].sampleRate());
  } else if (3 == index.column()) {
    return QString::number(_datasets[_datasetOrder[index.row()]].numTimeseries());
  }

  return QVariant();
}

QVariant
DataSetDir::headerData(int section, Qt::Orientation orientation, int role) const {
  if (section >= 4) { return QVariant(); }
  if (Qt::DisplayRole != role) { return QVariant(); }
  if (Qt::Horizontal != orientation) { return QVariant(); }

  switch (section) {
    case 0: return tr("Timestamp");
    case 1: return tr("Identifier");
    case 2: return tr("Duration [s]");
    case 3: return tr("# Timeseries");
    default: break;
  }

  return QVariant();
}


/* ********************************************************************************************* *
 * Implementation of RemoteTimeseries
 * ********************************************************************************************* */
RemoteTimeseries::RemoteTimeseries()
  : _identifier(), _location()
{
  // pass...
}

RemoteTimeseries::RemoteTimeseries(const QJsonObject &obj)
  : _identifier(), _location()
{
  if (obj.contains("id")) {
    _identifier = Identifier::fromBase32(obj.value("id").toString());
  }
  if (! obj.contains("location")) {
    logError() << "Invalid timeseries: No location specified.";
    return;
  }
  if (! obj.value("location").isObject()) {
    logError() << "Invalid timeseries: Location is not a JSON object.";
    return;
  }
  _location = Location(obj.value("location").toObject());
}

RemoteTimeseries::RemoteTimeseries(const RemoteTimeseries &other)
  : _identifier(other._identifier), _location(other._location)
{
  // pass...
}

RemoteTimeseries &
RemoteTimeseries::operator =(const RemoteTimeseries &other) {
  _identifier = other._identifier;
  _location = other._location;
  return *this;
}

const Identifier &
RemoteTimeseries::identifier() const {
  return _identifier;
}

const Location &
RemoteTimeseries::location() const {
  return _location;
}


/* ********************************************************************************************* *
 * Implementation of RemoteDataSet
 * ********************************************************************************************* */
RemoteDataSet::RemoteDataSet()
  : _timestamp(), _samples(0), _sampleRate(0), _parents(), _timeseries(), _remotes()
{
  // pass...
}

RemoteDataSet::RemoteDataSet(const Identifier &remote, const QJsonObject &obj)
  : _timestamp(), _samples(0), _sampleRate(0), _parents(), _timeseries(), _remotes()
{
  _remotes.insert(remote);
  if (obj.contains("timestamp")) {
    QDateTime timestamp = QDateTime::fromString(obj.value("timestamp").toString(),"yyyy-MM-dd hh:mm:ss");
    timestamp.setTimeSpec(Qt::UTC);
    _timestamp = timestamp.toLocalTime();
  }
  if (obj.contains("samples")) {
    _samples = obj.value("samples").toInt();
  }
  if (obj.contains("samplerate")) {
    _sampleRate = obj.value("samplerate").toInt();
  }
  if (obj.contains("timeseries") && obj.value("timeseries").isArray()) {
    QJsonArray timeseries = obj.value("timeseries").toArray();
    for (int i=0; i<timeseries.size(); i++) {
      RemoteTimeseries ts(timeseries.at(i).toObject());
      if (! ts.location().isNull())
        _timeseries.append(ts);
    }
  }
}

RemoteDataSet::RemoteDataSet(const RemoteDataSet &other)
  : _timestamp(other._timestamp), _samples(other._samples), _sampleRate(other._sampleRate),
    _parents(other._parents), _timeseries(other._timeseries), _remotes(other._remotes)
{
  // pass...
}

RemoteDataSet &
RemoteDataSet::operator =(const RemoteDataSet &other) {
  _timestamp = other._timestamp;
  _samples = other._samples;
  _sampleRate = other._sampleRate;
  _parents = other._parents;
  _timeseries = other._timeseries;
  _remotes = other._remotes;
  return *this;
}

const QDateTime &
RemoteDataSet::datetime() const {
  return _timestamp;
}

size_t
RemoteDataSet::samples() const {
  return _samples;
}

size_t
RemoteDataSet::sampleRate() const {
  return _sampleRate;
}


size_t
RemoteDataSet::numTimeseries() const {
  return _timeseries.size();
}

const RemoteTimeseries &
RemoteDataSet::timeseries(size_t i) const {
  return _timeseries[i];
}

size_t
RemoteDataSet::numRemotes() const {
  return _remotes.size();
}

void
RemoteDataSet::addRemote(const Identifier &remote) {
  _remotes.insert(remote);
}

const QSet<Identifier> &
RemoteDataSet::remotes() const {
  return _remotes;
}


/* ********************************************************************************************* *
 * Implementation of RemoteDataSetList
 * ********************************************************************************************* */
RemoteDataSetList::RemoteDataSetList(Station &station, QObject *parent)
  : QAbstractTableModel(parent), _station(station), _datasetOrder(), _datasets()
{
  // Get notified if a station got updated
  connect(&_station.stations(), SIGNAL(stationUpdated(StationItem)),
          this, SLOT(_onUpdateRemoteDataSets(StationItem)));
}

size_t
RemoteDataSetList::numDatasets() const {
  return _datasetOrder.size();
}

bool
RemoteDataSetList::contains(const Identifier &id) const {
  return _datasets.contains(id);
}

RemoteDataSet
RemoteDataSetList::dataset(const Identifier &id) const {
  return _datasets[id];
}

void
RemoteDataSetList::add(const Identifier &remote, const QJsonObject &list) {
  QJsonObject::const_iterator entry = list.begin();
  for (; entry != list.end(); entry++) {
    if (_datasets.contains(Identifier::fromBase32(entry.key()))) {
      _datasets[Identifier::fromBase32(entry.key())].addRemote(remote);
      int idx = _datasetOrder.indexOf(Identifier::fromBase32(entry.key()));
      emit dataChanged(index(idx, 0),index(idx, 4));
    } else {
      beginInsertRows(QModelIndex(), _datasetOrder.size(), _datasetOrder.size());
      _datasets.insert(Identifier::fromBase32(entry.key()),
                       RemoteDataSet(remote, entry.value().toObject()));
      _datasetOrder.append(Identifier::fromBase32(entry.key()));
      endInsertRows();
    }
  }
}

int
RemoteDataSetList::rowCount(const QModelIndex &parent) const {
  return _datasetOrder.size();
}

int
RemoteDataSetList::columnCount(const QModelIndex &parent) const {
  return 5;
}

QVariant
RemoteDataSetList::data(const QModelIndex &index, int role) const {
  if (index.row() >= _datasetOrder.size()) { return QVariant(); }
  if (index.column() >= columnCount(QModelIndex())) { return QVariant(); }
  if (Qt::DisplayRole != role) { return QVariant(); }

  RemoteDataSet dataset = _datasets[_datasetOrder[index.row()]];
  if (0 == index.column()) {
    return dataset.datetime().toString();
  } else if (1 == index.column()) {
    return _datasetOrder[index.row()].toBase32();
  } else if (2 == index.column()) {
    return QString::number(double(dataset.samples())/dataset.sampleRate(), 'f', '1');
  } else if (3 == index.column()) {
    return int(dataset.numTimeseries());
  } else if (4 == index.column()) {
    return int(dataset.numRemotes());
  }

  return QVariant();
}

QVariant
RemoteDataSetList::headerData(int section, Qt::Orientation orientation, int role) const {

  if (section>=columnCount(QModelIndex())) { return QVariant(); }
  if (Qt::Horizontal != orientation) { return QVariant(); }
  if (Qt::DisplayRole != role) { return QVariant(); }

  switch (section) {
    case 0: return tr("Timestamp");
    case 1: return tr("Identifier");
    case 2: return tr("Duration [s]");
    case 3: return tr("# Timeseries");
    case 4: return tr("# Remotes");
    default: break;
  }

  return QVariant();
}

void
RemoteDataSetList::_onUpdateRemoteDataSets(const StationItem &station) {
  // logDebug() << "Station " << station.id() << " updated -> Get dataset list.";
  DataSetListQuery *query = new DataSetListQuery(_station, station.node());
  connect(query, SIGNAL(dataSetListReceived(Identifier,QJsonObject)),
          this, SLOT(add(Identifier,QJsonObject)));
}
