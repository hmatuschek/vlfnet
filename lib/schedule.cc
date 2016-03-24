#include "schedule.hh"
#include "station.hh"
#include "stationlist.hh"
#include "query.hh"
#include <ovlnet/logger.hh>

#include <QJsonObject>
#include <QTimeZone>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFont>


inline QString
dayOfWeekName(int dow) {
  switch (dow) {
    case Qt::Monday: return QObject::tr("Monday");
    case Qt::Tuesday: return QObject::tr("Tuesday");
    case Qt::Wednesday: return QObject::tr("Wednesday");
    case Qt::Thursday: return QObject::tr("Thursday");
    case Qt::Friday: return QObject::tr("Friday");
    case Qt::Saturday: return QObject::tr("Saturday");
    case Qt::Sunday: return QObject::tr("Sunday");
    default: break;
  }
  return "";
}


/* ********************************************************************************************* *
 * Implementation of ScheduledEvent interface
 * ********************************************************************************************* */
ScheduledEvent::ScheduledEvent()
  : _type(SINGLE), _first()
{
  // pass...
}

ScheduledEvent::ScheduledEvent(const QDateTime &first, Type type)
  : _type(type), _first(first)
{
  // pass...
}

ScheduledEvent::ScheduledEvent(const QJsonObject &obj)
  : _type(SINGLE), _first()
{
  // Check if first event date-time is present
  if (! obj.contains("first")) { return; }
  // Read date-time as UTC
  QDateTime first = QDateTime::fromString(obj.value("first").toString(), "yyyy-MM-dd hh:mm:ss");
  if (! first.isValid()) { return; }
  // Specify that the given date time was in UTC
  first.setTimeSpec(Qt::UTC);
  // get local date time
  first = first.toLocalTime();

  // Check if type is present
  if (! obj.contains("repeat")) { return; }
  if ("never" == obj.value("repeat").toString()) {
    _type = SINGLE;
    _first = first;
  } else if ("daily" == obj.value("repeat").toString()) {
    _type = DAILY;
    _first = first;
  } else if ("weekly" == obj.value("repeat").toString()) {
    _type = WEEKLY;
    _first = first;
  }
}

ScheduledEvent::ScheduledEvent(const ScheduledEvent &other)
  : _type(other._type), _first(other._first)
{
  // pass...
}

ScheduledEvent &
ScheduledEvent::operator =(const ScheduledEvent &other) {
  _type = other._type;
  _first = other._first;
  return *this;
}

QDateTime
ScheduledEvent::nextEvent(const QDateTime &now) const {
  if (now < _first) {
    return _first;
  }

  if (DAILY == _type) {
    if (now.time() > _first.time()) {
      return QDateTime(now.date().addDays(1), _first.time());
    }
    return QDateTime(now.date(), _first.time());
  } else if (WEEKLY == _type) {
    if ((now.date().dayOfWeek()>_first.date().dayOfWeek()) ||
        ((_first.date().dayOfWeek()==now.date().dayOfWeek()) && (now.time() > _first.time()))) {
      // past...
      int ddays = now.date().dayOfWeek()-_first.date().dayOfWeek();
      return QDateTime(now.date().addDays(7-ddays), _first.time());
    }
    int ddays = _first.date().dayOfWeek()-now.date().dayOfWeek();
    return QDateTime(now.date().addDays(ddays), _first.time());
  }

  return QDateTime();
}

bool
ScheduledEvent::isValid() const {
  return _first.isValid();
}

ScheduledEvent::Type
ScheduledEvent::type() const {
  return _type;
}

const QDateTime &
ScheduledEvent::first() const {
  return _first;
}

double
ScheduledEvent::cost() const {
  switch (_type) {
    case DAILY: return 28.;
    case WEEKLY: return 4;
    case SINGLE: return 1;
  }
  return 0.;
}

QJsonObject
ScheduledEvent::toJson() const {
  QJsonObject obj;
  obj.insert("first", _first.toUTC().toString("yyyy-MM-dd hh:mm:ss"));
  switch (_type) {
    case SINGLE: obj.insert("repeat", QString("never")); break;
    case DAILY: obj.insert("repeat", QString("daily")); break;
    case WEEKLY: obj.insert("repeat", QString("weekly")); break;
  }
  return obj;
}

bool
ScheduledEvent::passed(const QDateTime &timestamp) const {
  if (_type != SINGLE)
    return false;
  return _first < timestamp;
}

bool
ScheduledEvent::operator ==(const ScheduledEvent &other) const {
  if (_type != other._type) { return false; }
  return _first == other._first;
}

