#ifndef QUERY_HH
#define QUERY_HH

#include <ovlnet/httpclient.hh>
#include "schedule.hh"

class StationItem;


/** Self-destructing search query to resolve a node identifier of a station. */
class StationResolveQuery: public QObject, public SearchQuery
{
  Q_OBJECT

public:
  StationResolveQuery(Node &node, const Identifier &id);

  void succeeded();
  void failed();

signals:
  void found(const NodeItem &node);
  void notFound();
};


/** Self-destructing query for the station info. */
class StationInfoQuery: public QObject
{
  Q_OBJECT

public:
  StationInfoQuery(Node &node, const Identifier &remote);
  StationInfoQuery(Node &node, const NodeItem &remote);

signals:
  void stationInfoReceived(const StationItem &station);
  void failed();

protected slots:
  void _onNodeFound(const NodeItem &node);
  void _onConnectionEstablished();
  void _onResponseReceived();
  void _onError();
  void _onReadyRead();

protected:
  Node &_node;
  HttpClientConnection *_connection;
  HttpClientResponse *_response;
  size_t _responseLength;
  QByteArray _buffer;
};


/** Self-destructing query for the list of stations from another station. */
class StationListQuery: public QObject
{
  Q_OBJECT

public:
  StationListQuery(Node &node, const Identifier &remote);
  StationListQuery(Node &node, const NodeItem &remote);

signals:
  void stationListReceived(const QList<Identifier> &ids);
  void failed();

protected slots:
  void _onNodeFound(const NodeItem &node);
  void _onConnectionEstablished();
  void _onResponseReceived();
  void _onError();
  void _onReadyRead();

protected:
  Node &_node;
  HttpClientConnection *_connection;
  HttpClientResponse *_response;
  size_t _responseLength;
  QByteArray _buffer;
};


/** Self-destructing query for the station schedule. */
class StationScheduleQuery: public QObject
{
  Q_OBJECT

public:
  StationScheduleQuery(Node &node, const Identifier &remote);
  StationScheduleQuery(Node &node, const NodeItem &remote);

signals:
  void stationScheduleReceived(const QList<ScheduledEvent> &events);
  void failed();

protected slots:
  void _onNodeFound(const NodeItem &node);
  void _onConnectionEstablished();
  void _onResponseReceived();
  void _onError();
  void _onReadyRead();

protected:
  Node &_node;
  HttpClientConnection *_connection;
  HttpClientResponse *_response;
  size_t _responseLength;
  QByteArray _buffer;
};


#endif // QUERY_HH
