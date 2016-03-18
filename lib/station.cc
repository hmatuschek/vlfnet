#include "station.hh"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QAudioDeviceInfo>

#include <ovlnet/node.hh>

#include "location.hh"
#include "stationlist.hh"
#include "schedule.hh"
#include "receiver.hh"
#include "bootstraplist.hh"


/* ********************************************************************************************* *
 * Implementation of Station
 * ********************************************************************************************* */
Station::Station(const QString &path, const QHostAddress &addr, uint16_t port, QObject *parent)
  : Node(path+"/identity.pem", addr, port, parent), HttpRequestHandler(), _path(path),
    _location(Location::fromFile(_path+"/location.json")), _stations(0),
    _schedule(0), _datasets(0), _receiver(0), _bootstrapTimer(), _ctrlWhitelist()
{
  _stations = new StationList(*this);
  _schedule = new MergedSchedule(_path+"/schedule.json", *this, 28, this);
  _datasets = new DataSetDir(_path+"/data");

  // Create receiver...
  _receiver = new Receiver(*this, ReceiverConfig(_path+"/receiver.json"), this);

  // Register service
  registerService("vlf::station", new HttpService(*this, this, this));

  // Setup boostrap timer
  _bootstrapTimer.setInterval(60*1000);
  _bootstrapTimer.setSingleShot(false);
  connect(&_bootstrapTimer, SIGNAL(timeout()), this, SLOT(_onBootstrap()));
  _bootstrapTimer.start();
  // try to boostrap immediately
  _onBootstrap();

  connect(this, SIGNAL(connected()), this, SLOT(_onConnected()));
  connect(this, SIGNAL(disconnected()), this, SLOT(_onDisconnected()));

  // Whenever a new node appears in the OVL network, check if this one is a Station (i.e. a
  // member of the VLF network).
  connect(this, SIGNAL(nodeAppeard(NodeItem)), _stations, SLOT(contactStation(NodeItem)));
  // Start scheduled recording
  connect(_schedule, SIGNAL(startRecording(double)), _receiver, SLOT(start(double)));
}

const Location &
Station::location() const {
  return _location;
}

void
Station::setLocation(const Location &loc) {
  _location = loc;
  // Save location into file.
  QFile file(_path+"/location.json");
  if (! file.open(QIODevice::WriteOnly)) {
    logDebug() << "Cannot save location to file " << _path+"/location.json";
    return;
  }
  file.write(QJsonDocument(_location.toJson()).toJson());
  file.flush(); file.close();
}

StationList &
Station::stations() {
  return *_stations;
}

MergedSchedule &
Station::schedule() {
  return *_schedule;
}

DataSetDir &
Station::datasets() {
  return *_datasets;
}

QAudioDeviceInfo
Station::inputDevice() const {
  return _receiver->device();
}

bool
Station::setInputDevice(const QAudioDeviceInfo &device) {
  _receiver->setDevice(device);
  ReceiverConfig cfg;
  cfg.setDevice(device);
  cfg.save(_path+"/receiver.json");
  return true;
}

bool
Station::acceptReqest(HttpRequest *request) {
  if ((HTTP_GET == request->method()) && ("/status" == request->uri().path())) {
    return true;
  }
  if ((HTTP_GET == request->method()) && ("/list" == request->uri().path())) {
    return true;
  }
  if ((HTTP_GET == request->method()) && ("/schedule" == request->uri().path())) {
    return true;
  }
  if ((HTTP_GET == request->method()) && request->uri().path().startsWith("/data")) {
    return true;
  }
  if (request->uri().path().startsWith("/ctrl/") && _ctrlWhitelist.contains(request->remote().id())) {
    return true;
  }
  return false;
}

