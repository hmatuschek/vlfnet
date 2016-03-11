#ifndef MONITOR_HH
#define MONITOR_HH

#include <QWidget>
#include <fftw3.h>

class Application;
class Audio;

/** Interface for all color maps. */
class ColorMap
{
protected:
  /** Hidden constructor. */
  ColorMap(double min, double max);

public:
  /** Destructor. */
  virtual ~ColorMap();

  /** Maps a value to a color. */
  inline QColor operator()(const double &value) {
    if (value > _max) { return this->map(1); }
    if (value < _min) { return this->map(0); }
    return this->map((value-_min)/(_max-_min));
  }

  /** Maps a value on the interval [0,1] to a color.
   * Needs to be implemented by all sub-classes. */
  virtual QColor map(const double &value) = 0;

protected:
  /** Minimum value. */
  double _min;
  /** Maximum value. */
  double _max;
};

/** A simple gray-scale color map. */
class GrayScaleColorMap: public ColorMap
{
public:
  /** Constructor.
   * @param mindB Specifices the minimum value in dB of the color-scale. Mapping values [min, max]
   * to a gray-scale. */
  GrayScaleColorMap(double min, double max);
  /** Destructor. */
  virtual ~GrayScaleColorMap();
  /** Implements the color mapping. */
  virtual QColor map(const double &value);
};

/** A linear interpolating color map. */
class LinearColorMap: public ColorMap {
public:
  LinearColorMap(const QVector<QColor> &colors, double min, double max);
  /** Destructor. */
  virtual ~LinearColorMap();
  virtual QColor map(const double &value);

protected:
  QVector<QColor> _colors;
};


class Monitor : public QWidget
{
  Q_OBJECT

public:
  explicit Monitor(Application &app, QWidget *parent = 0);
  virtual ~Monitor();

protected slots:
  void processStream(const int16_t *data, size_t len);

protected:
  void updatePSD();
  void updatePlot();
  void paintEvent(QPaintEvent *evt);
  void drawKnownStations(QPainter &painter);

protected:
  Application &_application;
  Audio *_input;

  double *_fftInBuffer;
  size_t _nFFTBuffer;
  double *_fftwOutBuffer;

  fftw_plan _fft;

  double *_psd;
  size_t _psdSumCount;

  LinearColorMap _colormap;
  QPixmap _plot;
};

#endif // MONITOR_HH
