#ifndef SELECTWIDGET_H
#define SELECTWIDGET_H

#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QEvent>
#include "BaoBaoType.h"

class SelectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SelectWidget(QWidget *parent = nullptr);

    // 事件过滤器：让returnIconLabel响应点击（绕过透明按钮覆盖问题）
    bool eventFilter(QObject *obj, QEvent *event) override;

    QList<BaoBaoType> getP1SelectedTypes() const;  // 获取1P选择的3个豹豹类型
    QList<BaoBaoType> getP2SelectedTypes() const;  // 获取2P选择的3个豹豹类型

private slots:
    void OnAckClick();      // PLAY按钮 → 确认选择并进入游戏
    void OnReturnClick();   // 返回按钮 → 回到开始界面

private:
    QPushButton *AckButton;        // PLAY按钮（位于界面右下区域）
    QPushButton *ReturnButton;     // 返回按钮
    QLabel *returnIconLabel;       // 返回图标（独立于ReturnButton的视觉层）
    QLabel *bgLabel;               // 背景图片（随circleButtons切换）
    QPushButton *circleButtons[6]; // 6个圆形区域按钮（底部，切换背景）
    QPushButton *selectButtons[12];// 12个选择圈按钮（顶部横向排列）
    QLabel *selectLabels[12];      // 12个"1P"/"2P"标签（覆盖在选择圈上）
    QList<int> p1VisibleList;      // 1P已选中的选择圈索引列表（偶数位）
    QList<int> p2VisibleList;      // 2P已选中的选择圈索引列表（奇数位）
    void changeBackground(int index);  // 切换背景图（index 1~6对应6种豹豹）
    void updatePlayButtonState();      // 根据双方选择数量启用/禁用PLAY按钮
signals:
    void goToGameWidget();     // 确认选择 → 进入游戏界面
    void goToStartWidget();    // 返回 → 回到开始界面
};

#endif // SELECTWIDGET_H
