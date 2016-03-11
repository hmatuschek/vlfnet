#ifndef MAINWINDOW_HH
#define MAINWINDOW_HH

#include <QMainWindow>


class Application;

class MainWindow : public QMainWindow
{
  Q_OBJECT

public:
  explicit MainWindow(Application &app, QWidget *parent = 0);

protected:
  Application &_application;
};

#endif // MAINWINDOW_HH
