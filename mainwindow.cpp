#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "db_manager.h"
#include <QGroupBox>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>
#include <QDateEdit>
#include <QTableWidget>
#include <QTcpSocket>           /* ★Day163:TCP 实时通道 */
#include <QJsonDocument>
#include <QJsonObject>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    this->resize(900, 520);
    this->setWindowTitle("车载监控 v0.9.1·TCP实时");

    tabs = new QTabWidget;
    tabs->addTab(createRealtimePage(), "实时监控");
    tabs->addTab(createHistoryPage(), "历史回放");
    this->setCentralWidget(tabs);

    if (!DbManager::instance().open("/home/wu/sync/vehicle.db")) {
        qDebug() << "[DB] 同步库没就绪——先跑 ~/start_work.sh";
    }

    initChart();

    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, [=](){
        refreshRealtime();
        checkAlert();
    });
    refreshTimer->start(1000);

    connectToBoard();     /* ★Day163:TCP 实时通道上线 */
}

// ── 实时页(原样)──
QWidget* MainWindow::createRealtimePage()
{
    lblLat   = new QLabel("纬度: --");
    lblLon   = new QLabel("经度: --");
    lblSat   = new QLabel("卫星: --");
    lblSpeed = new QLabel("速度: --");
    lblTemp  = new QLabel("温度: --");
    lblHumi  = new QLabel("湿度: --");
    lblPitch = new QLabel("俯仰: --");
    lblRoll  = new QLabel("横滚: --");
    lblDist  = new QLabel("距离: --");
    lblAlert = new QLabel("告警: 无");

    lblSpeed->setStyleSheet("font-size: 24px; color: #00FF00; font-weight: bold;");
    lblLat->setStyleSheet("font-size: 16px;");
    lblLon->setStyleSheet("font-size: 16px;");
    lblSat->setStyleSheet("font-size: 16px;");
    lblTemp->setStyleSheet("font-size: 16px;");
    lblHumi->setStyleSheet("font-size: 16px;");
    lblPitch->setStyleSheet("font-size: 16px;");
    lblRoll->setStyleSheet("font-size: 16px;");
    lblDist->setStyleSheet("font-size: 16px;");
    lblAlert->setStyleSheet("font-size: 16px; color: #4fc3f7;");

    QGroupBox *gpsGroup = new QGroupBox("GPS数据");
    QVBoxLayout *gpsLayout = new QVBoxLayout;
    gpsLayout->addWidget(lblLat);
    gpsLayout->addWidget(lblLon);
    gpsLayout->addWidget(lblSat);
    gpsGroup->setLayout(gpsLayout);

    QGroupBox *envGroup = new QGroupBox("环境数据");
    QVBoxLayout *envLayout = new QVBoxLayout;
    envLayout->addWidget(lblSpeed);
    envLayout->addWidget(lblTemp);
    envLayout->addWidget(lblHumi);
    envLayout->addWidget(lblPitch);
    envLayout->addWidget(lblRoll);
    envLayout->addWidget(lblDist);
    envLayout->addWidget(lblAlert);
    envGroup->setLayout(envLayout);

    mainLayout = new QHBoxLayout;
    mainLayout->addWidget(gpsGroup);
    mainLayout->addWidget(envGroup);

    QWidget *page = new QWidget;
    page->setLayout(mainLayout);
    return page;
}

