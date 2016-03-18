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


class Station : public Node, public HttpRequestHandler
{
  Q_OBJECT

public:
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

  QAudioDeviceInfo inputDevice() const;
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



class CtrlResponse: public HttpResponse
{
  Q_OBJECT

public:
  explicit CtrlResponse(Station &station, HttpRequest *request);

public slots:
  void process(const QJsonDocument &doc);

protected slots:
  void _onHeadersSend();
  void _onReadyRead();
  void _onBytesWritten(qint64 n);

protected:
  Station &_station;
  HttpMethod _method;
  QString _path;
  size_t _requestSize;
  QByteArray _buffer;
};


#endif // STATION_HH
