#include "osmwidget.hh"
#include <QWebFrame>
#include <QFile>


OSMWidget::OSMWidget(QWidget *parent)
  : QWebView(parent)
{
  QWebSettings::globalSettings()->setAttribute(QWebSettings::DeveloperExtrasEnabled, true);

  QFile file("://stations_map.html");
  if (file.open(QIODevice::ReadOnly)) {
    setHtml(file.readAll());
  } else {
    logError() << "Cannot open resource: " << file.fileName();
  }
}

void
OSMWidget::setLocation(const Location &location) {
  page()->mainFrame()->evaluateJavaScript(
        QString("setLocal(%1, %2)").arg(location.longitude()).arg(location.latitude()));
}

void
OSMWidget::addStation(const StationItem &station) {
  page()->mainFrame()->evaluateJavaScript(
        QString("addStation(\"%1\", %2, %3)")
        .arg(station.id().toBase32())
        .arg(station.location().longitude())
        .arg(station.location().latitude()));
}
