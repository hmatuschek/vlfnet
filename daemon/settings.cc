#include "settings.hh"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <ovlnet/logger.hh>


Settings::Settings(const QString &path, QObject *parent)
  : QObject(parent), _path(path), _port(4471), _ddnsUpdateUrl(""), _externalPort(0)
{
  QFile file(path);
  if (! file.open(QIODevice::ReadOnly)) {
    logError() << "Cannot read settings from file " << _path << ".";
    return;
  }

  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
  if (QJsonParseError::NoError != err.error) {
    logError() << "Cannot parse settings from file " << _path
               << ": " << err.errorString() << ".";
    return;
  }

  if (! doc.isObject()) {
    logError() << "Cannot process settings in file " << _path
               << ": Root is not an object.";
    return;
  }

  QJsonObject obj = doc.object();
  if (obj.contains("port")) {
    _port = obj.value("port").toInt(_port);
  }
  if (obj.contains("ddns")) {
    _ddnsUpdateUrl = obj.value("ddns").toString(_ddnsUpdateUrl);
  }
  if (obj.contains("upnp_external_port")) {
    _externalPort = obj.value("upnp_external_port").toInt();
  }
}


uint16_t
Settings::port() const {
  return _port;
}

void
Settings::setPort(uint16_t port) {
  _port = port;
  _save();
}

bool
Settings::hasDDNSUpdateUrl() const {
  return ! _ddnsUpdateUrl.isEmpty();
}

const QString &
Settings::ddnsUpdateUrl() const {
  return _ddnsUpdateUrl;
}

void
Settings::setDDNSUpdateURL(const QString &url) {
  _ddnsUpdateUrl = url;
  _save();
}

bool
Settings::hasUPnPExternalPort() const {
  return 0 != _externalPort;
}

uint16_t
Settings::upnpExternalPort() const {
  return _externalPort;
}

void
Settings::setUPnPExternalPort(uint16_t port) {
  _externalPort = port;
}

void
Settings::_save() const {
  QJsonObject obj;
  obj.insert("port", _port);
  if (! _ddnsUpdateUrl.isEmpty()) {
    obj.insert("ddns", _ddnsUpdateUrl);
  }
  if (0 != _externalPort) {
    obj.insert("upnp_external_port", _externalPort);
  }

  QFile file(_path);
  if (! file.open(QIODevice::WriteOnly)) {
    logError() << "Cannot save settings to file " << _path << ".";
    return;
  }

  file.write(QJsonDocument(obj).toJson());
  file.flush();
  file.close();
}
