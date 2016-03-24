#include "station.hh"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QAudioDeviceInfo>

#include <ovlnet/node.hh>
#include <ovlnet/socks.hh>

#include "location.hh"
#include "stationlist.hh"
#include "schedule.hh"
#include "receiver.hh"
#include "bootstraplist.hh"
#include "socksservice.hh"


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
  registerService("vlf::station", new HttpService(*this, this));
  registerService("::socks", new SocksService(_path+"/sockswhitelist.json", *this));

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
