#ifndef MAINWINDOW_H
#define MAINWINDOW_H

// =============================================================================
// MainWindow.h — 应用主窗口与页面导航
// 使用QStackedWidget管理4个页面切换：
//   index 0: StartWidget   — 开始界面
//   index 1: SelectWidget  — 角色选择界面
//   index 2: GameWidget    — 游戏核心界面
//   index 3: ResultWidget  — 结算界面
// =============================================================================

#include <QMainWindow>
#include <QStackedWidget>
#include <QPushButton>
#include <QPixmap>
#include <QIcon>
#include "StartWidget.h"
#include "SelectWidget.h"
#include "GameWidget.h"
#include "ResultWidget.h"

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    SelectWidget *selectPage;  // 角色选择界面
    StartWidget *startPage;    // 开始界面
    GameWidget *gamePage;      // 游戏核心界面
    ResultWidget *resultPage;  // 结算界面

    // ---- 静音按钮（圆形覆盖按钮，位于所有界面上层）---------------------------
    QPushButton *m_muteButton;     // 静音按钮控件
    bool m_muted = false;           // 静音状态：false=有声音, true=静音
    QPixmap m_volOnPixmap;          // 音量开启图标（中心裁剪后的42×42px）
    QPixmap m_mutePixmap;           // 静音图标（中心裁剪后的42×42px）

private slots:
    void onMuteToggled();           // 静音按钮点击：切换图标和音量状态
};
#endif // MAINWINDOW_H
