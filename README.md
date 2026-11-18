# VehicleMonitor · 车载行车数据 Qt 监控界面

> 毕业设计《基于嵌入式Linux的车载行车数据记录仪》的**界面层**。
> 在 Ubuntu 上运行的 Qt5 监控台，读取 ALPHA 板同步过来的 SQLite 库，提供实时监控、告警弹窗与历史回放。

[![Platform](https://img.shields.io/badge/platform-Ubuntu%2024.04-blue)](#)
[![Framework](https://img.shields.io/badge/framework-Qt%205.15-green)](#)
[![Language](https://img.shields.io/badge/language-C%2B%2B-orange)](#)

## 概览

本仓库实现监控端的两大页面：

- **实时监控页**：9 项数据标签每秒刷新 + QChart 温度实时曲线（60 点滑动窗口）+ 告警弹窗（序号去重 / 冷启动对齐 / 非模态）
- **历史回放页**：按日期参数化查询，QTableWidget 12 列表格 + QDateTimeAxis 时间轴曲线，一次查询双呈现（LIMIT 5000）

此外还有 TCP 实时通道：告警事件经 TCP 橱窗推送，界面按 `ev_seq` 去重，端到端告警延迟从 35 秒优化到 2 秒。

## 特性

- **信号槽驱动**：QTimer 每秒刷新，耗时操作不进 UI 线程
- **双页面架构**：`createRealtimePage()` 返回 QWidget，用 `addTab` 拼装，页面解耦
- **数据库安全**：`prepare / bindValue` 参数化查询防 SQL 注入；`DbManager` 单例管理连接
- **文件句柄自愈**：同步脚本每分钟原子替换数据库文件，`ensureOpen()` 检测 mtime 变化自动 reopen，避免攥着旧句柄显示旧数据
- **优雅降级**：数据库未就绪时界面保持 `--` 不崩溃
- **曲线自动跟随**：`axisX()->setRange()` 每秒滑动，解决 Qt Charts 轴不自动跟随导致曲线"消失"的坑

## 数据通路

```
STM32 (CAN 500k)
   → I.MX6ULL sensor_framework（落库 /home/root/data/vehicle.db）
   → VM 同步脚本（ssh 触发板上 sqlite3 .backup → scp -O 拉取 → mv 原子替换，每分钟）
   → Qt QTimer 每秒查询本地副本（mtime 变化自动 reopen）
   → 界面：实时曲线 / 历史回放 / 告警弹窗

另：TCP 实时通道（:9000）推送最新值与事件，告警 35s → 2s
```

## 环境

- Ubuntu 24.04.5 / Qt 5.15.13（qmake）
- 依赖模块：`charts`（实时曲线）、`sql`（数据库）、`network`（TCP）
- 构建：Qt Creator 桌面 kit，或 `qmake && make`

## 目录结构

```
├── mainwindow.cpp/.h   # 主窗口：页面拼装、刷新逻辑、TCP 接收、告警去重
├── mainwindow.ui       # UI 设计文件
├── db_manager.h        # DbManager 单例（QSqlDatabase 封装）
├── main.cpp            # 程序入口
└── VehicleMonitor.pro  # qmake 工程（功能开关面板）
```

## 验收

141-160 组9 八项必过全过（2026-10-24）：工程健康 / 信号槽 / 实时显示 / 曲线 / 三处一致 / 告警 / 历史查询 / 优雅降级。

## 相关仓库

- 数据来源（上位机框架）：[08_linux_sensor_framework](https://github.com/WQZ107/08_linux_sensor_framework)
- 实时通道（TCP 服务端）：[10_net_gateway](https://github.com/WQZ107/10_net_gateway)
