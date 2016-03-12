#include "stationlistview.hh"
#include "application.hh"
#include "lib/station.hh"
#include "lib/stationlist.hh"
#include "locationeditdialog.hh"
#include "osmwidget.hh"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLabel>
#include <QPushButton>
#include <QInputDialog>
#include <QTabWidget>


#define N_KBYTE 1024
#define N_MBYTE (N_KBYTE*N_KBYTE)
#define N_GBYTE (N_MBYTE*N_KBYTE)
#define N_TBYTE (N_GBYTE*N_KBYTE)

inline QString formatVolume(double amount) {
  if (amount < N_KBYTE) {
    return QString("%1 b").arg(QString::number(amount, 'f', 2));
  }

  if (amount < N_MBYTE) {
    return QString("%1 kb").arg(QString::number(amount/N_KBYTE, 'f', 2));
  }

  if (amount < N_GBYTE) {
    return QString("%1 Mb").arg(QString::number(amount/N_MBYTE, 'f', 2));
  }

  return QString("%1 Gb").arg(QString::number(amount/N_GBYTE, 'f', 2));
}



StationListView::StationListView(Application &app, QWidget *parent)
  : QWidget(parent), _application(app), _updateTimer()
{
  QTabWidget *tabs = new QTabWidget();

  _map = new OSMWidget();
  _map->setLocation(_application.station().location());

  QTableView *lst = new QTableView();
  lst->setModel(&_application.station().stations());

  tabs->addTab(_map, tr("Map"));
  tabs->addTab(lst, tr("Table"));

  _stateLabel = new QLabel(tr("<a href=\"#\">disconnected</a>"));
  _locationLabel = new QLabel(
        QString("<a href=\"#\">%1</a>").arg(
          _application.station().location().toString()));
  _locationLabel->setTextInteractionFlags(Qt::TextBrowserInteraction);

  _numNodesLabel = new QLabel("0/0");
  _numSocksLabel = new QLabel("0");
  _inRateLabel = new QLabel(tr("--- b (--- b/s)"));
  _outRateLabel = new QLabel(tr("--- b (--- b/s)"));

  _updateTimer.setInterval(1000);
  _updateTimer.setSingleShot(false);

  QFormLayout *form_left = new QFormLayout();
  form_left->addRow(tr("Identifier"), new QLabel(_application.station().id().toBase32()));
  form_left->addRow(tr("Location"), _locationLabel);
  form_left->addRow(tr("Nodes/Stations"), _numNodesLabel);
  form_left->addRow(tr("Connections"), _numSocksLabel);

  QFormLayout *form_right = new QFormLayout();
  form_right->addRow(tr("State"), _stateLabel);
  form_right->addRow(tr("Received"), _inRateLabel);
  form_right->addRow(tr("Send"), _outRateLabel);

  QHBoxLayout *form_layout = new QHBoxLayout();
  form_layout->addLayout(form_left);
  form_layout->addLayout(form_right);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addLayout(form_layout);
  layout->addWidget(tabs);

  setLayout(layout);

  connect(&_updateTimer, SIGNAL(timeout()), this, SLOT(_onUpdateStats()));
  connect(_locationLabel, SIGNAL(linkActivated(QString)), this, SLOT(_onEditLocation()));
  connect(_stateLabel, SIGNAL(linkActivated(QString)), this, SLOT(_onBootstrap()));
  connect(_map, SIGNAL(loadFinished(bool)), this, SLOT(_onMapLoaded(bool)));
  _updateTimer.start();
}

void
StationListView::_onUpdateStats() {
  if (_application.station().started() && _application.station().numNodes()) {
    _stateLabel->setText(tr("<a href=\"#\">connected</a>"));
  } else {
    _stateLabel->setText(tr("<a href=\"#\">not connected</a>"));
  }
  _numNodesLabel->setText(
        QString("%1/%2")
        .arg(QString::number(_application.station().numNodes()))
        .arg(QString::number(_application.station().stations().numStations())));
  _numSocksLabel->setText(QString::number(_application.station().numSockets()));
  _inRateLabel->setText(
        tr("%1 (%2/s)")
        .arg(formatVolume(_application.station().bytesReceived()))
        .arg(formatVolume(_application.station().inRate())));
  _outRateLabel->setText(
        tr("%1 (%2/s)")
        .arg(formatVolume(_application.station().bytesSend()))
        .arg(formatVolume(_application.station().outRate())));
}

void
StationListView::_onBootstrap() {
  QString hostport = QInputDialog::getText(0, tr("Enter bootstrap node"),
                                           tr("Enter a boostrap node as HOST[:PORT]:"));
  if (hostport.isEmpty()) { return; }

  QString host = hostport;
  uint16_t port = 7741;

  if (hostport.contains(':')) {
    int idx = hostport.indexOf(':');
    host = hostport.left(idx);
    port = hostport.mid(idx+1).toUInt();
  }
  _application.bootstrap(host, port);
}

void
StationListView::_onEditLocation() {
  LocationEditDialog dialog(_application.station().location());
  if (QDialog::Accepted != dialog.exec()) {
    return;
  }

  _application.station().setLocation(dialog.location());
  _map->setLocation(_application.station().location());
  _locationLabel->setText(QString("<a href=\"#\">%1</a>").arg(_application.station().location().toString()));
}

void
StationListView::_onMapLoaded(bool ok) {
  _map->setLocation(_application.station().location());
}
