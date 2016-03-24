#include "socksservice.hh"
#include "station.hh"

#include <QFile>
#include <QJsonDocument>
#include <QJsonArray>


/* ********************************************************************************************* *
 * Implementation of SocksService
 * ********************************************************************************************* */
SocksService::SocksService(const QString &whitelist, Station &station)
  : AbstractService(), _station(station), _whitelist()
{
  QFile file(whitelist);
  if (! file.open(QIODevice::ReadOnly)) {
    logError() << "Cannot open whitelist " << whitelist << ".";
    return;
  }
  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
  if (QJsonParseError::NoError != err.error) {
    logError() << "Cannot parse whitelist " << whitelist
               << ": " << err.errorString();
    return;
  }
  if (! doc.isArray()) {
    logError() << "Cannot process whitelist " << whitelist
               << ": Not an array.";
    return;
  }
  QJsonArray arr = doc.array();
  for (int i=0; i<arr.size(); i++) {
    _whitelist.insert(Identifier::fromBase32(arr.at(i).toString()));
  }
}

SecureSocket *
SocksService::newSocket() {
  return new SocksOutStream(_station);
}

bool
SocksService::allowConnection(const NodeItem &peer) {
  if (_whitelist.contains(peer.id())) {
    logDebug() << "SocksService: Allow SOCKS connection from " << peer.id() << ".";
    return true;
  }
  logDebug() << "SocksService: Deny SOCKS connection from " << peer.id() << ".";
  return false;
}

void
SocksService::connectionStarted(SecureSocket *stream) {
  logDebug() << "SocksService: SOCKS connection to " << stream->peerId() << " started.";
  dynamic_cast<SocksOutStream *>(stream)->open(QIODevice::ReadWrite);
}

void
SocksService::connectionFailed(SecureSocket *stream) {
  logDebug() << "SocksService: SOCKS connection to " << stream->peerId() << " failed.";
  delete stream;
}

