#include "location.hh"

#include <cmath>

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

#include <ovlnet/logger.hh>


/* ********************************************************************************************* *
 * Implementation of Location
 * ********************************************************************************************* */
Location::Location()
  : _longitude(0), _latitude(0), _radius(0)
{
  // pass...
}

Location::Location(double lon, double lat, double height)
  : _longitude(lon*M_PI/180.), _latitude(lat*M_PI/180), _radius(height/1000.+6371.0088)
{
  // pass...
}

Location::Location(const QJsonValue &val)
  : _longitude(0), _latitude(0), _radius(0)
{
  if (! val.isObject()) {
    logDebug() << "Cannot construct Location from JSON: Is not an object.";
    return;
  }
  QJsonObject obj = val.toObject();

  if (! obj.contains("longitude")) {
    logDebug() << "Cannot construct Location from JSON: No longitude specified.";
    return;
  }
  _longitude = obj.value("longitude").toDouble()*M_PI/180;

  if (! obj.contains("latitude")) {
    logDebug() << "Cannot construct Location from JSON: No latitude specified.";
    return;
  }
  _latitude = obj.value("latitude").toDouble()*M_PI/180;;

  if (! obj.contains("height")) {
    logDebug() << "Cannot construct Location from JSON: No height specified.";
    return;
  }
  _radius = obj.value("height").toDouble()/1000+6371.0088;
}

Location::Location(const Location &other)
  : _longitude(other._longitude), _latitude(other._latitude), _radius(other._radius)
{
  // pass...
}

Location &
Location::operator =(const Location &other) {
  _longitude = other._longitude;
  _latitude = other._latitude;
  _radius = other._radius;
  return *this;
}

bool
Location::isNull() const {
  return 0 == _radius;
}

double
Location::longitude() const {
  return 180*_longitude/M_PI;
}

double
Location::latitude() const {
  return 180*_latitude/M_PI;
}

double
Location::height() const {
  return (_radius - 6371.0088)*1000.0;
}

double
Location::dist(const Location &other) {
  double dx = _radius*std::sin(_longitude);
  double dy = _radius*std::cos(_longitude);
  double dz = _radius*std::sin(_latitude);
  dx -= other._radius*std::sin(other._longitude);
  dy -= other._radius*std::cos(other._longitude);
  dz -= other._radius*std::sin(other._latitude);
  return std::sqrt(dx*dx + dy*dy + dz*dz);
}

Location
Location::fromFile(const QString &path) {
  QFile locFile(path);
  if (! locFile.open(QIODevice::ReadOnly)) {
    logDebug() << "Cannot read station location from file '" << path << "'.";
    return Location();
  }

  QJsonParseError error;
  QJsonDocument doc = QJsonDocument::fromJson(locFile.readAll(), &error);
  if (QJsonParseError::NoError != error.error) {
    logDebug() << "Cannot parse location file '" << path << "': " << error.errorString() << ".";
    return Location();
  }

  if (! doc.isObject()) {
    logDebug() << "Cannot parse location file '" << path << "': File does not contain an object.";
    return Location();
  }
  QJsonObject obj = doc.object();

  if (! obj.contains("longitude")) {
    logDebug() << "Cannot parse location file '" << path << "': No longitude specified.";
    return Location();
  }
  double longitude = obj.value("longitude").toDouble();

  if (! obj.contains("latitude")) {
    logDebug() << "Cannot parse location file '" << path << "': No latitude specified.";
    return Location();
  }
  double latitude = obj.value("latitude").toDouble();

  if (! obj.contains("height")) {
    logDebug() << "Cannot parse location file '" << path << "': No height specified.";
    return Location();
  }
  double height = obj.value("height").toDouble();

  return Location(longitude, latitude, height);
}

