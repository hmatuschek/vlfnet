#ifndef LOCATIONEDITDIALOG_HH
#define LOCATIONEDITDIALOG_HH

#include <QDialog>
#include "lib/location.hh"


class QLineEdit;
class QGeoPositionInfoSource;
class QGeoPositionInfo;


class LocationEditDialog : public QDialog
{
  Q_OBJECT

public:
  LocationEditDialog(const Location &location, QWidget *parent=0);

  Location location() const;

protected slots:
  void _onLocate();
  void _onLocateSucceeded(const QGeoPositionInfo &where);
  void _onLocateFailed();

protected:
  QGeoPositionInfoSource *_geoInfo;
  QLineEdit *_latitude;
  QLineEdit *_longitude;
  QLineEdit *_height;
};

#endif // LOCATIONEDITDIALOG_HH
