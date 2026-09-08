#include "client.h"

#include <QApplication>

int main(int argc, char *argv[])
{
  QApplication a(argc, argv);
  a.setStyle("Fusion");
  Client w;
  w.show();
  return a.exec();
}