HttpResponse *
Station::processRequest(HttpRequest *request) {
  if ((HTTP_GET == request->method()) && ("/status" == request->uri().path())) {
    // Handle station info request
    // Assemble location
    QJsonObject location;
    location.insert("longitude", _location.longitude());
    location.insert("latitude", _location.latitude());
    location.insert("height", _location.height());
    // Assemble result
    QJsonObject result;
    result.insert("id", id().toBase32());
    result.insert("location", location);
    // Send response as JSON
    return new HttpJsonResponse(QJsonDocument(result), request);
  } else if ((HTTP_GET == request->method()) && ("/list" == request->uri().path())) {
    // Handle stations list request
    QJsonArray stations;
    for (size_t i=0; i<_stations->numStations(); i++) {
      stations.append(_stations->station(i).id().toBase32());
    }
    return new HttpJsonResponse(QJsonDocument(stations), request);
  } else if ((HTTP_GET == request->method()) && ("/schedule" == request->uri().path())) {
    // Handle schedule request
    QJsonArray schedule;
    for (size_t i=0; i<_schedule->numEvents(); i++) {
      schedule.append(_schedule->scheduledEvent(i).toJson());
    }
    return new HttpJsonResponse(QJsonDocument(schedule), request);
  } else if ((HTTP_GET == request->method()) && ("/data" == request->uri().path())) {
    // Handle dataset list queries
    return new HttpJsonResponse(QJsonDocument(_datasets->toJson()), request);
  } else if ((HTTP_GET == request->method()) && request->uri().path().startsWith("/data")) {
    // Handle data download queries
    QString id = request->uri().path().mid(6);
    if (! _datasets->contains(Identifier::fromBase32(id))) {
      return new HttpStringResponse(request->version(), HTTP_NOT_FOUND, "Not found.",
                                    request->socket());
    }
    // serve file
    return new HttpFileResponse(
          _datasets->dataset(Identifier::fromBase32(id)).filename(), request);
  } else if (request->uri().path().startsWith("/ctrl/") &&
             _ctrlWhitelist.contains(request->remote().id())) {
    // Handle CTRL requests
    return new CtrlResponse(*this, request);
  }

  // Unknown request -> send a 404
  return new HttpStringResponse(
        request->version(), HTTP_NOT_FOUND, "Not found", request->socket());
}

void
Station::_onBootstrap() {
  // boostrap list
  QList< QPair<QString, uint16_t> > boothost = BootstrapList::fromFile(
        _path+"/bootstrap.json");
  QList<QPair<QString, uint16_t>>::iterator host = boothost.begin();
  for (; host != boothost.end(); host++) {
    logDebug() << "Try to bootstrap network with " << host->first << ": " << host->second << ".";
    ping(host->first, host->second);
  }
}

void
Station::_onConnected() {
  _bootstrapTimer.stop();
}

void
Station::_onDisconnected() {
  _bootstrapTimer.start();
}

/* ********************************************************************************************* *
 * Implementation of CtrlResponse
 * ********************************************************************************************* */
CtrlResponse::CtrlResponse(Station &station, HttpRequest *request)
  : HttpResponse(request->version(), HTTP_RESP_INCOMPLETE, request->socket()),
    _station(station), _path(request->uri().path().mid(6)), _requestSize(0), _buffer()
{
  connect(_socket, SIGNAL(readChannelFinished()), this, SIGNAL(completed()));
  if (HTTP_POST != request->method()) { return; }
  if (! request->hasHeader("Content-Length")) {
    logInfo() << "HTTP_POST w/o Content-Length.";
    this->setResponseCode(HTTP_BAD_REQUEST);
    this->sendHeaders();
    return;
  }

  bool ok; _requestSize = request->header("Content-Length").toUInt(&ok);
  if (! ok) {
    logInfo() << "Malformed Content-Length header: '" << request->header("Content-Length") << "'.";
    this->setResponseCode(HTTP_BAD_REQUEST);
    this->sendHeaders();
    return;
  }

  connect(_socket, SIGNAL(readyRead()), this, SLOT(_onReadyRead()));
  _onReadyRead();
}

void
CtrlResponse::process(const QJsonDocument &doc) {
  /// @todo Implement JSON RPC for remote ctrl.
  setResponseCode(HTTP_NOT_FOUND);
}

void
CtrlResponse::_onReadyRead() {
  while (_socket->bytesAvailable() && _requestSize) {
    QByteArray tmp = _socket->read(_requestSize);
    _requestSize -= tmp.size();
    _buffer.append(tmp);
  }
  if (0 == _requestSize) {
    disconnect(_socket, SIGNAL(readyRead()), this, SLOT(_onReadyRead()));
    QJsonDocument doc;
    if (_buffer.size()) {
      QJsonParseError err;
      doc.fromJson(_buffer, &err);
      _buffer.clear();
      if (QJsonParseError::NoError != err.error) {
        logInfo() << "JsonParserError: " << err.errorString();
        this->setResponseCode(HTTP_BAD_REQUEST);
        this->sendHeaders();
        return;
      }
    }
    process(doc);
    this->sendHeaders();
  }
}

void
CtrlResponse::_onHeadersSend() {
  if (_buffer.size()) {
    connect(_socket, SIGNAL(bytesWritten(qint64)), this, SLOT(_onBytesWritten(qint64)));
    _onBytesWritten(0);
  } else {
    emit completed();
  }
}

void
CtrlResponse::_onBytesWritten(qint64 n) {
  if (_buffer.size()) {
    _buffer = _buffer.mid(n);
    _socket->write(_buffer);
  } else {
    emit completed();
  }
}
