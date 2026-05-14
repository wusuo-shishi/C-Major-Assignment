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
