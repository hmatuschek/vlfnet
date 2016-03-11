#ifndef DATASETFILE_HH
#define DATASETFILE_HH

#include <QDir>
#include <QString>
#include <QDateTime>
#include <QJsonArray>
#include <QJsonObject>
#include <ovlnet/buckets.hh>
#include "location.hh"


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
  /** Specifies the number of parent dataset which this dataset was derived from.
   * The parent IDs follow the file header immediately. */
  uint16_t parents;
} DataSetFileHeader;


typedef struct __attribute__((packed)) {
  float longitude;
  float latitude;
  float height;
} DataSetHeader;


class DataSetFile
{
public:
  DataSetFile(const QString &filename);

  bool isValid() const;

  const QString &filename() const;
  const QDateTime &datetime() const;
  size_t samples() const;
  size_t sampleRate() const;
  size_t numParents() const;
  const Identifier &parent(size_t i) const;
  const QVector<Identifier> &parents() const;
  size_t numDatasets() const;
  const Location &datasetLocation(size_t i) const;
  bool readDataset(size_t i, int16_t *data) const;

  QJsonObject toJson() const;

protected:
  void _reset();

protected:
  QString _filename;
  QDateTime _timestamp;
  size_t _numSamples;
  size_t _sampleRate;
  QVector<Identifier> _parents;
  QVector< QPair<Location, size_t> > _datasets;
};


class DataSetDir
{
public:
  explicit DataSetDir(const QString &directory);

  bool addDataSet(const QString &name);
  bool addDataSet(const Identifier &id);

  void reload();

  QJsonArray toJson() const;

protected:
  QDir _dir;
  QHash<Identifier, DataSetFile> _datasets;
};

#endif // DATASETFILE_HH
