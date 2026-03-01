#include <QApplication>
#include "app/mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    app.setApplicationName("SubtitleForge");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("SubtitleForge");

    MainWindow window;
    window.resize(1400, 900);
    window.show();

    return app.exec();
}
