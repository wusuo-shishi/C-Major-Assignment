#ifndef SELECTWIDGET_H
#define SELECTWIDGET_H

#include <QWidget>
#include <QPushButton>  //按钮
#include <QVBoxLayout>  // 垂直布局
#include <QLabel>  // 文字标签

class SelectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit SelectWidget(QWidget *parent = nullptr);

private slots:
    void OnAckClick();
    void OnReturnClick();

private:
    QPushButton *AckButton;
    QPushButton *ReturnButton;
signals:
    void goToGameWidget();
    void goToStartWidget();
};

#endif // SELECTWIDGET_H
