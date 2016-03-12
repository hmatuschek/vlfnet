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
  SearchQuery::failed();
}


/* ********************************************************************************************* *
 * Implementation of StationInfoQuery
 * ********************************************************************************************* */
StationInfoQuery::StationInfoQuery(Node &node, const Identifier &remote)
  : QObject(0), _node(node)
{
  StationResolveQuery *query = new StationResolveQuery(_node, remote);
  connect(query, SIGNAL(found(NodeItem)), this, SLOT(_onNodeFound(NodeItem)));
  connect(query, SIGNAL(notFound()), this, SLOT(_onError()));
}

StationInfoQuery::StationInfoQuery(Node &node, const NodeItem &remote)
  : QObject(0), _node(node)
{
  _onNodeFound(remote);
}

void
StationInfoQuery::_onNodeFound(const NodeItem &node) {
  logDebug() << "Node found: Connect...";
  _connection = new HttpClientConnection(_node, node, "vlf::station", this);
  connect(_connection, SIGNAL(established()), this, SLOT(_onConnectionEstablished()));
  connect(_connection, SIGNAL(error()), this, SLOT(_onError()));
}

void
StationInfoQuery::_onConnectionEstablished() {
  logDebug() << "Connection established: Request status.";
  _response = _connection->get("/status");
  connect(_response, SIGNAL(finished()), this, SLOT(_onResponseReceived()));
  connect(_response, SIGNAL(error()), this, SLOT(_onError()));
}

void
StationInfoQuery::_onResponseReceived() {
  if (HTTP_OK != _response->responseCode()) {
    logDebug() << "Cannot query station status: Station returned " << _response->responseCode();
    _onError(); return;
  }
  if (! _response->hasResponseHeader("Content-Length")) {
    logDebug() << "Station response has no length!";
    _onError(); return;
  }

  _responseLength = _response->responseHeader("Content-Length").toUInt();
  logDebug() << "Receive " << _responseLength << "b response.";
  connect(_response, SIGNAL(readyRead()), this, SLOT(_onReadyRead()));
  _onReadyRead();
}

void
StationInfoQuery::_onError() {
  logDebug() << "Failed to access station.";
  emit failed();
  deleteLater();
}

void
StationInfoQuery::_onReadyRead() {
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

    if (! doc.isObject()) {
      logDebug() << "Station returned invalid JSON description: Not an object.";
      _onError(); return;
    }

    StationItem item(NodeItem(_connection->peerId(), _connection->peer()), doc.object());
    if (! item.isNull()) {
      _onError(); return;
    }

    emit stationInfoReceived(item);
    deleteLater();
  }
}


/* ********************************************************************************************* *
 * Implementation of StationListQuery
 * ********************************************************************************************* */
StationListQuery::StationListQuery(Node &node, const Identifier &remote)
  : QObject(0), _node(node)
{
  StationResolveQuery *query = new StationResolveQuery(_node, remote);
  connect(query, SIGNAL(found(NodeItem)), this, SLOT(_onNodeFound(NodeItem)));
  connect(query, SIGNAL(notFound()), this, SLOT(_onError()));
}

StationListQuery::StationListQuery(Node &node, const NodeItem &remote)
  : QObject(0), _node(node)
{
  _onNodeFound(remote);
}

void
StationListQuery::_onNodeFound(const NodeItem &node) {
  _connection = new HttpClientConnection(_node, node, "vlf::station", this);
  connect(_connection, SIGNAL(established()), this, SLOT(_onConnectionEstablished()));
  connect(_connection, SIGNAL(error()), this, SLOT(_onError()));
}

void
StationListQuery::_onConnectionEstablished() {
  _response = _connection->get("/list");
  connect(_response, SIGNAL(finished()), this, SLOT(_onResponseReceived()));
  connect(_response, SIGNAL(error()), this, SLOT(_onError()));
}

void
StationListQuery::_onResponseReceived() {
  if (HTTP_OK != _response->responseCode()) {
    logDebug() << "Cannot query station list: Station returned " << _response->responseCode();
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
StationListQuery::_onError() {
  logDebug() << "Failed to access station.";
  emit failed();
  deleteLater();
}

void
StationListQuery::_onReadyRead() {
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
    deleteLater();
  }
}


/* ********************************************************************************************* *
 * Implementation of StationScheduleQuery
 * ********************************************************************************************* */
StationScheduleQuery::StationScheduleQuery(Node &node, const Identifier &remote)
  : QObject(0), _node(node)
{
  StationResolveQuery *query = new StationResolveQuery(_node, remote);
  connect(query, SIGNAL(found(NodeItem)), this, SLOT(_onNodeFound(NodeItem)));
  connect(query, SIGNAL(notFound()), this, SLOT(_onError()));
}

StationScheduleQuery::StationScheduleQuery(Node &node, const NodeItem &remote)
  : QObject(0), _node(node)
{
  _onNodeFound(remote);
}

void
StationScheduleQuery::_onNodeFound(const NodeItem &node) {
  _connection = new HttpClientConnection(_node, node, "vlf::station", this);
  connect(_connection, SIGNAL(established()), this, SLOT(_onConnectionEstablished()));
  connect(_connection, SIGNAL(error()), this, SLOT(_onError()));
}

void
StationScheduleQuery::_onConnectionEstablished() {
  _response = _connection->get("/schedule");
  connect(_response, SIGNAL(finished()), this, SLOT(_onResponseReceived()));
  connect(_response, SIGNAL(error()), this, SLOT(_onError()));
}

void
StationScheduleQuery::_onResponseReceived() {
  if (HTTP_OK != _response->responseCode()) {
    logDebug() << "Cannot query station schedule: Station returned " << _response->responseCode();
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
StationScheduleQuery::_onError() {
  logDebug() << "Failed to access station.";
  emit failed();
  deleteLater();
}

void
StationScheduleQuery::_onReadyRead() {
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
    emit stationScheduleReceived(events);
    deleteLater();
  }
}

