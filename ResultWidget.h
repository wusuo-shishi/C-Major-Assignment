#ifndef RESULTWIDGET_H
#define RESULTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QApplication>

class ResultWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ResultWidget(QWidget *parent = nullptr);
    void setWinnerText(bool redWins);    // 设置获胜方文字（true=红色方, false=蓝色方）
private slots:
    void OnReplayClick();    // "再来一局"→用相同阵容重新开始
    void OnReselectClick();  // "重新选择"→返回角色选择界面

private:
    QLabel *winnerLabel;           // "红色方获胜！"/"蓝色方获胜！"
    QPushButton *ReplayButton;     // 再来一局（蓝色渐变）
    QPushButton *ReselectButton;   // 重新选择（红色渐变）

signals:
    void goToGameWidget();     // 再来一局 → 跳转游戏界面
    void goToSelectWidget();   // 重新选择 → 跳转选择界面
};

#endif // RESULTWIDGET_H
