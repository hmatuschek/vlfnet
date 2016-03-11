#include "bootstraplist.hh"
#include <QFile>
#include <QJsonParseError>
#include <QJsonDocument>
#include <QJsonArray>
#include <QJsonObject>
#include <ovlnet/logger.hh>

QList< QPair<QString, uint16_t> >
BootstrapList::fromFile(const QString &filename) {
  QList< QPair<QString, uint16_t> > res;
  QFile file(filename);
  if (! file.open(QIODevice::ReadOnly)) {
    logError() << "Cannot open bootstrap node list at " << filename << ".";
    return res;
  }

  QJsonParseError err;
  QJsonDocument doc = QJsonDocument::fromJson(file.readAll(), &err);
  file.close();

  if ((QJsonParseError::NoError != err.error) || (! doc.isArray())) {
    logError() << "Cannot read bootstrap node list at " << filename
               << ": " << err.errorString() << ".";
    return res;
  }

  QJsonArray lst = doc.array();
  QJsonArray::iterator item = lst.begin();
  for (; item != lst.end(); item++) {
    if (! item->isObject()) {
      continue;
    }
    QJsonObject obj = item->toObject();
    if ((! obj.contains("host")) || (! obj.contains("port"))) {
      continue;
    }
    res.append(QPair<QString, uint16_t>(obj.value("host").toString(), obj.value("port").toInt()));
  }

  return res;
}

void
BootstrapList::add(const QString &filename, const QString &host, uint16_t port) {
  QList< QPair<QString, uint16_t> > lst = fromFile(filename);
  lst.append(QPair<QString, uint16_t>(host, port));

  QJsonArray res;
  QList< QPair<QString, uint16_t> >::iterator item = lst.begin();
  for (; item != lst.end(); item++) {
    QJsonObject obj;
    obj.insert("host", item->first);
    obj.insert("port", item->second);
    res.append(obj);
  }

  QFile file(filename);
  if (! file.open(QIODevice::WriteOnly)) {
    logError() << "Cannot save bootstrap node list into " << filename;
    return;
  }

  file.write(QJsonDocument(res).toJson());
  file.flush();
  file.close();
}
