# VehicleMonitor — 车载行车数据 Qt 监控界面(09_qt_interface)

毕设《基于嵌入式Linux的车载行车数据记录仪》界面层(200天计划 Day141-160)。

## 是什么
在 Ubuntu 虚拟机里运行的 Qt5 监控台,读取 ALPHA 板上同步过来的 SQLite 库:
- **实时监控页**:9 项数据标签每秒刷新 + 温度实时曲线(滑动窗口 60 点)+ 告警弹窗(去重/冷启动)
- **历史回放页**:按日期查询,QTableWidget 12 列表格 + 时间轴温度曲线一次查询双呈现(LIMIT 5000)

## 数据通路
STM32(CAN 500k)→ I.MX6ULL sensor_framework(六线程落库 /home/root/data/vehicle.db)
→ VM 同步脚本(ssh 触发板上 sqlite3 .backup 快照 → scp -O 拉取 → mv 原子替换,每分钟)
→ Qt QTimer 每秒查询本地副本(mtime 变化自动 reopen)

## 环境
Ubuntu 24.04.5 / Qt 5.15.13 / qmake / 构建:Qt Creator 桌面 kit

## 验收
141-160 组9 八项必过全过(2026-10-24):工程健康/信号槽/实时显示/曲线/三处一致/告警/历史查询/优雅降级。
