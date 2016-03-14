#include "mainwindow.hh"
#include "application.hh"
#include "logwindow.hh"
#include "stationlistview.hh"
#include "datasetlistview.hh"
#include "monitor.hh"
#include "scheduleview.hh"
#include "aboutwidget.hh"
#include <lib/station.hh>
#include <QTabWidget>
#include <QTableView>

MainWindow::MainWindow(Application &app, QWidget *parent)
  : QMainWindow(parent), _application(app)
{
  setWindowTitle(tr("VLF Observatory Network Client"));

  QTabWidget *tabs = new QTabWidget();
  tabs->addTab(new StationListView(_application), tr("Stations"));
  tabs->addTab(new ScheduleView(_application), tr("Schedule"));
  tabs->addTab(new DataSetListView(_application.station()), tr("Datasets"));
  tabs->addTab(new Monitor(_application), tr("Monitor"));
  tabs->addTab(new LogWidget(_application), tr("Log"));
  tabs->addTab(new AboutWidget(), tr("About"));
  setCentralWidget(tabs);
}

