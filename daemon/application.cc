#include "application.hh"
#include "lib/station.hh"
#include "lib/bootstraplist.hh"
#include <ovlnet/crypto.hh>
#include <ovlnet/optionparser.hh>

#include <QStandardPaths>


Application::Application(int &argc, char *argv[])
  : QCoreApplication(argc, argv), _station(0), _identity(0)
{
  // Set application name
  setApplicationName("vlfdaemon");
  setOrganizationName("io.github.hmatuschek");
  setOrganizationDomain("io.github.hmatuschek");

  // Parse command line options
  OptionParser parser;
  parser.add("config-dir");
  parser.parse(argc, argv);

  // Determine default application data directory.
  _daemonDir = QStandardPaths::writableLocation(
        QStandardPaths::AppDataLocation);
  // if config dir is passed by argument
  if (parser.hasOption("config-dir")) {
    _daemonDir = parser.option("config-dir");
  }
  logDebug() << "Load VLF daemon settings from " << _daemonDir.absolutePath() << ".";

  // check if VLF directory exists
  if (! _daemonDir.exists()) {
    if (! _daemonDir.mkpath(_daemonDir.absolutePath())) {
      logError() << "Cannot create path " << _daemonDir.absolutePath();
    }
  }

  // Load or create identity
  QString idFile(_daemonDir.canonicalPath()+"/identity.pem");
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
  QString locFile(_daemonDir.canonicalPath()+"/location.json");
  if (QFile::exists(locFile)) {
    location = Location::fromFile(locFile);
  }

  // Create DHT instance
  _station = new Station(*_identity, location, _daemonDir.canonicalPath()+"/schedule.json",
                         _daemonDir.canonicalPath()+"/data/",
                         "",
                         QHostAddress::Any, 7741, this);

  // Load OVLNet boostrap list
  QList< QPair<QString, uint16_t> > boot = BootstrapList::fromFile(
        _daemonDir.canonicalPath()+"/bootstrap.json");
  // ping all boostrap nodes
  QList< QPair<QString, uint16_t> >::iterator nodeItem = boot.begin();
  for (; nodeItem != boot.end(); nodeItem++) {
    logDebug() << "Bootstrap from " << nodeItem->first << ":" << nodeItem->second;
    _station->ping(nodeItem->first, nodeItem->second);
  }


}

Application::~Application() {
  if (_identity) delete _identity;
}

Station &
Application::station() {
  return *_station;
}



