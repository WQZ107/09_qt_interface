#include "mainwindow.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // 全局暗色主题(Qt 版 CSS,整车载风格,直接抄)
    a.setStyleSheet(
        "QMainWindow { background-color: #1a1a2e; }"
        "QLabel { color: #1a1a1a; }"
        "QGroupBox { color: #4fc3f7; border: 1px solid #4fc3f7;"
        "            border-radius: 8px; margin-top: 10px; padding-top: 10px; }"
        "QGroupBox::title { subcontrol-origin: margin; left: 15px; }"
        "QPushButton { background-color: #0f3460; color: white;"
        "             border-radius: 5px; padding: 8px 16px; font-size: 14px; }"
        "QPushButton:hover { background-color: #16213e; }"
        "QPushButton:pressed { background-color: #4fc3f7; }"
        // Day163:状态栏定妆(默认继承深底灰字看不清 → 亮青字加粗)
        "QStatusBar { background-color: #0d1b2a; color: #00E5FF;"
        "            font-size: 13px; font-weight: bold;"
        "            border-top: 1px solid #4fc3f7; }"
        );

    MainWindow w;
    w.show();
    return a.exec();
}
