#include <QApplication>
#include "loginwindow.h"
#include "common/AppStyle.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setStyleSheet(AppStyle::baseWindowStyle());

    LoginWindow loginWindow;
    loginWindow.show();

    return app.exec();
}