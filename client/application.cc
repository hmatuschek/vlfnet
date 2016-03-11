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
  // Load or create identity
  QString idFile(_clientDir.canonicalPath()+"/identity.pem");
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
  QString locFile(_clientDir.canonicalPath()+"/location.json");
  if (QFile::exists(locFile)) {
    location = Location::fromFile(locFile);
  }

  // Create DHT instance
  _station = new Station(*_identity, location,
                         _clientDir.canonicalPath()+"/schedule.json",
                         _clientDir.canonicalPath()+"/data/",
                         "",
                         QHostAddress::Any, 7742, this);

  // boostrap list
  QList< QPair<QString, uint16_t> > boothost = BootstrapList::fromFile(
        _clientDir.canonicalPath()+"/bootstrap.json");
  QList<QPair<QString, uint16_t>>::iterator host = boothost.begin();
  for (; host != boothost.end(); host++) {
    _station->ping(host->first, host->second);
  }
}


Station &
Application::station() {
  return *_station;
}

LogModel &
Application::log() {
  return *_logmodel;
}

Location
Application::location() const {
  return _station->location();
}

void
Application::setLocation(const Location &location) {
  _station->setLocation(location);
  QFile locFile(_clientDir.canonicalPath()+"/location.json");
  if (locFile.open(QIODevice::WriteOnly)) {
    QJsonDocument doc(location.toJson());
    locFile.write(doc.toJson());
    locFile.flush();
    locFile.close();
  }
}

void
Application::bootstrap(const QString &host, uint16_t port) {
  _station->ping(host, port);
  BootstrapList::add(_clientDir.canonicalPath()+"/bootstrap.json", host, port);
}


