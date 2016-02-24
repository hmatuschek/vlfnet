#ifndef LOCATION_HH
#define LOCATION_HH

class Location
{
public:
  Location();
  /** Constructs a location from longitude, latitude and height.
   * @param lon Specifies the longitude in degrees.
   * @param lat Specifies the latitude in degrees.
   * @param height Specifies the heigth above sealevel in meters. */
  Location(double lon, double lat, double height);
  Location(const Location &other);

  Location &operator= (const Location &other);

  bool isNull() const;

  double longitude() const;
  double latitude() const;
  double height() const;

  /** Direct distance between two points. */
  double dist(const Location &other);

protected:
  /** Longitude in radians east. */
  double _longitude;
  /** Latitude in radians north. */
  double _latitude;
  /** Radius in km from earths center. */
  double _radius;
};


#endif // LOCATION_HH