bool
ScheduledEvent::operator !=(const ScheduledEvent &other) const {
  return !(*this == other);
}


/* ********************************************************************************************* *
 * Implementation of Schedule interface
 * ********************************************************************************************* */
Schedule::Schedule(QObject *parent)
  : QAbstractListModel(parent)
{
  // pass...
}

QDateTime
Schedule::next(const QDateTime &now) const {
  QDateTime next;
  for (size_t i=0; i<this->numEvents(); i++) {
    QDateTime evt = this->scheduledEvent(i).nextEvent(now);
    if (evt.isValid() && (!next.isValid() || (evt<next))) {
      next = evt;
    }
  }
  return next;
}

bool
Schedule::contains(const ScheduledEvent &event) const {
  for (size_t i=0; i<this->numEvents(); i++) {
    if (this->scheduledEvent(i) == event)
      return true;
  }
  return false;
}

QJsonArray
Schedule::toJson() const {
  QJsonArray res;
  for (size_t i=0; i<this->numEvents(); i++) {
    if (this->scheduledEvent(i).isValid())
      res.append(this->scheduledEvent(i).toJson());
  }
  return res;
}

int
Schedule::rowCount(const QModelIndex &parent) const {
  return this->numEvents();
}

QVariant
Schedule::data(const QModelIndex &index, int role) const {
  if (size_t(index.row()) >= this->numEvents()) { return QVariant(); }
  const ScheduledEvent &evt = this->scheduledEvent(index.row());

  if (Qt::DisplayRole == role) {
    if (ScheduledEvent::SINGLE == evt.type()) {
      return tr("Once on %1 at %2")
          .arg(evt.first().date().toString())
          .arg(evt.first().time().toString());
    } else if (ScheduledEvent::DAILY == evt.type()) {
      return tr("Daily at %1 starting on %2")
          .arg(evt.first().time().toString())
          .arg(evt.first().date().toString());
    } else if (ScheduledEvent::WEEKLY == evt.type()) {
      return tr("Every %1 at %2 starting on %3")
          .arg(dayOfWeekName(evt.first().date().dayOfWeek()))
          .arg(evt.first().time().toString())
          .arg(evt.first().date().toString());
    }

    return QVariant("Invalid.");
  }

  return QVariant();
}

QVariant
Schedule::headerData(int section, Qt::Orientation orientation, int role) const {
  if (Qt::DisplayRole != role) { return QVariant(); }
  if (Qt::Horizontal != orientation) { return QVariant(); }
  if (0 != section) { return QVariant(); }
  return tr("Scheduled events");
}


/* ********************************************************************************************* *
 * Implementation of LocalSchedule
 * ********************************************************************************************* */
LocalSchedule::LocalSchedule(const QString &path, QObject *parent)
  : Schedule(parent), _filename(path), _nextEvent(), _timer()
{
  logDebug() << "Load local schedule from file " << _filename << ".";
  QFile file(_filename);
  if (! file.open(QIODevice::ReadOnly)) {
    logError() << "Cannot read local schedule from file " << _filename << ".";
    return;
  }

  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
  if (QJsonParseError::NoError != err.error) {
    logError() << "Cannot read local schedule from file " << _filename
               << ": " << err.errorString() << ".";
    return;
  }

  if (! doc.isArray()) {
    logError() << "Cannot read local schedule from file " << _filename
               << ": Schedule is not an array of scheduled events.";
    return;
  }

  QJsonArray array = doc.array();
  for (int i=0; i < array.size(); i++) {
    if (! array.at(i).isObject()) {
      logWarning() << "Cannot read event from schedule " << _filename
                   << ": Item is not an object.";
      continue;
    }
    ScheduledEvent evt(array.at(i).toObject());
    if (! evt.isValid()) {
      logWarning() << "Cannot read event from schedule " << _filename
                   << ": Item is malformed.";
      continue;
    }

    size_t idx = _events.size();
    beginInsertRows(QModelIndex(), idx, idx);
    _events.push_back(evt);
    _updateSchedule();
    endInsertRows();

    _timer.setInterval(750);
    _timer.setSingleShot(false);

    connect(&_timer, SIGNAL(timeout()), this, SLOT(checkSchedule()));
    _timer.start();
  }
}

