#ifndef SCHEDULEVIEW_HH
#define SCHEDULEVIEW_HH

#include <QWidget>

class Application;
class QListView;

class ScheduleView : public QWidget
{
  Q_OBJECT

public:
  explicit ScheduleView(Application &app, QWidget *parent = 0);

protected slots:
  void _onAdd();
  void _onRem();

protected:
  Application &_application;
  QListView *_events;
};

#endif // SCHEDULEVIEW_HH
