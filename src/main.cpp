#include <QApplication>
#include "app/mainwindow.h"

int main(int argc, char *argv[])
{
    // Optional: set QT_MEDIA_BACKEND=windows if default has no audio on your system
    QApplication app(argc, argv);
    app.setApplicationName("Reels Forge");
    app.setApplicationVersion("1.0.0");
    app.setOrganizationName("ReelsForge");

    MainWindow window;
    window.resize(1400, 900);
    window.show();

    return app.exec();
}
