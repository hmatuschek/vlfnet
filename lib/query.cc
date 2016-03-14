#include "query.hh"
#include "stationlist.hh"
#include <QJsonObject>
#include <QJsonArray>


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
  //SearchQuery::failed();
  this->deleteLater();
}


/* ********************************************************************************************* *
 * Implementation of JsonQuery
 * ********************************************************************************************* */
JsonQuery::JsonQuery(const QString &path, Node &node, const Identifier &remote)
  : QObject(0), _query(path), _node(node)
{
  StationResolveQuery *query = new StationResolveQuery(_node, remote);
  connect(query, SIGNAL(found(NodeItem)), this, SLOT(_onNodeFound(NodeItem)));
  connect(query, SIGNAL(notFound()), this, SLOT(_onError()));
}

JsonQuery::JsonQuery(const QString &path, Node &node, const NodeItem &remote)
  : QObject(0), _query(path), _node(node)
{
  _onNodeFound(remote);
}

void
JsonQuery::_onNodeFound(const NodeItem &node) {
  _connection = new HttpClientConnection(_node, node, "vlf::station", this);
  connect(_connection, SIGNAL(established()), this, SLOT(_onConnectionEstablished()));
  connect(_connection, SIGNAL(error()), this, SLOT(_onError()));
}

void
JsonQuery::_onConnectionEstablished() {
  _response = _connection->get(_query);
  connect(_response, SIGNAL(finished()), this, SLOT(_onResponseReceived()));
  connect(_response, SIGNAL(error()), this, SLOT(_onError()));
}

void
JsonQuery::_onResponseReceived() {
  if (HTTP_OK != _response->responseCode()) {
    logDebug() << "Cannot query '" << _query << "': Station returned " << _response->responseCode();
    _onError(); return;
  }
  if (! _response->hasResponseHeader("Content-Length")) {
    logDebug() << "Station response has no length!";
    _onError(); return;
  }

  _responseLength = _response->responseHeader("Content-Length").toUInt();
  connect(_response, SIGNAL(readyRead()), this, SLOT(_onReadyRead()));
}

void
JsonQuery::_onError() {
  logDebug() << "Failed to access station.";
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
      logDebug() << "Station returned invalid JSON document as result.";
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
    logDebug() << "Station returned invalid JSON description: Not an object.";
    _onError(); return;
  }

  StationItem item(NodeItem(_connection->peerId(), _connection->peer()), doc.object());
  if (item.isNull()) {
    qDebug() << "Station returned invalid status: " << doc.toJson();
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
    logDebug() << "Station returned invalid JSON description: Not an array.";
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
  _onNodeFound(remote);
}

void
StationScheduleQuery::finished(const QJsonDocument &doc) {
  if (! doc.isArray()) {
    logDebug() << "Station returned invalid JSON description: Not an array.";
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
StationDataSetListQuery::StationDataSetListQuery(Node &node, const Identifier &remote)
  : JsonQuery("/data", node, remote)
{
  // pass...
}

StationDataSetListQuery::StationDataSetListQuery(Node &node, const NodeItem &remote)
  : JsonQuery("/data", node, remote)
{
  _onNodeFound(remote);
}

void
StationDataSetListQuery::finished(const QJsonDocument &doc) {
  if (! doc.isObject()) {
    logDebug() << "Station returned invalid JSON description: Not an object.";
    _onError(); return;
  }

  emit dataSetListReceived(_connection->peerId(), doc.object());
  JsonQuery::finished(doc);
}

