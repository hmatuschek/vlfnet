#ifndef SETTINGS_HH
#define SETTINGS_HH

#include <QString>
#include <QObject>

class Settings: public QObject
{
  Q_OBJECT

public:
  Settings(const QString &path, QObject *parent=0);

  uint16_t port() const;
  void setPort(uint16_t port);

  bool hasDDNSUpdateUrl() const;
  const QString &ddnsUpdateUrl() const;
  void setDDNSUpdateURL(const QString &url);

protected:
  void _save() const;

protected:
  QString _path;
  uint16_t _port;
  QString _ddnsUpdateUrl;
};

#endif // SETTINGS_HH
