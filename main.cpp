// main.cpp
// This is the entry point of our application. It sets up the Qt application object
// and creates our main window.
#include <QApplication>
#include "MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    app.setWindowIcon(QIcon(":/app_icon.png"));
    
    MainWindow window;
    window.show();
    return app.exec();
}