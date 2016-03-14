#ifndef DATASETLISTVIEW_HH
#define DATASETLISTVIEW_HH

#include <QWidget>

class Station;
class DataSetDir;
class RemoteDataSetList;

class DataSetListView : public QWidget
{
  Q_OBJECT
public:
  explicit DataSetListView(Station &station, QWidget *parent = 0);

protected:
  DataSetDir &_datadir;
  RemoteDataSetList *_remoteList;
};

#endif // DATASETLISTVIEW_HH
