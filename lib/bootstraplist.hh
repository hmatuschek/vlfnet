#ifndef BOOSTRAPLIST_HH
#define BOOSTRAPLIST_HH

#include <QList>
#include <QString>
#include <QPair>

class BootstrapList
{
public:
  static QList< QPair<QString, uint16_t> > fromFile(const QString &filename);
  static void add(const QString &filename, const QString &host, uint16_t port);
};

#endif // BOOSTRAPLIST_HH
