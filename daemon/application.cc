#include "application.hh"
#include "lib/station.hh"
#include "lib/bootstraplist.hh"
#include "settings.hh"
#include <ovlnet/crypto.hh>
#include <ovlnet/optionparser.hh>

#include <QStandardPaths>
#include <QNetworkAccessManager>
#include <QNetworkRequest>

Application::Application(int &argc, char *argv[])
  : QCoreApplication(argc, argv), _station(0), _settings(0), _ddnsTimer()
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
        QStandardPaths::GenericDataLocation);
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

  // Load settings
  _settings = new Settings(_daemonDir.canonicalPath()+"/settings.json", this);

  // Create DHT instance
  _station = new Station(_daemonDir.canonicalPath(), QHostAddress::Any, 7741, this);

  // DDNS stuff (update every hour)
  _ddnsTimer.setInterval(360000);
  if (_settings->hasDDNSUpdateUrl()) {
    connect(&_ddnsTimer, SIGNAL(timeout()), this, SLOT(updateDDNS()));
    _ddnsTimer.start();
    updateDDNS();
  }
}

Application::~Application() {
  // pass...
}

Station &
Application::station() {
  return *_station;
}

void
Application::updateDDNS() {
  QNetworkAccessManager *net = new QNetworkAccessManager();
  // self destruction
  connect(net, SIGNAL(finished(QNetworkReply*)), net, SLOT(deleteLater()));
  // Just fire-up a request to the update URL
  net->get(QNetworkRequest(QUrl(_settings->ddnsUpdateUrl())));
}


