#include "station.hh"

#include <ovlnet/node.hh>
#include "logwindow.hh"
#include "stationlist.hh"

#include <QDir>
#include <QStandardPaths>


/* ********************************************************************************************* *
 * Implementation of StationItem
 * ********************************************************************************************* */
StationItem::StationItem()
  : _lastSeen(), _node(), _location(), _description()
{
  // pass...
}

StationItem::StationItem(const Identifier &id, const Location &location, const QString &descr)
  : _lastSeen(QDateTime::currentDateTime()), _node(id, QHostAddress(), 0), _location(location),
    _description(descr)
{
  // pass...
}

StationItem::StationItem(const NodeItem &node, const Location &location, const QString &descr)
  : _lastSeen(QDateTime::currentDateTime()), _node(node), _location(location), _description(descr)
{
  // pass...
}

StationItem::StationItem(const StationItem &other)
  : _lastSeen(other._lastSeen), _node(other._node), _location(other._location),
    _description(other._description)
{
  // pass...
}

StationItem &
StationItem::operator =(const StationItem &other) {
  _lastSeen = other._lastSeen;
  _node = other._node;
  _location = other._location;
  _description = other._description;
  return *this;
}

bool
StationItem::isNull() const {
  return _node.id().isEmpty();
}

const QDateTime &
StationItem::lastSeen() const {
  return _lastSeen;
}

const Identifier &
StationItem::id() const {
  return _node.id();
}

const NodeItem &
StationItem::node() const {
  return _node;
}

const PeerItem &
StationItem::peer() const {
  return _node;
}

const Location &
StationItem::location() const {
  return _location;
}

const QString &
StationItem::description() const {
  return _description;
}

void
StationItem::update(const PeerItem &peer) {
  _node = NodeItem(_node.id(), peer);
  _lastSeen = QDateTime::currentDateTime();
}


/* ********************************************************************************************* *
 * Implementation of Station
 * ********************************************************************************************* */
Station::Station(int &argc, char **argv)
  : QApplication(argc, argv), _identity(0), _node(0), _logmodel(0), _stations(0)
{
  // Create log model
  _logmodel = new LogModel();
  Logger::addHandler(_logmodel);

  // Set application name
  setApplicationName("ovlclient");
  setOrganizationName("io.github.hmatuschek");
  setOrganizationDomain("io.github.hmatuschek");

  // Try to load identity from file
  QDir nodeDir = QStandardPaths::writableLocation(
        QStandardPaths::DataLocation);
  // check if VLF directory exists
  if (! nodeDir.exists()) {
    nodeDir.mkpath(nodeDir.absolutePath());
  }
  // Load or create identity
  QString idFile(nodeDir.canonicalPath()+"/identity.pem");
  if (!QFile::exists(idFile)) {
    logInfo() << "No identity found -> create new identity.";
    _identity = Identity::newIdentity();
    if (_identity) { _identity->save(idFile); }
  } else {
    logDebug() << "Load identity from" << idFile;
    _identity = Identity::load(idFile);
  }

  if (0 == _identity) {
    logError() << "Error while loading or creating identity.";
    return;
  }

  // Create log model
  //_logModel = new LogModel();
  Logger::addHandler(_logmodel);

  // Create DHT instance
  _node = new Node(*_identity, QHostAddress::Any, 7742);

  _stations = new StationList(*this);
}

Node &
Station::node() {
  return *_node;
}

LogModel &
Station::log() {
  return *_logmodel;
}

StationList &
Station::stations() {
  return *_stations;
}

