#include "application.hh"
#include "mainwindow.hh"
#include <ovlnet/logger.hh>


int main(int argc, char *argv[])
{
  Logger::addHandler(new IOLogHandler());

  Application app(argc, argv);

  MainWindow win(app);
  win.show();

  app.exec();

  return 0;
}
