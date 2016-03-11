#include "newscheduledeventdialog.hh"

#include <QComboBox>
#include <QDateTimeEdit>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>


NewScheduledEventDialog::NewScheduledEventDialog(QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Add a new scheduled reception ..."));

  _type = new QComboBox();
  _type->addItem(tr("Once"));
  _type->addItem(tr("Daily"));
  _type->addItem(tr("Weekly"));
  _type->setCurrentIndex(0);

  _when = new QDateTimeEdit(QDateTime::currentDateTime());
  _when->setCalendarPopup(true);

  QDialogButtonBox *bb = new QDialogButtonBox(QDialogButtonBox::Cancel | QDialogButtonBox::Save);

  QFormLayout *form = new QFormLayout();
  form->addRow(tr("Type"), _type);
  form->addRow(tr("When/First"), _when);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addLayout(form);
  layout->addWidget(bb);

  setLayout(layout);

  connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
  connect(bb, SIGNAL(rejected()), this, SLOT(reject()));
}

ScheduledEvent::Type
NewScheduledEventDialog::type() const {
  switch (_type->currentIndex()) {
    case 0: return ScheduledEvent::SINGLE;
    case 1: return ScheduledEvent::DAILY;
    case 2: return ScheduledEvent::WEEKLY;
    default:
      break;
  }

  return ScheduledEvent::DAILY;
}

QDateTime
NewScheduledEventDialog::firstEvent() const {
  QTime time = _when->time();
  time.setHMS(time.hour(), time.minute(), 0);
  return QDateTime(_when->date(), time);
}
