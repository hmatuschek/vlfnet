#include "application.hh"
#include <ovlnet/logger.hh>

int main(int argc, char *argv[]) {
  Logger::addHandler(new IOLogHandler());

  Application app(argc, argv);

  app.exec();

  return 0;
}
