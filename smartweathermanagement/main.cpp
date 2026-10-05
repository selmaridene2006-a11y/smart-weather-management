#include "smartweather.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    smartweather w;
    w.show();
    return QApplication::exec();
}