#include "stationlist.hh"
#include "station.hh"
#include "query.hh"
#include <ovlnet/utils.hh>
#include <QJsonObject>


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

StationItem::StationItem(const NodeItem &node, const QJsonObject &obj)
  : _lastSeen(QDateTime::currentDateTime()), _node(node), _location(), _description()
{
  if (! obj.contains("id")) {
    logDebug() << "Cannot construct StationItem from JSON document: Does not specify a station ID.";
    _node = NodeItem(); return;
  }
  // Verify node info with ID
  Identifier id = Identifier::fromBase32(obj.value("id").toString());
  if (_node.id() != id) {
    logDebug() << "Cannot construct StationItem from JSON document: Node ID missmatch!";
    _node = NodeItem(); return;
  }

  if (! obj.contains("location")) {
    logDebug() << "Cannot construct StationItem from JSON document: No location specified.";
    _node = NodeItem(); return;
  }

  _location = Location(obj.value("location").toObject());
  _description = obj.value("description").toString();

  /*logDebug() << "Got info for station " << _node.id() << " (" << _node.addr() << ":" << _node.port()
             << ") @" << _location.longitude() << ", " << _location.latitude(); */
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
 * Implementation of StationList
 * ********************************************************************************************* */
StationList::StationList(Station &station)
  : QAbstractTableModel(&station), _station(station)
{
  // every 10min update the network of known stations
  _networkUpdateTimer.setInterval(1000*600);
  _networkUpdateTimer.setSingleShot(false);

  connect(&_networkUpdateTimer, SIGNAL(timeout()), this, SLOT(_onUpdateNetwork()));

  _networkUpdateTimer.start();

  // done...
}

size_t
StationList::numStations() const {
  return _stations.size();
}

bool
StationList::hasStation(const Identifier &id) const {
  for (int i=0; i<_stations.size(); i++) {
    if (_stations[i].id() == id) {
      return true;
    }
  }
  return false;
}

const StationItem &
StationList::station(size_t i) const {
  return _stations[i];
}

StationItem &
StationList::station(size_t i) {
  return _stations[i];
}

const StationItem &
StationList::station(const Identifier &id) const {
  return _stations[indexOf(id)];
}

StationItem &
StationList::station(const Identifier &id) {
  return _stations[indexOf(id)];
}

size_t
StationList::indexOf(const Identifier &id) const {
  for (int i=0; i<_stations.size(); i++) {
    if (_stations[i].id() == id) { return i; }
  }
  return _stations.size();

}

void
StationList::addCandidate(const Identifier &id) {
  if (! hasStation(id)) {
    _candidates.insert(id);
  }
}

void
StationList::contactStation(const NodeItem &node) {
  StationInfoQuery *query = new StationInfoQuery(_station, node);
  connect(query, SIGNAL(stationInfoReceived(StationItem)),
          this, SLOT(updateStation(StationItem)));
}

void
StationList::contactStation(const Identifier &node) {
  StationInfoQuery *query = new StationInfoQuery(_station, node);
  connect(query, SIGNAL(stationInfoReceived(StationItem)),
          this, SLOT(updateStation(StationItem)));
}

void
StationList::updateStation(const StationItem &station) {
  if (station.isNull()) { return; }
  if (hasStation(station.id())) {
    // If station extists: update station
    size_t idx = indexOf(station.id());
    _stations[idx] = station;
    emit dataChanged(index(idx, 0), index(idx, 4));
  } else {
    // Remove from candidates
    _candidates.remove(station.id());
    // & add to station list
    beginInsertRows(QModelIndex(), _stations.size(), _stations.size());
    _stations.append(station);
    endInsertRows();
  }
  // logDebug() << "Station " << station.id() << ": Status updated.";
  emit stationUpdated(station);
}

void
StationList::addToCandidates(const QList<Identifier> &nodes) {
  // logDebug() << "Received list of " << nodes.size() << " station identifiers.";
  QList<Identifier>::const_iterator node = nodes.begin();
  for (; node != nodes.end(); node++) {
    // only add new stations
    if ((! hasStation(*node)) && (_station.id() != *node)) {
      _candidates.insert(*node);
    }
  }
}

void
StationList::_onUpdateNetwork() {
  // As long as there are items in the candidate queue -> contact them
  if (_candidates.size()) {
    // Take some element
    Identifier id = *_candidates.begin(); _candidates.remove(id);
    contactStation(id);
    return;
  }

  // If all candidates has been contacted search for new candidates
  if (_stations.size()) {
    size_t idx = dht_rand32() % _stations.size();
    // Update station
    contactStation(_stations[idx].id());
    // and get station list
    // logDebug() << "Update network: Query start list from " << _stations[idx].id();
    StationListQuery *query = new StationListQuery(_station, _stations[idx].id());
    connect(query, SIGNAL(stationListReceived(QList<Identifier>)),
            this, SLOT(addToCandidates(QList<Identifier>)));
  }
}

/* ******************** Implementation of QAbstractTableModel interface ******************** */
int
StationList::rowCount(const QModelIndex &parent) const {
  return numStations();
}

int
StationList::columnCount(const QModelIndex &parent) const {
  return 7;
}

QVariant
StationList::data(const QModelIndex &index, int role) const {
  if (index.row() >= _stations.size()) { return QVariant(); }
  if (Qt::DisplayRole != role) { return QVariant(); }

  switch (index.column()) {
    case 0:
      return _stations[index.row()].id().toBase32();
    case 1:
      return _stations[index.row()].peer().addr().toString()
          + ":" + QString::number(_stations[index.row()].peer().port());
    case 2:
      return _stations[index.row()].location().longitude();
    case 3:
      return _stations[index.row()].location().latitude();
    case 4:
      return _stations[index.row()].location().height();
    case 5:
      return tr("%1 (%2) km")
          .arg(QString::number(_stations[index.row()].location().arcDist(_station.location()), 'f', 1))
          .arg(QString::number(_stations[index.row()].location().lineDist(_station.location()), 'f', 1));
    case 6:
      return _stations[index.row()].description();
  }

  return QVariant();
}

QVariant
StationList::headerData(int section, Qt::Orientation orientation, int role) const {
  if (Qt::Horizontal != orientation) { return QVariant(); }
  if (Qt::DisplayRole != role) { return QVariant(); }
  switch (section) {
    case 0: return tr("Identifier");
    case 1: return tr("Address/Port");
    case 2: return tr("Longitude");
    case 3: return tr("Latitude");
    case 4: return tr("Height");
    case 5: return tr("Distance");
    case 6: return tr("Destription");
  }
  return QVariant();
}
