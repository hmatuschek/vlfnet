#ifndef SCHEDULEVIEW_HH
#define SCHEDULEVIEW_HH

#include <QWidget>
#include <QTabWidget>


class Application;
class QListView;
class RemoteSchedule;


class LocalScheduleView : public QWidget
{
  Q_OBJECT

public:
  explicit LocalScheduleView(Application &app, QWidget *parent = 0);

protected slots:
  void _onAdd();
  void _onRem();

protected:
  Application &_application;
  QListView *_localEvents;
};


class RemoteScheduleView : public QWidget
{
  Q_OBJECT

public:
  explicit RemoteScheduleView(Application &app, QWidget *parent = 0);

protected slots:
  void _onAdd();

protected:
  Application &_application;
  RemoteSchedule *_remoteSchedule;
  QListView *_remoteEvents;
};


class ScheduleView: public QTabWidget
{
  Q_OBJECT;

public:
  explicit ScheduleView(Application &app, QWidget *parent=0);
};

#endif // SCHEDULEVIEW_HH
