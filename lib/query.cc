#include "query.hh"
#include "station.hh"
#include "stationlist.hh"
#include <QJsonObject>
#include <QJsonArray>
#include <QTemporaryFile>


/* ********************************************************************************************* *
 * Implementation of StationResolveQuery
 * ********************************************************************************************* */
StationResolveQuery::StationResolveQuery(Node &node, const Identifier &id)
  : QObject(), SearchQuery(id)
{
  node.findNode(this);
}

void
StationResolveQuery::succeeded() {
  emit found(first());
  SearchQuery::succeeded();
}

void
StationResolveQuery::failed() {
  emit notFound();
  this->deleteLater();
}


/* ********************************************************************************************* *
 * Implementation of JsonQuery
 * ********************************************************************************************* */
JsonQuery::JsonQuery(const QString &path, Node &node, const Identifier &remote)
  : QObject(0), _query(path), _node(node), _remoteId(remote)
{
  StationResolveQuery *query = new StationResolveQuery(_node, remote);
  connect(query, SIGNAL(found(NodeItem)), this, SLOT(_onNodeFound(NodeItem)));
  connect(query, SIGNAL(notFound()), this, SLOT(_onError()));
}

JsonQuery::JsonQuery(const QString &path, Node &node, const NodeItem &remote)
  : QObject(0), _query(path), _node(node), _remoteId(remote.id())
{
  _onNodeFound(remote);
}

void
JsonQuery::_onNodeFound(const NodeItem &node) {
  /*logDebug() << "Try to connect station '" << node.id().toBase32()
             << "' for '" << _query << "'."; */
  _connection = new HttpClientConnection(_node, node, "vlf::station", this);
  connect(_connection, SIGNAL(established()), this, SLOT(_onConnectionEstablished()));
  connect(_connection, SIGNAL(error()), this, SLOT(_onError()));
}

void
JsonQuery::_onConnectionEstablished() {
  /*logDebug() << "Try to query '" << _query
             << "' from station '" << _connection->peerId() << "'."; */
  _response = _connection->get(_query);
  if (_response) {
    connect(_response, SIGNAL(finished()), this, SLOT(_onResponseReceived()));
    connect(_response, SIGNAL(error()), this, SLOT(_onError()));
  } else {
    _onError();
  }
}

void
JsonQuery::_onResponseReceived() {
  if (HTTP_OK != _response->responseCode()) {
    logError() << "Cannot query '" << _query << "': Station returned " << _response->responseCode();
    _onError(); return;
  }
  if (! _response->hasResponseHeader("Content-Length")) {
    logError() << "Station response has no length!";
    _onError(); return;
  }

  _responseLength = _response->responseHeader("Content-Length").toUInt();
  connect(_response, SIGNAL(readyRead()), this, SLOT(_onReadyRead()));
}

void
JsonQuery::_onError() {
  logError() << "Failed to access " << _query << " at " << _remoteId.toBase32() << ".";
  emit failed();
  deleteLater();
}

void
JsonQuery::_onReadyRead() {
  if (_responseLength) {
    QByteArray tmp = _response->read(_responseLength);
    _buffer.append(tmp); _responseLength -= tmp.size();
  }

  if (0 == _responseLength) {
    // Response complete
    QJsonParseError error;
    QJsonDocument doc = QJsonDocument::fromJson(_buffer, &error);
    if (QJsonParseError::NoError != error.error) {
      logInfo() << "Station returned invalid JSON document as result.";
      _onError(); return;
    }
    this->finished(doc);
  }
}

void
JsonQuery::finished(const QJsonDocument &doc) {
  this->deleteLater();
}


/* ********************************************************************************************* *
 * Implementation of StationInfoQuery
 * ********************************************************************************************* */
StationInfoQuery::StationInfoQuery(Node &node, const Identifier &remote)
  : JsonQuery("/status", node, remote)
{
  // pass...
}

StationInfoQuery::StationInfoQuery(Node &node, const NodeItem &remote)
  : JsonQuery("/status", node, remote)
{
  // pass...
}

void
StationInfoQuery::finished(const QJsonDocument &doc) {
  if (! doc.isObject()) {
    logError() << "Station returned invalid JSON description: Not an object.";
    _onError(); return;
  }

  StationItem item(NodeItem(_connection->peerId(), _connection->peer()), doc.object());
  if (item.isNull()) {
    logError() << "Station returned invalid status.";
    _onError(); return;
  }

  emit stationInfoReceived(item);
  JsonQuery::finished(doc);
}


/* ********************************************************************************************* *
 * Implementation of StationListQuery
 * ********************************************************************************************* */
StationListQuery::StationListQuery(Node &node, const Identifier &remote)
  : JsonQuery("/list", node, remote)
{
  // pass...
}

StationListQuery::StationListQuery(Node &node, const NodeItem &remote)
  : JsonQuery("/list", node, remote)
{
  // pass...
}

void
StationListQuery::finished(const QJsonDocument &doc) {
  if (! doc.isArray()) {
    logError() << "Station returned invalid JSON description: Not an array.";
    _onError(); return;
  }

  QList<Identifier> ids;
  QJsonArray array = doc.array();
  for (int i=0; i<array.size(); i++) {
    ids.push_back(Identifier::fromBase32(array.at(i).toString()));
  }

  emit stationListReceived(ids);
  JsonQuery::finished(doc);
}


/* ********************************************************************************************* *
 * Implementation of StationScheduleQuery
 * ********************************************************************************************* */
