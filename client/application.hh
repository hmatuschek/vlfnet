#ifndef APPLICATION_HH
#define APPLICATION_HH

#include <QApplication>
#include <QDir>
#include "lib/location.hh"

class Identity;
class Station;
class LogModel;

class Application : public QApplication
{
  Q_OBJECT

public:
  Application(int &argc, char *argv[]);

  /** Returns the OvlNet node of the station.*/
  Station &station();

  /** Returns a weak reference to the log-message table model. */
  LogModel &log();

  Location location() const;
  void setLocation(const Location &location);

  void bootstrap(const QString &host, uint16_t port);

protected:
  QDir _clientDir;
  Identity *_identity;
  /** A model to capture log messages for display. */
  LogModel *_logmodel;
  Station *_station;
};

#endif // APPLICATION_HH
