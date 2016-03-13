#include "aboutwidget.hh"
#include <QFile>
#include <ovlnet/logger.hh>


AboutWidget::AboutWidget(QWidget *parent)
  : QWebView(parent)
{
  QFile file("://about.html");
  if (file.open(QIODevice::ReadOnly)) {
    setHtml(file.readAll());
  } else {
    logError() << "Cannot open resource: " << file.fileName();
  }
}

