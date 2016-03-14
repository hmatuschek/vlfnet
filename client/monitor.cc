#include "monitor.hh"
#include "application.hh"

#include <QPainter>
#include <QPaintEngine>
#include <QFontMetrics>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>

#include <ovlnet/logger.hh>
#include "lib/audio.hh"
#include "lib/station.hh"
#include <fftw3.h>
#include <cmath>


#define N_FFT       8192
#define N_PSD       (N_FFT/2+1)
#define N_PSD_SUM   5
#define N_PLOT      (std::min(1100, N_PSD))
#define PLOT_OFFSET (N_PSD - N_PLOT)
#define N_PLOT_HIST 512
#define DB_MAX      10.
#define DB_MIN      -40.


/* ****************************************************************************************** *
 *  Implementation of ColorMap
 * ****************************************************************************************** */
ColorMap::ColorMap(double min, double max)
  : _min(min), _max(max)
{
  // pass...
}

ColorMap::~ColorMap() {
  // pass...
}

/* ****************************************************************************************** *
 *  Implementation of GrayScaleColorMap
 * ****************************************************************************************** */
GrayScaleColorMap::GrayScaleColorMap(double min, double max)
  : ColorMap(min, max)
{
  // pass...
}

GrayScaleColorMap::~GrayScaleColorMap() {
  // pass...
}

QColor
GrayScaleColorMap::map(const double &value) {
  int h = 255*value;
  return QColor(h,h,h);
}


/* ****************************************************************************************** *
 *  Implementation of LinearColorMap
 * ****************************************************************************************** */
LinearColorMap::LinearColorMap(const QVector<QColor> &colors, double min, double max)
  : ColorMap(min, max), _colors(colors)
{
  // pass...
}

LinearColorMap::~LinearColorMap() {
  // pass...
}

QColor
LinearColorMap::map(const double &value) {
  // Calc indices
  double upper = ceil(value*(_colors.size()-1));
  double lower = floor(value*(_colors.size()-1));
  int idx = int(lower);
  if (lower == upper) { return _colors[idx]; }
  double dv = upper-(value*(_colors.size()-1));
  double dr = dv * (_colors[idx].red()   - _colors[idx+1].red());
  double dg = dv * (_colors[idx].green() - _colors[idx+1].green());
  double db = dv * (_colors[idx].blue()  - _colors[idx+1].blue());
  return QColor(int(_colors[idx+1].red()+dr),
                int(_colors[idx+1].green()+dg),
                int(_colors[idx+1].blue()+db));
}


/* ****************************************************************************************** *
 *  Implementation of MonitorView
 * ****************************************************************************************** */
MonitorView::MonitorView(const QAudioDeviceInfo &device, Application &app, QWidget *parent)
  : QWidget(parent), _application(app), _input(0), _fftInBuffer(0), _nFFTBuffer(0),
    _fftwOutBuffer(0), _psd(0), _psdSumCount(0),
    _colormap(QVector<QColor> {Qt::black, Qt::red, Qt::yellow, Qt::white}, DB_MIN, DB_MAX),
    _plot(N_PLOT, N_PLOT_HIST)
{
  _input = new Audio(device, this);

  _fftInBuffer = new double[N_FFT]; _nFFTBuffer = N_FFT;
  _fftwOutBuffer = new double[N_FFT];
  _fft = fftw_plan_r2r_1d(N_FFT, _fftInBuffer, _fftwOutBuffer, FFTW_R2HC, FFTW_ESTIMATE);
  _psd = new double[N_PSD];

  _plot.fill(Qt::black);

  connect(_input, SIGNAL(stream(const int16_t*,size_t)),
          this, SLOT(processStream(const int16_t*,size_t)));

  _input->start();

}

MonitorView::~MonitorView() {
  fftw_destroy_plan(_fft);
  if (_fftInBuffer)
    delete[] _fftInBuffer;
  if (_fftwOutBuffer)
    delete[] _fftwOutBuffer;
  if (_psd)
    delete[] _psd;
}

bool
MonitorView::setDevice(const QAudioDeviceInfo &device) {
  if (_input->setDevice(device)) {
    return _input->start();
  }
  return false;
}

void
MonitorView::processStream(const int16_t *data, size_t len) {
  while (len) {
    // Determine number of samples to store in buffer
    size_t n = std::min(len, _nFFTBuffer);
    // store in buffer
    size_t offset = N_FFT-_nFFTBuffer;
    for (size_t i=0; i<n; i++) {
      _fftInBuffer[offset+i] = double(data[i])/(1<<15);
    }
    // Update counts
    _nFFTBuffer -= n; len -= n;
    // If buffer is full -> update plot
    if (0 == _nFFTBuffer) {
      // Perform FFT
      fftw_execute(_fft);
      updatePSD();
      _nFFTBuffer = N_FFT;
    }
  }
}

