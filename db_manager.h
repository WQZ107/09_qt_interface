#ifndef DB_MANAGER_H
#define DB_MANAGER_H

#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>
#include <QFileInfo>
#include <QDateTime>

class DbManager
{
public:
    static DbManager& instance() {
        static DbManager inst;
        return inst;
    }

    bool open(const QString &path) {
        m_path = path;
        m_db = QSqlDatabase::addDatabase("QSQLITE");
        m_db.setDatabaseName(path);
        if (!m_db.open()) {
            qWarning() << "[DB] 数据库打开失败:" << m_db.lastError().text();
            return false;
        }
        m_lastMtime = QFileInfo(path).lastModified();
        qDebug() << "[DB] 数据库打开成功:" << path;
        return true;
    }

    // 组5 新增:同步脚本是 mv"换文件"(不是改文件)——Qt 攥着旧文件句柄,
    // 不重开的话数字永远停在启动那一刻。发现 mtime 变了就重新 open(拿新句柄)。
    void ensureOpen() {
        QDateTime mtime = QFileInfo(m_path).lastModified();
        if (mtime != m_lastMtime) {
            m_db.close();
            m_db.open();
            m_lastMtime = mtime;
            qDebug() << "[DB] 检测到库更新,已重新打开";
        }
    }

    bool queryLatest(double &lat, double &lon, int &sat,
                     double &speed, double &temp, double &humi,
                     double &pitch, double &roll, double &dist,
                     int &alert)
    {
        QSqlQuery query;
        query.exec("SELECT latitude, longitude, satellites, speed,"
                   " temperature, humidity, pitch, roll, distance, alert"
                   " FROM data_point ORDER BY id DESC LIMIT 1;");
        if (query.next()) {
            lat   = query.value(0).toDouble();   // 0 = SELECT 第1列
            lon   = query.value(1).toDouble();
            sat   = query.value(2).toInt();
            speed = query.value(3).toDouble();
            temp  = query.value(4).toDouble();
            humi  = query.value(5).toDouble();
            pitch = query.value(6).toDouble();
            roll  = query.value(7).toDouble();
            dist  = query.value(8).toDouble();
            alert = query.value(9).toInt();
            return true;
        }
        return false;               // 表空:调用方保持原显示,不崩
    }

private:
    DbManager() {}
    QSqlDatabase m_db;
    QString m_path;                 // 组5:记住路径,ensureOpen 用
    QDateTime m_lastMtime;          // 组5:上次打开时的文件修改时间
};
#endif // DB_MANAGER_H
