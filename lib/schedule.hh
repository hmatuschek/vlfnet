#ifndef SCHEDULE_HH
#define SCHEDULE_HH

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>
#include <QTimer>

#include <ovlnet/buckets.hh>


class Station;
class StationItem;


/** Represents a single or repeating event. */
class ScheduledEvent
{
public:
  /** Type of an event. */
  typedef enum {
    SINGLE, ///< This event ocurres only once.
    DAILY,  ///< This event is repeated daily.
    WEEKLY  ///< This event is repeated weekly.
  } Type;

public:
  /** Empty constructor. */
  ScheduledEvent();
  /** Constructor for an event at the specified date and time. */
  ScheduledEvent(const QDateTime &first, Type=SINGLE);
  /** Copy constructor. */
  ScheduledEvent(const ScheduledEvent &other);
  /** Constructs an event from JSON. */
  ScheduledEvent(const QJsonObject &obj);
  /** Assignment operator. */
  ScheduledEvent &operator=(const ScheduledEvent &other);

  /** Returns the next date and time relative to the given date and time, at which the event
   * (re) ocurres. Returns an invalid date and time if a single event passed. */
  QDateTime nextEvent(const QDateTime &now) const;

  /** Returns @c true if the event is valid. */
  bool isValid() const;
  /** Returns the type of the event. */
  Type type() const;
  /** Returns the local date and time of the first occurence of the event. */
  const QDateTime &first() const;
  /** Returns the cost associated with the event.
   * Costs are used for the heuristic to determine which remote events are selected to be included
   * in the schedule. The costs of an event is determined by its type.
   * That is, a single event has a cost of 1, weekly events have a cost of 4 and daily event costs
   * 28. */
  double cost() const;

  /** Serializes the event into a JSON object. */
  QJsonObject toJson() const;

  bool operator==(const ScheduledEvent &other) const;
  bool operator!=(const ScheduledEvent &other) const;

  /** Returns true if the event passed (only single events can pass). */
  bool passed(const QDateTime &timestamp) const;

protected:
  /** The type of the event. */
  Type _type;
  /** Date and time of the first event. */
  QDateTime _first;
};


class Schedule: public QAbstractListModel
{
  Q_OBJECT

public:
  explicit Schedule(QObject *parent = 0);

  virtual size_t numEvents() const = 0;
  virtual const ScheduledEvent &scheduledEvent(size_t idx) const = 0;

  virtual bool contains(const ScheduledEvent &event) const;
  virtual QDateTime next(const QDateTime &now) const;

  virtual QJsonArray toJson() const;

  /* *** Implementation of QAbstractListModel interface. */
  int rowCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

signals:
  /** Gets emitted on schedule update (e.g. addition, removal or changing an event). */
  void updated();
};


class LocalSchedule: public Schedule
{
  Q_OBJECT

public:
  explicit LocalSchedule(const QString &path, QObject *parent=0);

  virtual size_t numEvents() const;
  virtual const ScheduledEvent &scheduledEvent(size_t idx) const;

  size_t add(const ScheduledEvent &event);
  size_t addSingle(const QDateTime &when);
  size_t addDaily(const QDateTime &first);
  size_t addWeekly(const QDateTime &first);

  void removeScheduledEvent(size_t idx);

  bool save();

public slots:
  void checkSchedule();

signals:
  void startRecording(double mSec);

protected slots:
  void _updateSchedule();

protected:
  QString _filename;
  QDateTime _nextEvent;
  QVector<ScheduledEvent> _events;
  QTimer _timer;
};


class RemoteScheduledEvent: public ScheduledEvent
{
public:
  /** Empty constructor. */
  RemoteScheduledEvent();
  /** Constructor for an event at the specified date and time. */
  RemoteScheduledEvent(const Identifier &node, const QDateTime &first, ScheduledEvent::Type=SINGLE);
  /** Copy constructor. */
  RemoteScheduledEvent(const RemoteScheduledEvent &other);
  RemoteScheduledEvent(const Identifier &node, const ScheduledEvent &obj);
  /** Assignment operator. */
  RemoteScheduledEvent &operator=(const RemoteScheduledEvent &other);

  size_t numNodes() const;
  const QSet<Identifier> &nodes() const;
  void addNode(const Identifier &id);

protected:
  QSet<Identifier> _nodes;
};


class RemoteSchedule: public Schedule
{
  Q_OBJECT

public:
  explicit RemoteSchedule(Station &station, QObject *parent=0);

  size_t numEvents() const;
  const ScheduledEvent &scheduledEvent(size_t i) const;

  void add(const Identifier &node, const ScheduledEvent &obj);
  size_t numNodes(size_t i) const;

protected slots:
  void _onUpdateStationSchedule(const StationItem &station);
  void _onStationScheduleReceived(const Identifier &remote, const QList<ScheduledEvent> &events);

protected:
  Station &_station;
  QVector<RemoteScheduledEvent> _events;
};


class MergedSchedule: public Schedule
{
  Q_OBJECT

public:
  explicit MergedSchedule(const QString &path, Station &station, double maxCosts=28, QObject *parent=0);

  size_t numEvents() const;
  const ScheduledEvent &scheduledEvent(size_t idx) const;

  LocalSchedule &local();
  RemoteSchedule &remote();

public slots:
  void checkSchedule();

signals:
  void startRecording(double mSec);

protected slots:
  void _updateSchedule();
  void _onScheduleUpdate();

protected:
  double _maxCosts;
  LocalSchedule _local;
  RemoteSchedule _remote;
  QVector<ScheduledEvent> _mergedRemoteEvents;
  QDateTime _nextEvent;
  QTimer _timer;
};

#endif // SCHEDULE_HH
