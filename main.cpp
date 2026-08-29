#include <QApplication>

#include "database.h"
#include "loginwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    if (!Database::instance().initialize())
    {
        return -1;
    }

    LoginWindow loginWindow;

    loginWindow.show();

    return app.exec();
}