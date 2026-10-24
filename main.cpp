#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 全局暗色主题(Qt 版 CSS,整车载风格,直接抄)
    a.setStyleSheet(
        "QMainWindow { background-color: #1a1a2e; }"
        "QLabel { color: #e0e0e0; }"
        "QGroupBox { color: #4fc3f7; border: 1px solid #4fc3f7;"
        "            border-radius: 8px; margin-top: 10px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 15px; }"
        "QPushButton { background-color: #0f3460; color: white;"
        "             border-radius: 5px; padding: 8px 16px; font-size: 14px; }"
        "QPushButton:hover { background-color: #16213e; }"
        "QPushButton:pressed { background-color: #4fc3f7; }"
        );

    MainWindow w;
    w.show();
    return a.exec();
}
