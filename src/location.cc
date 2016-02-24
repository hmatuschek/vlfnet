#include "location.hh"
#include <cmath>

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



