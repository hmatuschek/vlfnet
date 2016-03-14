#include "application.hh"
#include "logwindow.hh"
#include "lib/bootstraplist.hh"
#include "lib/station.hh"
#include <ovlnet/crypto.hh>
#include <QDir>
#include <QStandardPaths>
#include <QJsonDocument>

Application::Application(int &argc, char *argv[])
  : QApplication(argc, argv)
{
  // Create log model
  _logmodel = new LogModel();
  Logger::addHandler(_logmodel);

  // Set application name
  setApplicationName("vlfclient");
  setOrganizationName("io.github.hmatuschek");
  setOrganizationDomain("io.github.hmatuschek");

  // Try to load identity from file
  _clientDir = QStandardPaths::writableLocation(
        QStandardPaths::DataLocation);
  // check if VLF directory exists
  if (! _clientDir.exists()) {
    _clientDir.mkpath(_clientDir.absolutePath());
  }

  // Create DHT instance
  _station = new Station(_clientDir.canonicalPath(), QHostAddress::Any, 7742, this);
}


Station &
Application::station() {
  return *_station;
}

LogModel &
Application::log() {
  return *_logmodel;
}

void
Application::bootstrap(const QString &host, uint16_t port) {
  _station->ping(host, port);
  BootstrapList::add(_clientDir.canonicalPath()+"/bootstrap.json", host, port);
}


