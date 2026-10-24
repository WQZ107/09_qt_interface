#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QLabel>
#include <QHBoxLayout>
#include <QTimer>
#include <QPainter>
#include <QTabWidget>
#include <QtCharts>
using namespace QtCharts;

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void initChart();
    void refreshRealtime();
    void checkAlert();
    QWidget *createRealtimePage();   // ★组7:组2~6 的面板装成"页面积木"
    QWidget *createHistoryPage();    // ★组7:历史页(今晚占位,明晚建成)

private:
    Ui::MainWindow *ui;

    // ── 标签们 + 实时页布局 ──
    QLabel *lblLat;     // 纬度
    QLabel *lblLon;     // 经度
    QLabel *lblSat;     // 卫星
    QLabel *lblSpeed;   // 速度
    QLabel *lblTemp;    // 温度
    QLabel *lblHumi;    // 湿度
    QLabel *lblPitch;   // 俯仰
    QLabel *lblRoll;    // 横滚
    QLabel *lblDist;    // 距离
    QLabel *lblAlert;   // 告警状态
    QHBoxLayout *mainLayout;   // 实时页主布局
    QTabWidget *tabs;           // ★组7:主窗口两个标签页

    QTimer *refreshTimer;
    QLineSeries *tempSeries;
    QChart *tempChart;
    int x_counter = 0;
};
#endif // MAINWINDOW_H
