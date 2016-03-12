#include "schedule.hh"
#include <ovlnet/logger.hh>

#include <QJsonObject>
#include <QTimeZone>
#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>
#include <QFont>


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
  QDateTime first = QDateTime::fromString(obj.value("first").toString(), "YYYY-MM-dd hh:mm:ss");
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

QJsonObject
ScheduledEvent::toJson() const {
  QJsonObject obj;
  obj.insert("first", _first.toUTC().toString("YYYY-MM-dd hh:mm:ss"));
  switch (_type) {
    case SINGLE: obj.insert("repeat", "never"); break;
    case DAILY: obj.insert("repeat", "daily"); break;
    case WEEKLY: obj.insert("repeat", "weekly"); break;
  }
  return obj;
}

/* ********************************************************************************************* *
 * Implementation of Schedule interface
 * ********************************************************************************************* */
Schedule::Schedule(QObject *parent)
  : QAbstractListModel(parent), _nextEvent()
{
  // pass...
}

QDateTime
Schedule::next(const QDateTime &now) const {
  QDateTime next;
  for (int i=0; i<_events.size(); i++) {
    QDateTime evt = _events[i].nextEvent(now);
    if (evt.isValid() && (!next.isValid() || (evt<next))) {
      next = evt;
    }
  }
  return next;
}

size_t
Schedule::numEvents() const {
  return _events.size();
}

size_t
Schedule::addSingle(const QDateTime &when) {
  size_t idx = _events.size();
  beginInsertRows(QModelIndex(), idx, idx);
  _events.push_back(ScheduledEvent(when, ScheduledEvent::SINGLE));
  _updateSchedule();
  endInsertRows();
  return idx;
}

size_t
Schedule::addDaily(const QDateTime &first) {
  size_t idx = _events.size();
  beginInsertRows(QModelIndex(), idx, idx);
  _events.push_back(ScheduledEvent(first, ScheduledEvent::DAILY));
  _updateSchedule();
  endInsertRows();
  return idx;
}

size_t
Schedule::addWeekly(const QDateTime &first) {
  size_t idx = _events.size();
  beginInsertRows(QModelIndex(), idx, idx);
  _events.push_back(ScheduledEvent(first, ScheduledEvent::WEEKLY));
  _updateSchedule();
  endInsertRows();
  return idx;
}

const ScheduledEvent &
Schedule::scheduledEvent(size_t idx) const {
  return _events[idx];
}

void
Schedule::removeScheduledEvent(size_t idx) {
  if (int(idx) < _events.size()) {
    beginRemoveRows(QModelIndex(), idx, idx);
    _events.remove(idx, 1);
    _updateSchedule();
    endRemoveRows();
  }
}

void
Schedule::_updateSchedule() {
  _nextEvent = this->next(QDateTime::currentDateTime());
}

void
Schedule::_checkSchedule() {
  if (_nextEvent.isValid() && (_nextEvent == QDateTime::currentDateTime())) {
    emit startRecording();
  }
}

bool
Schedule::save() {
  return true;
}

int
Schedule::rowCount(const QModelIndex &parent) const {
  return _events.size();
}

QVariant
Schedule::data(const QModelIndex &index, int role) const {
  if (index.row() >= _events.size()) { return QVariant(); }
  const ScheduledEvent &evt = _events[index.row()];

  if (Qt::DisplayRole == role) {
    if (ScheduledEvent::SINGLE == evt.type()) {
      return tr("Once on %1 at %2").arg(
            evt.first().date().toString()).arg(
            evt.first().time().toString());
    } else if (ScheduledEvent::DAILY == evt.type()) {
      return tr("Daily at %1 starting on %2").arg(
            evt.first().time().toString()).arg(
            evt.first().date().toString());
    } else if (ScheduledEvent::WEEKLY == evt.type()) {
      return tr("Every %1 at %2 starting on %3").arg(
            evt.first().date().dayOfWeek()).arg(
            evt.first().time().toString()).arg(
            evt.first().date().toString());
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
  : Schedule(parent), _filename(path)
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