bool
LocalSchedule::save() {
  logDebug() << "Save local schedule to " << _filename << ".";
  QJsonArray events;
  for (int i=0; i<_events.size(); i++) {
    if (_events[i].isValid()) {
      events.append(_events[i].toJson());
    }
  }

  QJsonDocument doc(events);

  QFile file(_filename);
  if (! file.open(QIODevice::WriteOnly)) {
    logError() << "Cannot save schedule: Cannot open file " << _filename << ".";
    return false;
  }

  file.write(doc.toJson());
  file.flush();
  file.close();

  return true;
}

size_t
LocalSchedule::numEvents() const {
  return _events.size();
}

size_t
LocalSchedule::add(const ScheduledEvent &event) {
  for (int i=0; i<_events.size(); i++) {
    if (_events[i] == event) { return i; }
  }
  size_t idx = _events.size();
  beginInsertRows(QModelIndex(), idx, idx);
  _events.push_back(event);
  _updateSchedule();
  endInsertRows();
  emit updated();
  return idx;
}

size_t
LocalSchedule::addSingle(const QDateTime &when) {
  return add(ScheduledEvent(when, ScheduledEvent::SINGLE));
}

size_t
LocalSchedule::addDaily(const QDateTime &first) {
  return add(ScheduledEvent(first, ScheduledEvent::DAILY));
}

size_t
LocalSchedule::addWeekly(const QDateTime &first) {
  return add(ScheduledEvent(first, ScheduledEvent::WEEKLY));
}

const ScheduledEvent &
LocalSchedule::scheduledEvent(size_t idx) const {
  return _events[idx];
}

void
LocalSchedule::removeScheduledEvent(size_t idx) {
  if (int(idx) < _events.size()) {
    beginRemoveRows(QModelIndex(), idx, idx);
    _events.remove(idx, 1);
    _updateSchedule();
    endRemoveRows();
    emit updated();
  }
}

void
LocalSchedule::_updateSchedule() {
  _nextEvent = this->next(QDateTime::currentDateTime());
}

void
LocalSchedule::checkSchedule() {
  if (_nextEvent.isValid() && (_nextEvent == QDateTime::currentDateTime())) {
    _updateSchedule();
    // Start a recording of 10 min
    emit startRecording(1000*60*10);
  }
}


/* ********************************************************************************************* *
 * Implementation of RemoteScheduledEvent
 * ********************************************************************************************* */
RemoteScheduledEvent::RemoteScheduledEvent()
  : ScheduledEvent(), _nodes()
{
  // pass...
}

RemoteScheduledEvent::RemoteScheduledEvent(const Identifier &node, const QDateTime &first, ScheduledEvent::Type type)
  : ScheduledEvent(first, type), _nodes()
{
  _nodes.insert(node);
}

RemoteScheduledEvent::RemoteScheduledEvent(const RemoteScheduledEvent &other)
  : ScheduledEvent(other), _nodes(other._nodes)
{
  // pass...
}

RemoteScheduledEvent::RemoteScheduledEvent(const Identifier &node, const ScheduledEvent &obj)
  : ScheduledEvent(obj), _nodes()
{
  _nodes.insert(node);
}

RemoteScheduledEvent &
RemoteScheduledEvent::operator =(const RemoteScheduledEvent &other) {
  ScheduledEvent::operator =(other);
  _nodes = other._nodes;
  return *this;
}

size_t
RemoteScheduledEvent::numNodes() const {
  return _nodes.size();
}

const QSet<Identifier> &
RemoteScheduledEvent::nodes() const {
  return _nodes;
}

void
RemoteScheduledEvent::addNode(const Identifier &id) {
  _nodes.insert(id);
}


/* ********************************************************************************************* *
 * Implementation of RemoteSchedule
 * ********************************************************************************************* */
RemoteSchedule::RemoteSchedule(Station &station, QObject *parent)
  : Schedule(parent), _station(station)
{
  connect(&_station.stations(), SIGNAL(stationUpdated(StationItem)),
          this, SLOT(_onUpdateStationSchedule(StationItem)));
}

size_t
RemoteSchedule::numEvents() const {
  return _events.size();
}

const ScheduledEvent &
RemoteSchedule::scheduledEvent(size_t i) const {
  return _events[i];
}

void
RemoteSchedule::add(const Identifier &node, const ScheduledEvent &obj) {
  RemoteScheduledEvent evt(node, obj);
  if (! evt.isValid()) { return; }

  for (int i=0; i<_events.size(); i++) {
    if (_events[i] == evt) {
      emit dataChanged(index(i,0), index(i,0));
      _events[i].addNode(node);
      emit updated();
      return;
    }
  }

  beginInsertRows(QModelIndex(), _events.size(), _events.size());
  _events.append(evt);
  endInsertRows();
  emit updated();
}

