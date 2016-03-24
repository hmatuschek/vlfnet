#ifndef SOCKSSERVICE_HH
#define SOCKSSERVICE_HH

#include <ovlnet/socks.hh>

class Station;

class SocksService: public AbstractService
{
public:
  SocksService(const QString &whitelist, Station &station);
  SecureSocket *newSocket();
  bool allowConnection(const NodeItem &peer);
  void connectionStarted(SecureSocket *stream);
  void connectionFailed(SecureSocket *stream);

protected:
  Station &_station;
  QSet<Identifier> _whitelist;
};

#endif // SOCKSSERVICE_HH
