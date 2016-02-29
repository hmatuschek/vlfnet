#include "station.hh"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

#include <ovlnet/node.hh>

#include "location.hh"
#include "logwindow.hh"
#include "stationlist.hh"


/* ********************************************************************************************* *
 * Implementation of Station
 * ********************************************************************************************* */
Station::Station(Identity &id, const Location &location, const QHostAddress &addr, uint16_t port, QObject *parent)
  : Node(id, addr, port, parent), HttpRequestHandler(), _location(location), _stations(0)
{
  _stations = new StationList(*this);
  registerService("vlf::station", new HttpService(*this, this, this));
}

const Location &
Station::location() const {
  return _location;
}

void
Station::setLocation(const Location &loc) {
  _location = loc;
}

StationList &
Station::stations() {
  return *_stations;
}

bool
Station::acceptReqest(HttpRequest *request) {
  if ((HTTP_GET == request->method()) && ("/status" == request->uri().path())) {
    return true;
  }
  if ((HTTP_GET == request->method()) && ("/list" == request->uri().path())) {
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
  } else if ((HTTP_GET == request->method()) && ("/status" == request->uri().path())) {
    // Handle stations list request
    QJsonArray stations;
    for (size_t i=0; i<_stations->numStations(); i++) {
      stations.append(_stations->station(i).id().toBase32());
    }
    return new HttpJsonResponse(QJsonDocument(stations), request);
  }

  // Unknown request -> send a 404
  return new HttpStringResponse(
        request->version(), HTTP_NOT_FOUND, "Not found", request->socket());
}
