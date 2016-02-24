#ifndef STATION_HH
#define STATION_HH

#include <ovlnet/crypto.hh>
#include <QApplication>

class Node;
class LogModel;
class StationList;


class Station : public QApplication
{
  Q_OBJECT

public:
  explicit Station(int &argc, char **argv);

  /** Returns the OvlNet node of the station.*/
  Node &node();

  /** Returns a weak reference to the log-message table model. */
  LogModel &log();

  /** Returns a list of known stations. */
  StationList &stations();

protected:
  /** The identity of the node within the ovlnet. */
  Identity *_identity;
  /** Overlay network node representing this station. */
  Node *_node;

  /** A model to capture log messages for display. */
  LogModel *_logmodel;
  /** A list of known stations. */
  StationList *_stations;
};

#endif // STATION_HH