void
MonitorView::updatePSD() {
  _psd[0] += _fftwOutBuffer[0]*_fftwOutBuffer[0];
  for (size_t i=1; i<N_PSD; i++) {
    _psd[i] +=
          _fftwOutBuffer[i]*_fftwOutBuffer[i]
          + _fftwOutBuffer[N_FFT-i]*_fftwOutBuffer[N_FFT-i];
  }
  _psdSumCount++;
  if (N_PSD_SUM == _psdSumCount) {
    updatePlot();
    // Reset PSD sum
    for (size_t i=0; i<N_PSD; i++) { _psd[i] = 0; }
    _psdSumCount = 0;
  }
}

void
MonitorView::updatePlot() {
  QPainter painter(&_plot);
  painter.drawPixmap(0, 0, _plot, 0, 1, N_PLOT, N_PLOT_HIST);
  for (int i=1; i<N_PLOT; i++) {
    double db = 10*std::log10(_psd[PLOT_OFFSET+i-1])
        - 10*std::log10(N_FFT) - 10*std::log10(N_PSD_SUM);
    db = std::max(DB_MIN, std::min(db, DB_MAX));
    painter.setPen(_colormap(db));
    painter.drawLine(i-1, N_PLOT_HIST-1, i, N_PLOT_HIST-1);
  }
  this->update();
}

void
MonitorView::paintEvent(QPaintEvent *evt) {
  QPainter painter(this);
  painter.drawPixmap(0,0, width(), height(),
                     _plot, 0, 0, N_PLOT, N_PLOT_HIST);
  drawKnownStations(painter);
}

void
MonitorView::drawKnownStations(QPainter &painter) {
  double Fs=48e3, dF=Fs/N_FFT, F0=dF*PLOT_OFFSET;
  QMap<QString, double> stations;
  stations.insert("ALPHA",  14.880952e3);
  stations.insert("JXN",  16.4e3);
  stations.insert("RDL",  18.1e3);
  stations.insert("HWU",  18.3e3);
  stations.insert("GBZ",  19.6e3);
  stations.insert("ICV",  20.27e3);
  stations.insert("BETA",  20.5e3);
  stations.insert("FTA",  20.9e3);
  stations.insert("GQD",  22.1e3);
  stations.insert("RJH",  23.0e3);
  stations.insert("DHO",  23.4e3);
  stations.insert("NAA",  24.0e3);

  painter.setPen(QPen(Qt::blue, 3));
  QMap<QString, double>::const_iterator station = stations.begin();
  for (; station != stations.end(); station++) {
    int x = width()*(station.value()-F0)/(dF*N_PLOT), y = height()-20;
    painter.drawLine(x,y, x, y-5);
    int w = painter.fontMetrics().width(station.key());
    painter.drawText(x-w/2, height()-3, station.key());
  }
}


/* ****************************************************************************************** *
 *  Implementation of Monitor
 * ****************************************************************************************** */
Monitor::Monitor(Application &app, QWidget *parent)
  : QWidget(parent), _application(app), _devices(), _monitor(0)
{
  _deviceList = new QComboBox();
  _updateDeviceList();

  QPushButton *update = new QPushButton(tr("update"));

  _monitor = new MonitorView(_devices.at(_deviceList->currentIndex()), _application);

  QHBoxLayout *bbox = new QHBoxLayout();
  bbox->addWidget(_deviceList, 1);
  bbox->addWidget(update, 0);

  QVBoxLayout *layout = new QVBoxLayout();
  layout->addLayout(bbox);
  layout->addWidget(_monitor);

  setLayout(layout);

  connect(_deviceList, SIGNAL(currentIndexChanged(int)), this, SLOT(_deviceSelected(int)));
  connect(update, SIGNAL(clicked(bool)), this, SLOT(_updateDeviceList()));
}

void
Monitor::_updateDeviceList() {
  disconnect(_deviceList, SIGNAL(currentIndexChanged(int)), this, SLOT(_deviceSelected(int)));
  _deviceList->clear();
  _devices = QAudioDeviceInfo::availableDevices(QAudio::AudioInput);
  // Add null-device
  _devices.prepend(QAudioDeviceInfo());
  QAudioDeviceInfo defaultDevice = _application.station().inputDevice();
  QList<QAudioDeviceInfo>::iterator device = _devices.begin();
  for (size_t i=0; device != _devices.end(); device++, i++) {
    if (device->isNull())
      _deviceList->addItem("none", "");
    else
      _deviceList->addItem(device->deviceName(), device->deviceName());
    if (*device == defaultDevice) {
      _deviceList->setCurrentIndex(i);
    }
  }
  connect(_deviceList, SIGNAL(currentIndexChanged(int)), this, SLOT(_deviceSelected(int)));
}

void
Monitor::_deviceSelected(int idx) {
  _monitor->setDevice(_devices.at(idx));
  _application.station().setInputDevice(_devices.at(idx));
}
