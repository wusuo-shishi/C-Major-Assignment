// =============================================================================
// main.cpp — 应用程序入口
//
// 流程：静默libpng警告 → 创建QApplication → 创建MainWindow(1600×900) → 显示 → 事件循环
// =============================================================================

#include "MainWindow.h"

#include <QApplication>
#include <cstdio>

int main(int argc, char *argv[])
{
    // 静默libpng iCCP警告（stderr重定向到nul，不影响Qt日志输出）
    freopen("nul", "w", stderr);

    QApplication a(argc, argv);
    MainWindow w;
    w.show();
    return a.exec();
}
