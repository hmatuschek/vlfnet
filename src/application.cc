#include "application.hh"
#include "logwindow.hh"
#include "station.hh"
#include <ovlnet/crypto.hh>
#include <QDir>
#include <QStandardPaths>


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
  QDir nodeDir = QStandardPaths::writableLocation(
        QStandardPaths::DataLocation);
  // check if VLF directory exists
  if (! nodeDir.exists()) {
    nodeDir.mkpath(nodeDir.absolutePath());
  }
  // Load or create identity
  QString idFile(nodeDir.canonicalPath()+"/identity.pem");
  if (!QFile::exists(idFile)) {
    logInfo() << "No identity found -> create new identity.";
    _identity = Identity::newIdentity();
    if (_identity) { _identity->save(idFile); }
  } else {
    logDebug() << "Load identity from" << idFile;
    _identity = Identity::load(idFile);
  }
  // check if Identity was loaded or created
  if (0 == _identity) {
    logError() << "Error while loading or creating identity.";
    return;
  }

  Location location;
  QString locFile(nodeDir.canonicalPath()+"/location.json");
  if (QFile::exists(locFile)) {
    location = Location::fromFile(locFile);
  }

  // Create log model
  //_logModel = new LogModel();
  Logger::addHandler(_logmodel);

  // Create DHT instance
  _station = new Station(*_identity, location, QHostAddress::Any, 7742, this);
}


Station &
Application::station() {
  return *_station;
}

LogModel &
Application::log() {
  return *_logmodel;
}


