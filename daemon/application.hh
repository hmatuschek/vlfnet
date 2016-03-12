#ifndef APPLICATION_HH
#define APPLICATION_HH

#include <QCoreApplication>
#include <QDir>
#include <QTimer>


class Station;
class Settings;
class Identity;


class Application : public QCoreApplication
{
  Q_OBJECT

public:
  Application(int &argc, char *argv[]);
  virtual ~Application();

  Station &station();

protected slots:
  void updateDDNS();

protected:
  Station *_station;
  Settings *_settings;
  QDir _daemonDir;
  QTimer _ddnsTimer;
};

#endif // APPLICATION_HH
