#ifndef NEWSCHEDULEDEVENTDIALOG_HH
#define NEWSCHEDULEDEVENTDIALOG_HH

#include <QDialog>
#include "lib/schedule.hh"


class QComboBox;
class QDateTimeEdit;

class NewScheduledEventDialog : public QDialog
{
  Q_OBJECT

public:
  NewScheduledEventDialog(QWidget *parent=0);

  ScheduledEvent::Type type() const;
  QDateTime firstEvent() const;

protected:
  QComboBox *_type;
  QDateTimeEdit *_when;
};

#endif // NEWSCHEDULEDEVENTDIALOG_HH
