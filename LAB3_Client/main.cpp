#include "widget.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setApplicationDisplayName("Client");
    Widget *w = new Widget();
    w->show();

    return a.exec();
}
