#include "datasetlistview.hh"
#include "lib/datasetfile.hh"
#include "lib/station.hh"

#include <QTableView>
#include <QVBoxLayout>

DataSetListView::DataSetListView(Station &station, QWidget *parent)
  : QWidget(parent), _datadir(station.datasets())
{
  _remoteList = new RemoteDataSetList(station, this);

  QTableView *localTable = new QTableView();
  localTable->setModel(&_datadir);
  QTableView *remoteTable = new QTableView();
  remoteTable->setModel(_remoteList);

  QTabWidget *tabs = new QTabWidget();
  tabs->addTab(localTable, tr("Local Datasets"));
  tabs->addTab(remoteTable, tr("Remote Datasets"));

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addWidget(tabs);

  setLayout(layout);
}

