#include "maindialog.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QApplication::setOrganizationName("Cyril Margorin");
    QApplication::setOrganizationDomain("tower.pp.ru");
    QApplication::setApplicationName("jpeg-archiver");
    QApplication::setApplicationDisplayName(QApplication::tr("JPEG Archiver"));
    MainDialog w;
    w.show();

    return a.exec();
}
