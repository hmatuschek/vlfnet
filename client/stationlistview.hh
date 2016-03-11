#ifndef STATIONLISTVIEW_HH
#define STATIONLISTVIEW_HH

#include <QTableView>
#include <QTimer>

class QLabel;
class Application;
class OSMWidget;

class StationListView : public QWidget
{
  Q_OBJECT

public:
  explicit StationListView(Application &app, QWidget *parent=0);

protected slots:
  void _onUpdateStats();
  void _onBootstrap();
  void _onEditLocation();
  void _onMapLoaded(bool ok);

protected:
  Application &_application;
  QTimer _updateTimer;

  QLabel *_locationLabel;
  QLabel *_stateLabel;
  QLabel *_numNodesLabel;
  QLabel *_numSocksLabel;
  QLabel *_inRateLabel;
  QLabel *_outRateLabel;
  OSMWidget *_map;
};

#endif // STATIONLISTVIEW_HH
