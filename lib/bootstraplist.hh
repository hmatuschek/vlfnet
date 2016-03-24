#ifndef BOOSTRAPLIST_HH
#define BOOSTRAPLIST_HH

#include <QList>
#include <QString>
#include <QPair>

/** A simple list of boostrap nodes specified as JSON.
 * This "class" just provides two static methods to read from and write the boostrap list to
 * a file. */
class BootstrapList
{
public:
  /** Reads the boostrap list from the given @c filename. On error it returns an empty list. */
  static QList< QPair<QString, uint16_t> > fromFile(const QString &filename);
  /** First reads the boostrap list form the given file, adds the specified host and writes it back
   * to the file. */
  static void add(const QString &filename, const QString &host, uint16_t port);
};

#endif // BOOSTRAPLIST_HH
