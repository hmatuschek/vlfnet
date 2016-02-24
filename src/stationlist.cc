#include "stationlist.hh"
#include "station.hh"


StationList::StationList(Station &station)
  : QAbstractTableModel(&station), _station(station)
{
  // Connect to signals of station instance about appearance of other stations
  connect(&_station, SIGNAL(stationAppeared(Identifier)),
          this, SLOT(_onStationAppeared(Identifier)));
  connect(&_station, SIGNAL(stationDisappeared(Identifier)),
          this, SLOT(_onStationDisappeared(Identifier)));

  // done...
}

size_t
StationList::numStations() const {
  return _stations.size();
}

bool
StationList::hasStation(const Identifier &id) const {
  for (size_t i=0; i<_stations.size(); i++) {
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
  for (size_t i=0; i<_stations.size(); i++) {
    if (_stations[i].id() == id) { return i; }
  }
  return _stations.size();

}

void
StationList::_onNodeAppeared(const NodeItem &node) {
  if (hasStation(node.id())) {
    station(node.id()).update(node);
  }
}

void
StationList::_onNodeDisappeared(const NodeItem &id) {
  // pass...
}

int
StationList::rowCount(const QModelIndex &parent) const {
  return numStations();
}

int
StationList::columnCount(const QModelIndex &parent) const {
  return 5;
}

QVariant
StationList::data(const QModelIndex &index, int role) const {
  if (index.row() >= _stations.size()) { return QVariant(); }
  if (Qt::DisplayRole != role) { return QVariant(); }

  switch (index.column()) {
    case 0:
      return _stations[index.row()].id().toBase32();
    case 1:
      return _stations[index.row()].location().longitude();
    case 2:
      return _stations[index.row()].location().latitude();
    case 3:
      return _stations[index.row()].location().height();
    case 4:
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
    case 1: return tr("Longitude");
    case 2: return tr("Latitude");
    case 3: return tr("Height");
    case 4: return tr("Destription");
  }
  return QVariant();
}
