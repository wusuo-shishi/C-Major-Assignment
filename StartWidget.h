#ifndef STARTWIDGET_H
#define STARTWIDGET_H

#include <QWidget>
#include <QPushButton>  //按钮
#include <QVBoxLayout>  // 垂直布局
#include <QHBoxLayout>
#include <QLabel>  // 文字标签
#include <QApplication>
class StartWidget : public QWidget
{
    Q_OBJECT
public:
    explicit StartWidget(QWidget *parent = nullptr);
private slots:
    void OnStartClick();  //点击函数
    void OnExitClick();
private:
    QPushButton * StartButton;  //开始按钮
    QPushButton * ExitButton;

signals:
    void goToSelectWidget();  //跳转选择界面
};

#endif // STARTWIDGET_H
