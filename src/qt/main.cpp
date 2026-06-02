#include "passman.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    PassMan w;
    w.startup();
    w.initialise();
#if defined(Q_OS_IOS)
    w.showMaximized();
#else
    w.show();
#endif
    return a.exec();
}
