#ifndef STATION_HH
#define STATION_HH

#include "location.hh"
#include <ovlnet/node.hh>
#include <ovlnet/httpservice.hh>

class StationList;
class Schedule;
class Receiver;

class Station : public Node, public HttpRequestHandler
{
  Q_OBJECT

public:
  explicit Station(Identity &id, const Location &location,
                   const QString scheduleFile, const QString &dataDir,
                   const QString &audioDeviceName="",
                   const QHostAddress &addr=QHostAddress::Any, uint16_t port=7741,
                   QObject *parent=0);

  /** Returns the location of the station. */
  const Location &location() const;
  /** Sets the location of the station. */
  void setLocation(const Location &loc);

  /** Returns a list of known stations. */
  StationList &stations();

  /** Returns the schedule of the station. */
  Schedule &schedule();

  /** Filters HTTP requests. */
  bool acceptReqest(HttpRequest *request);
  /** Processes accepted HTTP requests. */
  HttpResponse *processRequest(HttpRequest *request);

protected:
  /** My location. */
  Location _location;
  /** A list of known stations. */
  StationList *_stations;
  /** The reception schedule of the station. */
  Schedule *_schedule;
  /** The receiver. */
  Receiver *_receiver;
};



#endif // STATION_HH
