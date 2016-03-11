#ifndef OSMWIDGET_HH
#define OSMWIDGET_HH

#include <QWebView>
#include "lib/location.hh"
#include <ovlnet/crypto.hh>

class OSMWidget : public QWebView
{
  Q_OBJECT

public:
  OSMWidget(QWidget *parent=0);

public slots:
  void setLocation(const Location &location);
  void addStation(const Identifier &id, const Location &location);
};

#endif // OSMWIDGET_HH