StationScheduleQuery::StationScheduleQuery(Node &node, const Identifier &remote)
  : JsonQuery("/schedule", node, remote)
{
  // pass...
}

StationScheduleQuery::StationScheduleQuery(Node &node, const NodeItem &remote)
  : JsonQuery("/schedule", node, remote)
{
  // pass...
}

void
StationScheduleQuery::finished(const QJsonDocument &doc) {
  if (! doc.isArray()) {
    logError() << "Station returned invalid JSON description: Not an array.";
    _onError(); return;
  }

  QList<ScheduledEvent> events;
  QJsonArray array = doc.array();
  for (int i=0; i<array.size(); i++) {
    if (array.at(i).isObject()) {
      events.push_back(ScheduledEvent(array.at(i).toObject()));
    }
  }
  emit stationScheduleReceived(_connection->peerId(), events);
  JsonQuery::finished(doc);
}


/* ********************************************************************************************* *
 * Implementation of StationDataSetListQuery
 * ********************************************************************************************* */
DataSetListQuery::DataSetListQuery(Node &node, const Identifier &remote)
  : JsonQuery("/data", node, remote)
{
  // pass...
}

DataSetListQuery::DataSetListQuery(Node &node, const NodeItem &remote)
  : JsonQuery("/data", node, remote)
{
  // pass...
}

void
DataSetListQuery::finished(const QJsonDocument &doc) {
  if (! doc.isObject()) {
    logError() << "Station returned invalid dataset list: Not an object.";
    _onError(); return;
  }
  emit dataSetListReceived(_connection->peerId(), doc.object());
  JsonQuery::finished(doc);
}


/* ********************************************************************************************* *
 * Implementation of DownloadDataSetQuery
 * ********************************************************************************************* */
DownloadDataSetQuery::DownloadDataSetQuery(const Identifier &dataSetId, const Identifier &remote,
                                           Station &station)
  : QObject(0), _station(station), _dataSetID(dataSetId)
{
  StationResolveQuery *query = new StationResolveQuery(_station, remote);
  connect(query, SIGNAL(found(NodeItem)), this, SLOT(_onNodeFound(NodeItem)));
  connect(query, SIGNAL(notFound()), this, SLOT(_onError()));
}

DownloadDataSetQuery::DownloadDataSetQuery(const Identifier &dataSetId, const NodeItem &remote,
                                           Station &station)
  : QObject(0), _station(station), _dataSetID(dataSetId)
{
  _onNodeFound(remote);
}

void
DownloadDataSetQuery::_onNodeFound(const NodeItem &node) {
  logDebug() << "Try to connect station '" << node.id()
             << "' for dataset '" << _dataSetID << "'.";
  _connection = new HttpClientConnection(_station, node, "vlf::station", this);
  connect(_connection, SIGNAL(established()), this, SLOT(_onConnectionEstablished()));
  connect(_connection, SIGNAL(error()), this, SLOT(_onError()));
}

void
DownloadDataSetQuery::_onConnectionEstablished() {
  logDebug() << "Try to download dataset '" << _dataSetID
             << "' from station '" << _connection->peerId() << "'.";

  _response = _connection->get("/data/"+_dataSetID.toBase32());
  if (_response) {
    connect(_response, SIGNAL(finished()), this, SLOT(_onResponseReceived()));
    connect(_response, SIGNAL(error()), this, SLOT(_onError()));
  } else {
    _onError();
  }
}

void
DownloadDataSetQuery::_onResponseReceived() {
  if (HTTP_OK != _response->responseCode()) {
    logError() << "Cannot query dataset '" << _dataSetID
               << "': Station returned " << _response->responseCode();
    _onError();
    return;
  }
  if (! _response->hasResponseHeader("Content-Length")) {
    logError() << "Station response has no length!";
    _onError(); return;
  }

  _responseLength = _response->responseHeader("Content-Length").toUInt();
  _buffer.open();
  OVLHashInit(&_mdctx);
  connect(_response, SIGNAL(readyRead()), this, SLOT(_onReadyRead()));
}

void
DownloadDataSetQuery::_onError() {
  logError() << "Failed to access dataset '" << _dataSetID << "'.";
  emit failed();
  deleteLater();
}

void
DownloadDataSetQuery::_onReadyRead() {
  if (_responseLength) {
    QByteArray tmp = _response->read(_responseLength);
    if (tmp.size() != _buffer.write(tmp)) {
      logError() << "Cannot write to temporary file " << _buffer.fileName() << ".";
    }
    OVLHashUpdate((const uint8_t *)tmp.constData(), tmp.size(), &_mdctx);
    _responseLength -= tmp.size();
  }

  if (0 == _responseLength) {
    _buffer.flush();
    _buffer.close();
    char hash[OVL_HASH_SIZE];
    OVLHashFinal(&_mdctx, (uint8_t *)hash);
    // Check hash
    if (Identifier(hash) != _dataSetID) {
      logError() << "Dataset ID '" << _dataSetID
                 << "' does not match hash of downloaded dataset '" << Identifier(hash) << "'.";
      _onError(); return;
    }
    if (! _buffer.copy(_station.datasets().path()+"/"+_dataSetID.toBase32())) {
      logError() << "Failed to copy downloaded dataset file to '"
                 << _station.datasets().path()+"/"+_dataSetID.toBase32() <<"'.";
      _onError(); return;
    }
    _station.datasets().addDataset(_dataSetID);
    emit succeeded();
  }
}


