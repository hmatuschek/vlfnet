#ifndef ABOUTWIDGET_HH
#define ABOUTWIDGET_HH

#include <QWebView>

class AboutWidget : public QWebView
{
  Q_OBJECT

public:
  AboutWidget(QWidget *parent=0);
};

#endif // ABOUTWIDGET_HH
