#include "SelectWidget.h"

SelectWidget::SelectWidget(QWidget *parent)
    : QWidget{parent}
{
    AckButton = new QPushButton(this);
    ReturnButton = new QPushButton(this);

    ReturnButton->setText("返回");
    AckButton->setText("确认");

    AckButton->setFixedSize(200,90);
    ReturnButton->setFixedSize(200,90);

    QLabel *label = new QLabel("选择界面", this);
    label->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout(this);

    layout->addWidget(label);
    layout->addWidget(AckButton);
    layout->addWidget(ReturnButton);

    this->setLayout(layout);
    connect(AckButton, &QPushButton::clicked, this, &SelectWidget::OnAckClick);
    connect(ReturnButton, &QPushButton::clicked, this, &SelectWidget::OnReturnClick);
}

void SelectWidget::OnAckClick()
{
    emit goToGameWidget();
}

void SelectWidget::OnReturnClick()
{
    emit goToStartWidget();
}
