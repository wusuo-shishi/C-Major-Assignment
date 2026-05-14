#ifndef STARTWIDGET_H
#define STARTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QApplication>

class StartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StartWidget(QWidget *parent = nullptr);
private slots:
    void OnStartClick();  // 开始游戏 → 跳转选择界面
    void OnExitClick();   // 退出游戏 → 关闭程序

private:
    QPushButton *StartButton;  // 开始按钮（右下角，透明覆盖在背景对应位置）
    QPushButton *ExitButton;   // 退出按钮（右上角，透明覆盖）

signals:
    void goToSelectWidget();  // 跳转到豹豹选择界面
};

#endif // STARTWIDGET_H
