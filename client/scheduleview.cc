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


ScheduleView::ScheduleView(Application &app, QWidget *parent)
  : QWidget(parent), _application(app)
{
  _events = new QListView();
  _events->setModel(&app.station().schedule());
  _events->setSelectionMode(QAbstractItemView::SingleSelection);
  _events->setStyleSheet("QListView { "
                         " font-family: serif;"
                         " font-size:  18pt;"
                         "};");

  QPushButton *add = new QPushButton(tr("+"));
  add->setToolTip(tr("Add an event to the schedule."));
  QPushButton *rem = new QPushButton(tr("-"));
  rem->setToolTip(tr("Removes the selected event from the schedule."));

  QHBoxLayout *bbox = new QHBoxLayout();
  bbox->addWidget(add);
  bbox->addWidget(rem);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addLayout(bbox, 0);
  layout->addWidget(_events, 1);

  connect(add, SIGNAL(clicked(bool)), this, SLOT(_onAdd()));
  connect(rem, SIGNAL(clicked(bool)), this, SLOT(_onRem()));

  setLayout(layout);
}

void
ScheduleView::_onAdd() {

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
ScheduleView::_onRem() {
  // Get selected event
  QModelIndex idx = _events->selectionModel()->currentIndex();
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