// ── 历史页(原样)──
QWidget* MainWindow::createHistoryPage()
{
    QWidget *page = new QWidget;
    QVBoxLayout *layout = new QVBoxLayout(page);

    // ── 条件栏 ──
    QHBoxLayout *condLayout = new QHBoxLayout;
    QDateEdit *dateEdit = new QDateEdit(QDate::currentDate());
    dateEdit->setCalendarPopup(true);
    dateEdit->setDisplayFormat("yyyy-MM-dd");
    QPushButton *btnQuery = new QPushButton("查询");
    condLayout->addWidget(new QLabel("选择日期:"));
    condLayout->addWidget(dateEdit);
    condLayout->addWidget(btnQuery);
    condLayout->addStretch();

    // ── 历史曲线:官方推荐写法(addAxis + attachAxis,不用废弃的 setAxisX)──
    QLineSeries *histSeries = new QLineSeries();
    histSeries->setName("温度 °C");

    QChart *histChart = new QChart();
    histChart->addSeries(histSeries);              // ① 先加序列

    QDateTimeAxis *xAxis = new QDateTimeAxis;      // ② 建轴
    xAxis->setFormat("HH:mm");
    QValueAxis *yAxis = new QValueAxis();
    yAxis->setRange(0, 50);

    histChart->addAxis(xAxis, Qt::AlignBottom);    // ③ 轴上画板(底/左)
    histChart->addAxis(yAxis, Qt::AlignLeft);
    histSeries->attachAxis(xAxis);                 // ④ 序列挂轴(点才对得上坐标)
    histSeries->attachAxis(yAxis);

    histChart->setTitle("当天温度曲线(历史,最近5000条)");
    histChart->legend()->hide();

    QChartView *histView = new QChartView(histChart);
    histView->setRenderHint(QPainter::Antialiasing);
    histView->setMinimumHeight(200);

    // ── 结果表格:12 列=真库契约 ──
    QTableWidget *table = new QTableWidget;
    table->setColumnCount(12);
    table->setHorizontalHeaderLabels(
        {"时间","纬度","经度","速度","加速度","温度",
         "湿度","俯仰","横滚","距离","卫星","告警"});

    layout->addLayout(condLayout);
    layout->addWidget(histView);
    layout->addWidget(table);

    // ── 查询逻辑 ──
    connect(btnQuery, &QPushButton::clicked, this, [=](){
        DbManager::instance().ensureOpen();

        QString date = dateEdit->date().toString("yyyy-MM-dd");
        QSqlQuery query;
        query.prepare("SELECT timestamp, latitude, longitude, speed,"
                      " accel_total, temperature, humidity, pitch, roll,"
                      " distance, satellites, alert"
                      " FROM (SELECT * FROM data_point WHERE date(timestamp)=?"
                      "       ORDER BY timestamp DESC LIMIT 5000)"
                      " ORDER BY timestamp ASC");
        query.addBindValue(date);
        query.exec();

        table->setRowCount(0);
        histSeries->clear();

        int row = 0;
        QDateTime tFirst, tLast;
        while (query.next()) {
            table->insertRow(row);
            for (int col = 0; col < 12; col++)
                table->setItem(row, col,
                               new QTableWidgetItem(query.value(col).toString()));

            QDateTime t = QDateTime::fromString(
                query.value(0).toString(), "yyyy-MM-dd hh:mm:ss");
            if (row == 0) {
                tFirst = t;
                if (!t.isValid())     // 时间解析失败=格式对不上,打样本自证
                    qDebug() << "[历史] 时间解析失败,样本:"
                             << query.value(0).toString();
            }
            tLast = t;
            histSeries->append(t.toMSecsSinceEpoch(),
                               query.value(5).toDouble());
            row++;
        }

        if (row > 0 && tFirst.isValid() && tLast.isValid()) {
            xAxis->setRange(tFirst, tLast);        // ★轴范围=数据实际范围
            qDebug() << "[历史] 画了" << histSeries->count() << "个点,范围"
                     << tFirst.toString("HH:mm") << "~"
                     << tLast.toString("HH:mm");
        } else {
            qDebug() << "[历史] 该日期无数据(row=" << row << ")";
        }

        table->resizeColumnsToContents();
        statusBar()->showMessage(QString("查询到 %1 条记录").arg(row));
    });

    return page;
}

// ── 实时页曲线(原样)──
void MainWindow::initChart()
{
    tempSeries = new QLineSeries();
    tempSeries->setName("温度 °C");

    tempChart = new QChart();
    tempChart->addSeries(tempSeries);
    tempChart->createDefaultAxes();
    tempChart->axisY()->setRange(0, 50);
    tempChart->setTitle("实时温度(真数据)");
    tempChart->legend()->hide();

    QChartView *chartView = new QChartView(tempChart);
    chartView->setRenderHint(QPainter::Antialiasing);

    mainLayout->addWidget(chartView);
}

// ── 每秒刷新(★Day163 加两行:TCP 在喂时让位)──
void MainWindow::refreshRealtime()
{
    DbManager::instance().ensureOpen();   /* 照常做:文件被 mv 换过要重开,
                                             checkAlert() 的告警查询也靠这条连接 */

    if (tcpConnected)
        return;   /* TCP 在喂(秒级新鲜),库副本旧约 1 分钟,标签/曲线不打架;
                     告警在 checkAlert() 里不受影响;TCP 断线自动回退到下面走库 */

    double lat, lon, speed, temp, humi, pitch, roll, dist;
    int sat, alert;
    if (!DbManager::instance().queryLatest(lat, lon, sat, speed,
                                           temp, humi, pitch, roll,
                                           dist, alert))
        return;

    lblLat->setText(QString("纬度: %1").arg(lat, 0, 'f', 6));
    lblLon->setText(QString("经度: %1").arg(lon, 0, 'f', 6));
    lblSat->setText(QString("卫星: %1").arg(sat));
    lblSpeed->setText(QString("速度: %1 km/h").arg(speed, 0, 'f', 0));
    lblTemp->setText(QString("温度: %1 °C").arg(temp, 0, 'f', 1));
    lblHumi->setText(QString("湿度: %1 %").arg(humi, 0, 'f', 1));
    lblPitch->setText(QString("俯仰: %1°").arg(pitch, 0, 'f', 1));
    lblRoll->setText(QString("横滚: %1°").arg(roll, 0, 'f', 1));
    lblDist->setText(QString("距离: %1 mm").arg(dist, 0, 'f', 0));

    tempSeries->append(x_counter++, temp);
    if (tempSeries->count() > 60)
        tempSeries->remove(0);
    tempChart->axisX()->setRange(qMax(0, x_counter - 59), x_counter);
}

