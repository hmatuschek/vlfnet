#ifndef OSMWIDGET_HH
#define OSMWIDGET_HH

#include <QWebView>
#include "lib/stationlist.hh"
#include <ovlnet/crypto.hh>

class OSMWidget : public QWebView
{
  Q_OBJECT

public:
  OSMWidget(QWidget *parent=0);

public slots:
  void setLocation(const Location &location);
  void addStation(const StationItem &station);
};

#endif // OSMWIDGET_HH
