#include <QApplication>
#include "gui/MainWindow.h"
#include "gui/Theme.h"

int main(int argc, char** argv) {
    QApplication app(argc, argv);
    applyPurpleTheme(app);

    MainWindow w;
    w.resize(1360, 860);
    w.show();
    return app.exec();
}