// ── 告警检查(原样)──
void MainWindow::checkAlert()
{
    static int last_alert_id = 0;
    static bool first = true;

    QSqlQuery query;
    query.exec("SELECT id, alert_type, detail FROM alert_log ORDER BY id DESC LIMIT 1;");
    if (query.next()) {
        int id = query.value(0).toInt();

        if (first) {
            last_alert_id = id;
            first = false;
            qDebug() << "[ALERT] 消费位置初始化到 id" << id << "(历史告警不弹)";
            return;
        }

        if (id != last_alert_id) {
            last_alert_id = id;
            QString type   = query.value(1).toString();
            QString detail = query.value(2).toString();

            lblAlert->setText("[!] 告警: " + type + " " + detail);
            lblAlert->setStyleSheet("color: red; font-size: 16px; font-weight: bold;");

            QMessageBox::warning(this, "车辆告警",
                                 QString("类型: %1\n详情: %2").arg(type, detail),
                                 QMessageBox::Ok);
        }
    }
}

// ── ★Day163 v0.9.1:TCP 实时通道(重连补洞版)──
void MainWindow::connectToBoard()
{
    /* 防双触发闸:disconnected 和 errorOccurred 可能先后都来,
       闸门保证 3 秒重试只排一次(DbManager 单例同款:函数内 static 程序级唯一) */
    static bool retryScheduled = false;
    retryScheduled = false;

    tcpSocket = new QTcpSocket(this);

    /* 数据到了(QIODevice 的信号,和 QTimer::timeout 同族:事件驱动,不轮询) */
    connect(tcpSocket, &QTcpSocket::readyRead, this, &MainWindow::onTcpData);

    connect(tcpSocket, &QTcpSocket::connected, this, [=](){
        tcpConnected = true;
        statusBar()->showMessage("TCP 实时通道已连接(每秒刷新)");
    });

    /* 路1:连上之后断开(板子程序死了/拔网线) */
    connect(tcpSocket, &QTcpSocket::disconnected, this, [=](){
        tcpConnected = false;
        if (!retryScheduled) {
            retryScheduled = true;
            statusBar()->showMessage("板子断开,3秒后重连...(期间回退数据库刷新)");
            QTimer::singleShot(3000, this, &MainWindow::connectToBoard);
        }
    });

    /* 路2(v0.9.1 补洞):连接被拒不发 disconnected,只发 errorOccurred——
       板子起得比 3 秒慢时,靠这条路把重试链续上 */
    connect(tcpSocket, &QAbstractSocket::errorOccurred, this,
            [=](QAbstractSocket::SocketError){
        if (tcpConnected)
            return;                    /* 已连接期间的错误走上面的 disconnected 路 */
        if (!retryScheduled) {
            retryScheduled = true;
            statusBar()->showMessage("板子没连上,3秒后重试...(期间回退数据库刷新)");
            QTimer::singleShot(3000, this, &MainWindow::connectToBoard);
        }
    });

    tcpSocket->connectToHost("192.168.1.100", 9000);
}

void MainWindow::onTcpData()
{
    /* canReadLine:凑齐一整行(\n 结尾)才读——JSON 按行切,半行不解析 */
    while (tcpSocket->canReadLine()) {
        QByteArray line = tcpSocket->readLine();
        QJsonDocument doc = QJsonDocument::fromJson(line);
        if (!doc.isObject())
            continue;                      /* 坏行丢弃,下一秒会有新行 */

        QJsonObject o = doc.object();
        double lat   = o["lat"].toDouble();
        double lon   = o["lon"].toDouble();
        double spd   = o["spd"].toDouble();
        double temp  = o["temp"].toDouble();
        double humi  = o["humi"].toDouble();
        double pitch = o["P"].toDouble();
        double roll  = o["R"].toDouble();
        double dist  = o["dist"].toDouble();
        int    sat   = o["sat"].toInt();

        lblLat->setText(QString("纬度: %1").arg(lat, 0, 'f', 6));
        lblLon->setText(QString("经度: %1").arg(lon, 0, 'f', 6));
        lblSat->setText(QString("卫星: %1").arg(sat));
        lblSpeed->setText(QString("速度: %1 km/h").arg(spd, 0, 'f', 0));
        lblTemp->setText(QString("温度: %1 °C").arg(temp, 0, 'f', 1));
        lblHumi->setText(QString("湿度: %1 %").arg(humi, 0, 'f', 1));
        lblPitch->setText(QString("俯仰: %1°").arg(pitch, 0, 'f', 1));
        lblRoll->setText(QString("横滚: %1°").arg(roll, 0, 'f', 1));
        lblDist->setText(QString("距离: %1 mm").arg(dist, 0, 'f', 0));

        /* 曲线吃 TCP(和走库时同一套滑动窗口,断线回退无缝衔接) */
        tempSeries->append(x_counter++, temp);
        if (tempSeries->count() > 60)
            tempSeries->remove(0);
        tempChart->axisX()->setRange(qMax(0, x_counter - 59), x_counter);
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}
