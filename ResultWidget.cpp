#include "ResultWidget.h"

ResultWidget::ResultWidget(QWidget *parent)
    : QWidget{parent}
{
    BackButton = new QPushButton(this);
    ExitButton = new QPushButton(this);

    BackButton->setFixedSize(200,90);
    ExitButton->setFixedSize(200,90);

    BackButton->setText("返回首页");
    ExitButton->setText("退出游戏");

    QLabel *label = new QLabel("结算界面", this);
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(BackButton);
    layout->addWidget(ExitButton);
    layout->addWidget(label);

    this->setLayout(layout);

    connect(BackButton, &QPushButton::clicked, this, &ResultWidget::OnBackClick);
    connect(ExitButton, &QPushButton::clicked, this, &ResultWidget::OnExitClick);
}

void ResultWidget::OnBackClick()
{
    emit goToStartWidget();
}

void ResultWidget::OnExitClick()
{
    qApp->closeAllWindows();
    qApp->quit();
}
