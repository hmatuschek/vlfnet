#ifndef STATION_HH
#define STATION_HH

#include "location.hh"
#include <ovlnet/node.hh>
#include <ovlnet/httpservice.hh>

class StationList;


class Station : public Node, public HttpRequestHandler
{
  Q_OBJECT

public:
  explicit Station(Identity &id, const Location &location,
                   const QHostAddress &addr=QHostAddress::Any, uint16_t port=7741,
                   QObject *parent=0);

  /** Returns the location of the station. */
  const Location &location() const;
  /** Sets the location of the station. */
  void setLocation(const Location &loc);

  /** Returns a list of known stations. */
  StationList &stations();

  bool acceptReqest(HttpRequest *request);
  HttpResponse *processRequest(HttpRequest *request);

protected:
  /** My location. */
  Location _location;
  /** A list of known stations. */
  StationList *_stations;
};



#endif // STATION_HH
