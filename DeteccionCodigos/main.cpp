#include "DeteccionCodigos.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    DeteccionCodigos w;
    w.show();
    return a.exec();
}
