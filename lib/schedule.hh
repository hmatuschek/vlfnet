#ifndef SCHEDULE_HH
#define SCHEDULE_HH

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>

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

  /** Serializes the event into a JSON object. */
  QJsonObject toJson() const;

  bool operator==(const ScheduledEvent &other) const;
  bool operator!=(const ScheduledEvent &other) const;

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

  size_t numEvents() const;

  size_t add(const ScheduledEvent &event);
  size_t addSingle(const QDateTime &when);
  size_t addDaily(const QDateTime &first);
  size_t addWeekly(const QDateTime &first);

  const ScheduledEvent &scheduledEvent(size_t idx) const;
  void removeScheduledEvent(size_t idx);

  QDateTime next(const QDateTime &now) const;

  virtual bool save();

  /* *** Implementation of QAbstractListModel interface. */
  int rowCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

public slots:
  void checkSchedule();

signals:
  void startRecording();

protected slots:
  void _updateSchedule();

protected:
  QDateTime _nextEvent;
  QVector<ScheduledEvent> _events;
};


class LocalSchedule: public Schedule
{
  Q_OBJECT

public:
  explicit LocalSchedule(const QString &path, QObject *parent=0);

  virtual bool save();

protected:
  QString _filename;
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


class RemoteSchedule: public QAbstractListModel
{
  Q_OBJECT

public:
  explicit RemoteSchedule(Station &station, QObject *parent=0);

  size_t numEvents() const;
  const RemoteScheduledEvent &scheduledEvent(size_t i) const;
  void add(const Identifier &node, const ScheduledEvent &obj);

  /* *** Implementation of QAbstractListModel interface. */
  int rowCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;

protected slots:
  void _onUpdateStationSchedule(const StationItem &station);
  void _onStationScheduleReceived(const Identifier &remote, const QList<ScheduledEvent> &events);

protected:
  Station &_station;
  QVector<RemoteScheduledEvent> _events;
};

#endif // SCHEDULE_HH
