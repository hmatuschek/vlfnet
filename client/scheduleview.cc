#include "scheduleview.hh"

#include "application.hh"
#include "lib/station.hh"
#include "lib/schedule.hh"

#include <QListView>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QMessageBox>
#include <QPushButton>

#include "newscheduledeventdialog.hh"


/* ********************************************************************************************* *
 * Implementation of LocalScheduleView
 * ********************************************************************************************* */
LocalScheduleView::LocalScheduleView(Application &app, QWidget *parent)
  : QWidget(parent), _application(app)
{
  _localEvents = new QListView();
  _localEvents->setModel(&app.station().schedule());
  _localEvents->setSelectionMode(QAbstractItemView::SingleSelection);

  QPushButton *add = new QPushButton(tr("+"));
  add->setToolTip(tr("Add an event to the schedule."));
  QPushButton *rem = new QPushButton(tr("-"));
  rem->setToolTip(tr("Removes the selected event from the schedule."));

  QHBoxLayout *bbox = new QHBoxLayout();
  bbox->addWidget(add);
  bbox->addWidget(rem);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addLayout(bbox, 0);
  layout->addWidget(_localEvents, 1);

  connect(add, SIGNAL(clicked(bool)), this, SLOT(_onAdd()));
  connect(rem, SIGNAL(clicked(bool)), this, SLOT(_onRem()));

  setLayout(layout);
}

void
LocalScheduleView::_onAdd() {

  NewScheduledEventDialog dialog;
  if (QDialog::Accepted != dialog.exec()) {
    return;
  }

  switch (dialog.type()) {
    case ScheduledEvent::SINGLE:
      _application.station().schedule().addSingle(dialog.firstEvent());
      break;
    case ScheduledEvent::DAILY:
      _application.station().schedule().addDaily(dialog.firstEvent());
      break;
    case ScheduledEvent::WEEKLY:
      _application.station().schedule().addWeekly(dialog.firstEvent());
      break;
  }

  _application.station().schedule().save();
}

void
LocalScheduleView::_onRem() {
  // Get selected event
  QModelIndex idx = _localEvents->selectionModel()->currentIndex();
  if (! idx.isValid()) {
    return;
  }

  if (QMessageBox::Yes != QMessageBox::question(0, tr("Remove scheduled event"), tr("Remove selected event?"),
                                                QMessageBox::No, QMessageBox::Yes)) {
    return;
  }

  _application.station().schedule().removeScheduledEvent(idx.row());
  _application.station().schedule().save();
}


/* ********************************************************************************************* *
 * Implementation of RemoteScheduleView
 * ********************************************************************************************* */
RemoteScheduleView::RemoteScheduleView(Application &app, QWidget *parent)
  : QWidget(parent), _application(app), _remoteSchedule(0)
{
  _remoteSchedule = new RemoteSchedule(app.station(), this);

  _remoteEvents = new QListView();
  _remoteEvents->setModel(_remoteSchedule);
  _remoteEvents->setSelectionMode(QAbstractItemView::SingleSelection);

  QPushButton *add = new QPushButton(tr("+"));
  add->setToolTip(tr("Add the selected remote event to the local schedule."));

  QHBoxLayout *bbox = new QHBoxLayout();
  bbox->addWidget(add);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addLayout(bbox, 0);
  layout->addWidget(_remoteEvents, 1);

  connect(add, SIGNAL(clicked(bool)), this, SLOT(_onAdd()));

  setLayout(layout);
}

void
RemoteScheduleView::_onAdd() {
  // Get selected event
  QModelIndex idx = _remoteEvents->selectionModel()->currentIndex();
  if (! idx.isValid()) {
    return;
  }

  if (QMessageBox::Yes != QMessageBox::question(0, tr("Add event"), tr("Add selected event to local schedule?"),
                                                QMessageBox::No, QMessageBox::Yes)) {
    return;
  }

  _application.station().schedule().add(_remoteSchedule->scheduledEvent(idx.row()));
  _application.station().schedule().save();
}


/* ********************************************************************************************* *
 * Implementation of ScheduleView
 * ********************************************************************************************* */
ScheduleView::ScheduleView(Application &app, QWidget *parent)
  : QTabWidget(parent)
{
  addTab(new LocalScheduleView(app), tr("Local Schedule"));
  addTab(new RemoteScheduleView(app), tr("Remote Schedules"));
}
