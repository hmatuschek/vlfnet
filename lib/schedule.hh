#ifndef SCHEDULE_HH
#define SCHEDULE_HH

#include <QAbstractListModel>
#include <QDateTime>
#include <QVector>


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

protected:
  /** The type of the event. */
  Type _type;
  /** Date and time of the first event. */
  QDateTime _first;
};


class Schedule : public QAbstractListModel
{
  Q_OBJECT

public:
  explicit Schedule(QObject *parent = 0);

  size_t numEvents() const;

  size_t addSingle(const QDateTime &when);
  size_t addDaily(const QDateTime &first);
  size_t addWeekly(const QDateTime &first);

  const ScheduledEvent &scheduledEvent(size_t idx) const;
  void removeScheduledEvent(size_t idx);

  QDateTime next(const QDateTime &now) const;

  /* *** Implementation of QAbstractListModel interface. */
  int rowCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

  virtual bool save();

signals:
  void startRecording();

protected slots:
  void _updateSchedule();
  void _checkSchedule();

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

#endif // SCHEDULE_HH
