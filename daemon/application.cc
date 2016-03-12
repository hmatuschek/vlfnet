#include "application.hh"
#include "lib/station.hh"
#include "lib/bootstraplist.hh"
#include <ovlnet/crypto.hh>
#include <ovlnet/optionparser.hh>

#include <QStandardPaths>


Application::Application(int &argc, char *argv[])
  : QCoreApplication(argc, argv), _station(0)
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
  logDebug() << "Load VLF daemon from " << _daemonDir.absolutePath() << ".";

  // check if VLF directory exists
  if ((!_daemonDir.exists()) && (! _daemonDir.mkpath(_daemonDir.absolutePath()))) {
    logError() << "Cannot create path " << _daemonDir.absolutePath();
    return;
  }

  // Create DHT instance
  _station = new Station(_daemonDir.canonicalPath(), "",
                         QHostAddress::Any, 7741, this);
}

Application::~Application() {
  // pass...
}

Station &
Application::station() {
  return *_station;
}



