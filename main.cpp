#include "mainwindow.h"
#include "event.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qRegisterMetaType<Event>("Event");
    MainWindow w;
    w.show();
    return a.exec();
}
