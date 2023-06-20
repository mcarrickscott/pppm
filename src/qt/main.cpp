#include "passman.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PassMan w;
    w.initialise();
    w.show();
    return a.exec();
}
