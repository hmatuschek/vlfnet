#include "locationeditdialog.hh"
#include <ovlnet/logger.hh>
#include <QPushButton>
#include <QLineEdit>
#include <QDoubleValidator>
#include <QFormLayout>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QGeoPositionInfoSource>
#include <QMessageBox>


LocationEditDialog::LocationEditDialog(const Location &location, QWidget *parent)
  : QDialog(parent)
{
  setWindowTitle(tr("Set station location"));

  logDebug() << "Avalilable location sources: "
             << QGeoPositionInfoSource::availableSources().join(", ") << ".";
  _geoInfo = QGeoPositionInfoSource::createDefaultSource(this);

  _latitude = new QLineEdit();
  _latitude->setValidator(new QDoubleValidator(-90, 90, 5));
  _longitude = new QLineEdit();
  _longitude->setValidator(new QDoubleValidator(-180, 180, 5));
  _height = new QLineEdit();
  _height->setValidator(new QDoubleValidator(0, 10e3, 0));

  if (! location.isNull()) {
    _latitude->setText(QString::number(location.latitude()));
    _longitude->setText(QString::number(location.longitude()));
    _height->setText(QString::number(location.height()));
  } else {
    _latitude->setText("0");
    _longitude->setText("0");
    _height->setText("0");
  }

  QDialogButtonBox *bb = new QDialogButtonBox();
  QPushButton *locate = bb->addButton(tr("Locate"), QDialogButtonBox::ActionRole);
  if (0 == _geoInfo) { locate->setEnabled(false); }
  bb->addButton(QDialogButtonBox::Cancel);
  bb->addButton(QDialogButtonBox::Save);

  QFormLayout *form = new QFormLayout();
  form->addRow(tr("Longitude (dec. deg.)"), _longitude);
  form->addRow(tr("Latitude (dec. deg.)"), _latitude);
  form->addRow(tr("Height (m above N.N.)"), _height);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addLayout(form);
  layout->addWidget(bb);

  setLayout(layout);

  connect(bb, SIGNAL(accepted()), this, SLOT(accept()));
  connect(bb, SIGNAL(rejected()), this, SLOT(reject()));
  connect(locate, SIGNAL(clicked(bool)), this, SLOT(_onLocate()));

  if (_geoInfo) {
    connect(_geoInfo, SIGNAL(positionUpdated(QGeoPositionInfo)),
            this, SLOT(_onLocateSucceeded(QGeoPositionInfo)));
    connect(_geoInfo, SIGNAL(error(QGeoPositionInfoSource::Error)),
            this, SLOT(_onLocateFailed()));
    connect(_geoInfo, SIGNAL(updateTimeout()),
            this, SLOT(_onLocateFailed()));
  }
}

void
LocationEditDialog::_onLocate() {
  _geoInfo->requestUpdate(10000);
}

void
LocationEditDialog::_onLocateSucceeded(const QGeoPositionInfo &where) {
  _latitude->setText(QString::number(where.coordinate().latitude()));
  _longitude->setText(QString::number(where.coordinate().longitude()));
  _height->setText(QString::number(where.coordinate().altitude()));
}

void
LocationEditDialog::_onLocateFailed() {
  QMessageBox::critical(0, tr("Cannot determine location"),
                        tr("Cannot determine current location."));
}

Location
LocationEditDialog::location() const {
  return Location(_longitude->text().toDouble(),
                  _latitude->text().toDouble(),
                  _height->text().toDouble());
}
