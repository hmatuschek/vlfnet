#ifndef APPLICATION_HH
#define APPLICATION_HH

#include <QCoreApplication>
#include <QDir>


class Station;
class Identity;


class Application : public QCoreApplication
{
  Q_OBJECT

public:
  Application(int &argc, char *argv[]);
  virtual ~Application();

  Station &station();

protected:
  Station *_station;
  QDir _daemonDir;
};

#endif // APPLICATION_HH
