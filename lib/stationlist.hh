#ifndef STATIONLIST_HH
#define STATIONLIST_HH

#include <ovlnet/buckets.hh>
#include <QJsonDocument>
#include <QAbstractTableModel>
#include <QTimer>
#include "location.hh"


class Node;
class Station;
class HttpClientConnection;
class HttpClientResponse;


class StationItem
{
public:
  StationItem();
  StationItem(const Identifier &id, const Location &location, const QString &descr="");
  StationItem(const NodeItem &node, const Location &location, const QString &descr="");
  StationItem(const NodeItem &node, const QJsonObject &obj);
  StationItem(const StationItem &other);

  StationItem &operator=(const StationItem &other);

  bool isNull() const;

  const QDateTime &lastSeen() const;
  const Identifier &id() const;
  const NodeItem &node() const;
  const PeerItem &peer() const;
  const Location &location() const;
  const QString &description() const;

  void update(const PeerItem &peer);

protected:
  QDateTime _lastSeen;
  NodeItem _node;
  Location _location;
  QString _description;
};


class StationList : public QAbstractTableModel
{
  Q_OBJECT

public:
  StationList(Station &station);

  size_t numStations() const;
  bool hasStation(const Identifier &id) const;
  const StationItem &station(size_t i) const;
  StationItem &station(size_t i);
  const StationItem &station(const Identifier &id) const;
  StationItem &station(const Identifier &id);

  /* *** Implementation of QAbstractTableModel interface. *** */
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

signals:
  void stationUpdated(const StationItem &item);

protected slots:
  void updateStation(const StationItem &station);
  void addCandidate(const Identifier &id);
  void addToCandidates(const QList<Identifier> &nodes);
  void contactStation(const NodeItem &node);
  void contactStation(const Identifier &node);

protected:
  size_t indexOf(const Identifier &id) const;

private slots:
  void _onUpdateNetwork();

protected:
  Station &_station;
  QVector<StationItem> _stations;
  QSet<Identifier> _candidates;
  QTimer _networkUpdateTimer;
};


#endif // STATIONLIST_HH
