#include <QApplication>
#include "PetController.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setQuitOnLastWindowClosed(false);

    QApplication::setApplicationName("DesktopPet");
    QApplication::setOrganizationName("DesktopPet");

    PetController controller;
    controller.init();

    return app.exec();
}
