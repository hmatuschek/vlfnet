#include "osmwidget.hh"
#include <QWebFrame>
#include <QFile>


OSMWidget::OSMWidget(QWidget *parent)
  : QWebView(parent)
{
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
OSMWidget::addStation(const Identifier &id, const Location &location) {

}