size_t
RemoteSchedule::numNodes(size_t i) const {
  return _events[i].numNodes();
}

void
RemoteSchedule::_onUpdateStationSchedule(const StationItem &station) {
  // logDebug() << "Station " << station.id() << " updated -> Update schedule.";
  StationScheduleQuery *query = new StationScheduleQuery(_station, station.node());
  connect(query, SIGNAL(stationScheduleReceived(Identifier,QList<ScheduledEvent>)),
          this, SLOT(_onStationScheduleReceived(Identifier,QList<ScheduledEvent>)));
}

void
RemoteSchedule::_onStationScheduleReceived(const Identifier &remote, const QList<ScheduledEvent> &events) {
  // logDebug() << "Station schedule received...";
  QList<ScheduledEvent>::const_iterator evt = events.begin();
  for (; evt != events.end(); evt++) {
    this->add(remote, *evt);
  }
}


/* ********************************************************************************************* *
 * Implementation of MergedSchedule
 * ********************************************************************************************* */
bool eventWeightLessThan(const QPair<ScheduledEvent, size_t> &a, const QPair<ScheduledEvent, size_t> &b) {
  // Returns the weight for the events that is (cost / # nodes )
  return (a.first.cost()/a.second) < (b.first.cost()/b.second);
}

MergedSchedule::MergedSchedule(const QString &path, Station &station, double maxCosts, QObject *parent)
  : Schedule(parent), _maxCosts(maxCosts), _local(path), _remote(station)
{
  // Connect to changes in the local and remote schedules
  connect(&_local, SIGNAL(updated()), this, SLOT(_onScheduleUpdate()));
  connect(&_remote, SIGNAL(updated()), this, SLOT(_onScheduleUpdate()));

  // Configure check timer
  _timer.setInterval(750);
  _timer.setSingleShot(false);
  // check for event regularily
  connect(&_timer, SIGNAL(timeout()), this, SLOT(checkSchedule()));

  _onScheduleUpdate();
  _timer.start();
}

size_t
MergedSchedule::numEvents() const {
  return _local.numEvents()+_mergedRemoteEvents.size();
}

const ScheduledEvent &
MergedSchedule::scheduledEvent(size_t idx) const {
  if (idx < _local.numEvents()) {
    return _local.scheduledEvent(idx);
  }
  return _mergedRemoteEvents[idx-_local.numEvents()];
}

LocalSchedule &
MergedSchedule::local() {
  return _local;
}

RemoteSchedule &
MergedSchedule::remote() {
  return _remote;
}

void
MergedSchedule::checkSchedule() {
  if (_nextEvent.isValid() && (_nextEvent == QDateTime::currentDateTime())) {
    _updateSchedule();
    // Start a recording of 10 min
    emit startRecording(1000*60*10);
  }
}

void
MergedSchedule::_updateSchedule() {
  _nextEvent = this->next(QDateTime::currentDateTime());
}

void
MergedSchedule::_onScheduleUpdate() {
  beginResetModel();
  double cost = _maxCosts;
  // Add consts from local events
  for (size_t i=0; i<_local.numEvents(); i++) {
    cost -= _local.scheduledEvent(i).cost();
  }
  _mergedRemoteEvents.clear();
  // If some cost margin left for remote events add some cheap enough but worth it
  if (cost > 0) {
    // Filter remote events by cost
    QVector< QPair<ScheduledEvent, size_t> > remEvt; remEvt.reserve(_remote.numEvents());
    for (size_t i=0; i<_remote.numEvents(); i++) {
      // Ignore all events to expesive or already on local schedule or passed
      if ( (_remote.scheduledEvent(i).cost() < cost)
           && !_local.contains(_remote.scheduledEvent(i))
           && !_remote.scheduledEvent(i).passed(QDateTime::currentDateTime()))
        remEvt.append(QPair<ScheduledEvent, size_t>(_remote.scheduledEvent(i), _remote.numNodes(i)));
    }
    // sort remote events by weight
    qStableSort(remEvt.begin(), remEvt.end(), &eventWeightLessThan);
    // Add as many events as allowed by the cost margin left
    for (int i=0; i<remEvt.size(); i++) {
      if (cost > remEvt[i].first.cost()) {
        _mergedRemoteEvents.append(remEvt[i].first);
        cost -= remEvt[i].first.cost();
      }
    }
  }
  endResetModel();
  // Update next event
  _updateSchedule();
}
