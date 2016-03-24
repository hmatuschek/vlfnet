#ifndef STATION_HH
#define STATION_HH

#include "location.hh"
#include <ovlnet/node.hh>
#include <ovlnet/httpservice.hh>
#include "datasetfile.hh"
#include <QAudioDeviceInfo>

class StationList;
class MergedSchedule;
class Receiver;


/** Central class of all vlfnet stations. It keeps track of all known stations in the network and
 * their schedules and data. */
class Station : public Node, public HttpRequestHandler
{
  Q_OBJECT

public:
  /** Constructs a new vlfnet station using the configuration at the directory specified by
   * @c path. The underlying ovlnet node gets bound to the specified @c addr and @c port. */
  explicit Station(const QString &path, const QHostAddress &addr=QHostAddress::Any,
                   uint16_t port=7741, QObject *parent=0);

  /** Returns the location of the station. */
  const Location &location() const;
  /** Sets the location of the station. */
  void setLocation(const Location &loc);

  /** Returns a list of known stations. */
  StationList &stations();

  /** Returns the schedule of the station. */
  MergedSchedule &schedule();

  /** Returns the datasets held by this station. */
  DataSetDir &datasets();

  /** Returns the configured default reception device. */
  QAudioDeviceInfo inputDevice() const;
  /** Sets the default reception device. */
  bool setInputDevice(const QAudioDeviceInfo &device);

  /** Filters HTTP requests. */
  bool acceptReqest(HttpRequest *request);
  /** Processes accepted HTTP requests. */
  HttpResponse *processRequest(HttpRequest *request);

protected slots:
  /** Gets called periodically until the node is connected to the network. */
  void _onBootstrap();
  /** Gets called once the connection to the network is lost. */
  void _onDisconnected();
  /** Gets called once the connection to the network is established. */
  void _onConnected();

protected:
  /** Path to the configuration directory. */
  QString _path;
  /** My location. */
  Location _location;
  /** A list of known stations. */
  StationList *_stations;
  /** The reception schedule of the station. */
  MergedSchedule *_schedule;
  /** The DB of all datasets. */
  DataSetDir *_datasets;
  /** The receiver. */
  Receiver *_receiver;
  /** Timer to bootstrap the net on connection loss. */
  QTimer _bootstrapTimer;
  /** Whitelist for the remote ctrl. */
  QSet<Identifier> _ctrlWhitelist;
};

#endif // STATION_HH
