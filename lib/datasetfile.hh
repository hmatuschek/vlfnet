#ifndef DATASETFILE_HH
#define DATASETFILE_HH

#include <QDir>
#include <QString>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <QAbstractTableModel>
#include <ovlnet/buckets.hh>
#include "location.hh"
#include "stationlist.hh"

#include <ovlnet/dht_config.hh>


class Timeseries
{
public:
  typedef struct __attribute__((packed)) {
    float longitude;
    float latitude;
    float height;
    char  identifier[OVL_HASH_SIZE];
  } Header;

public:
  Timeseries();
  Timeseries(size_t offset, const Header *header);
  Timeseries(const Timeseries &other);

  Timeseries &operator =(const Timeseries &other);

  size_t offset() const;
  const Identifier &identifier() const;
  const Location &location() const;

  QJsonObject toJson() const;

protected:
  size_t     _offset;
  Identifier _identifier;
  Location   _location;
};


class DataSetFile
{
public:
  typedef struct __attribute__((packed)) {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
    uint16_t datasets;
    uint32_t samples;
    uint32_t rate;
  } Header;

public:
  DataSetFile();
  DataSetFile(const QString &filename);
  DataSetFile(const DataSetFile &other);

  DataSetFile &operator=(const DataSetFile &other);

  bool isValid() const;

  const QString &filename() const;
  const QDateTime &datetime() const;
  size_t samples() const;
  size_t sampleRate() const;
  size_t numTimeseries() const;
  const Timeseries &timeseries(size_t i) const;
  bool readTimeseries(size_t i, int16_t *data) const;

  QJsonObject toJson() const;

protected:
  void _reset();

protected:
  QString _filename;
  QDateTime _timestamp;
  size_t _numSamples;
  size_t _sampleRate;
  QVector<Timeseries> _datasets;
};


/** Implements the dataset database.
 * This database is stored as a directory containing all datasets as separate files.
 * The name of these files corresponds to the ID of the dataset. Upon construction, the DB
 * reads the meta data from all datasets in the directory. */
class DataSetDir: public QAbstractTableModel
{
  Q_OBJECT

public:
  /** Constructs the dataset database located at the specified @c directory. */
  explicit DataSetDir(const QString &directory);

  /** Returns the path to the data directory. */
  QString path() const;
  /** Retunrs the number of datasets stored in the database. */
  size_t numDatasets() const;

  /** Returns @c true if the database contains the specified dataset. */
  bool contains(const Identifier &id) const;
  /** Returns @c true if the database contains the specified dataset implicitly. */
  bool containsImplicitly(const Identifier &id) const;

  /** Return the @c DataSetFile for the specified dataset. */
  DataSetFile dataset(const Identifier &id) const;
  /** Adds a dataset to the database. The dataset must be present in the directory. */
  bool addDataset(const QString &name);
  /** Adds a dataset to the database. The dataset must be present in the directory. */
  bool addDataset(const Identifier &id);

  /** Reloads the database. */
  void reload();

  /** Returns the database as a Json array. */
  QJsonObject toJson() const;

  /* *** Implementation of QAbstactTableModel */
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

protected:
  /** The database directory. */
  QDir _dir;
  /** The vector of identifiers of all datasets in the database. */
  QVector<Identifier> _datasetOrder;
  /** The table mapping dataset identifier to datasets. */
  QHash<Identifier, DataSetFile> _datasets;
  /** Reverse lookup table to match parents to derived datasets. */
  QHash<Identifier, Identifier> _parents;
};


class RemoteTimeseries
{
public:
  RemoteTimeseries();
  RemoteTimeseries(const QJsonObject &obj);
  RemoteTimeseries(const RemoteTimeseries &other);

  RemoteTimeseries &operator =(const RemoteTimeseries &other);

  const Identifier &identifier() const;
  const Location &location() const;

protected:
  Identifier _identifier;
  Location _location;
};


class RemoteDataSet
{
public:
  RemoteDataSet();
  RemoteDataSet(const Identifier &remote, const QJsonObject &obj);
  RemoteDataSet(const RemoteDataSet &other);

  RemoteDataSet &operator =(const RemoteDataSet &other);

  const QDateTime &datetime() const;

  size_t samples() const;
  size_t sampleRate() const;

  size_t numTimeseries() const;
  const RemoteTimeseries &timeseries(size_t i) const;

  size_t numRemotes() const;
  void addRemote(const Identifier &remote);
  const QSet<Identifier> &remotes() const;

protected:
  QDateTime _timestamp;
  size_t _samples;
  size_t _sampleRate;
  QVector<Identifier> _parents;
  QVector<RemoteTimeseries> _timeseries;
  QSet<Identifier> _remotes;
};


class RemoteDataSetList: public QAbstractTableModel
{
  Q_OBJECT

public:
  explicit RemoteDataSetList(Station &station, QObject *parent=0);

  /** Retunrs the number of datasets stored in the database. */
  size_t numDatasets() const;
  /** Returns @c true if the database contains the specified identifier. */
  bool contains(const Identifier &id) const;
  /** Return the @c RemoteDataSet for the specified dataset id. */
  RemoteDataSet dataset(const Identifier &id) const;

  /* *** Implementation of QAbstractTableModel interface. *** */
  int rowCount(const QModelIndex &parent) const;
  int columnCount(const QModelIndex &parent) const;
  QVariant data(const QModelIndex &index, int role) const;
  QVariant headerData(int section, Qt::Orientation orientation, int role) const;

public slots:
  void add(const Identifier &remote, const QJsonObject &list);

protected slots:
  void _onUpdateRemoteDataSets(const StationItem &station);

protected:
  Station &_station;
  QVector<Identifier> _datasetOrder;
  QHash<Identifier, RemoteDataSet> _datasets;
};


#endif // DATASETFILE_HH
