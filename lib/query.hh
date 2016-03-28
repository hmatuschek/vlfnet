#ifndef QUERY_HH
#define QUERY_HH

#include <ovlnet/httpclient.hh>
#include "schedule.hh"
#include <QTemporaryFile>

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
class JsonQuery: public QObject
{
  Q_OBJECT

public:
  JsonQuery(const QString &path, Node &node, const Identifier &remote);
  JsonQuery(const QString &path, Node &node, const NodeItem &remote);

signals:
  void failed();

protected:
  virtual void finished(const QJsonDocument &doc);

protected slots:
  void _onNodeFound(const NodeItem &node);
  void _onConnectionEstablished();
  void _onResponseReceived();
  void _onError();
  void _onReadyRead();

protected:
  QString _query;
  Node &_node;
  Identifier _remoteId;
  HttpClientConnection *_connection;
  HttpClientResponse *_response;
  size_t _responseLength;
  QByteArray _buffer;
};


/** Self-destructing query for the station info. */
class StationInfoQuery: public JsonQuery
{
  Q_OBJECT

public:
  StationInfoQuery(Node &node, const Identifier &remote);
  StationInfoQuery(Node &node, const NodeItem &remote);

signals:
  void stationInfoReceived(const StationItem &station);

protected:
  void finished(const QJsonDocument &doc);
};


/** Self-destructing query for the list of stations from another station. */
class StationListQuery: public JsonQuery
{
  Q_OBJECT

public:
  StationListQuery(Node &node, const Identifier &remote);
  StationListQuery(Node &node, const NodeItem &remote);

signals:
  void stationListReceived(const QList<Identifier> &ids);

protected:
  void finished(const QJsonDocument &doc);
};


/** Self-destructing query for the station schedule. */
class StationScheduleQuery: public JsonQuery
{
  Q_OBJECT

public:
  StationScheduleQuery(Node &node, const Identifier &remote);
  StationScheduleQuery(Node &node, const NodeItem &remote);

signals:
  void stationScheduleReceived(const Identifier &remote, const QList<ScheduledEvent> &events);

protected:
  void finished(const QJsonDocument &doc);
};

/** Self-destructing query for the dataset list. */
class DataSetListQuery: public JsonQuery
{
  Q_OBJECT

public:
  DataSetListQuery(Node &node, const Identifier &remote);
  DataSetListQuery(Node &node, const NodeItem &remote);

signals:
  void dataSetListReceived(const Identifier &remote, const QJsonObject &lst);

protected:
  void finished(const QJsonDocument &doc);
};


class DownloadDataSetQuery: public QObject
{
  Q_OBJECT

public:
  DownloadDataSetQuery(const Identifier &datasetid, const Identifier &remote, Station &station);
  DownloadDataSetQuery(const Identifier &datasetid, const NodeItem &remote, Station &station);

signals:
  void succeeded();
  void failed();

protected slots:
  void _onNodeFound(const NodeItem &node);
  void _onConnectionEstablished();
  void _onResponseReceived();
  void _onError();
  void _onReadyRead();

protected:
  Station &_station;
  Identifier _dataSetID;
  HttpClientConnection *_connection;
  HttpClientResponse *_response;
  size_t _responseLength;
  QTemporaryFile _buffer;
  EVP_MD_CTX _mdctx;
};

#endif // QUERY_HH
