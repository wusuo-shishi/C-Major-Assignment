#include "ResultWidget.h"

ResultWidget::ResultWidget(QWidget *parent)
    : QWidget{parent}
{
    setFixedSize(1600, 900);

    // 结算界面背景色
    setStyleSheet("ResultWidget { background-color: rgba(20, 20, 40, 230); }");

    // 获胜标签（初始空文本，由setWinnerText设置）
    winnerLabel = new QLabel(this);
    winnerLabel->setAlignment(Qt::AlignCenter);
    winnerLabel->setStyleSheet(
        "QLabel {"
        "   color: white;"
        "   font-size: 48px;"
        "   font-weight: bold;"
        "   background-color: rgba(0, 0, 0, 180);"
        "   border-radius: 20px;"
        "   padding: 30px 60px;"
        "}"
    );

    // 再来一局按钮（蓝色渐变，与PLAY按钮同款样式）
    ReplayButton = new QPushButton(this);
    ReplayButton->setText("再来一局");
    ReplayButton->setFixedSize(300, 80);
    ReplayButton->setStyleSheet(
        "QPushButton {"
        "   background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #67d7e8, stop:0.5 #4fc3f7, stop:1 #29b6f6);"
        "   border-radius: 40px;"
        "   border: none;"
        "   color: white;"
        "   font-size: 28px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #81e4f8, stop:0.5 #64d3f5, stop:1 #42c5f3);"
        "}"
        "QPushButton:pressed {"
        "   background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #4fc3f7, stop:0.5 #29b6f6, stop:1 #03a9f4);"
        "}"
    );

    // 重新选择按钮（红色渐变，警示色调）
    ReselectButton = new QPushButton(this);
    ReselectButton->setText("重新选择");
    ReselectButton->setFixedSize(300, 80);
    ReselectButton->setStyleSheet(
        "QPushButton {"
        "   background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #ff7675, stop:0.5 #e74c3c, stop:1 #c0392b);"
        "   border-radius: 40px;"
        "   border: none;"
        "   color: white;"
        "   font-size: 28px;"
        "   font-weight: bold;"
        "}"
        "QPushButton:hover {"
        "   background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #ff9f9e, stop:0.5 #f76b6a, stop:1 #e74c3c);"
        "}"
        "QPushButton:pressed {"
        "   background: qlineargradient(spread:pad, x1:0, y1:0, x2:0, y2:1, stop:0 #e74c3c, stop:0.5 #c0392b, stop:1 #a93226);"
        "}"
    );

    // 垂直居中布局
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->setAlignment(Qt::AlignCenter);
    layout->addStretch();
    layout->addWidget(winnerLabel, 0, Qt::AlignCenter);
    layout->addSpacing(50);
    layout->addWidget(ReplayButton, 0, Qt::AlignCenter);
    layout->addSpacing(20);
    layout->addWidget(ReselectButton, 0, Qt::AlignCenter);
    layout->addStretch();

    this->setLayout(layout);

    connect(ReplayButton, &QPushButton::clicked, this, &ResultWidget::OnReplayClick);
    connect(ReselectButton, &QPushButton::clicked, this, &ResultWidget::OnReselectClick);
}

void ResultWidget::setWinnerText(bool redWins)
{
    if (redWins) {
        winnerLabel->setText("红色方获胜！");
    } else {
        winnerLabel->setText("蓝色方获胜！");
    }
}

void ResultWidget::OnReplayClick()
{
    emit goToGameWidget();
}

void ResultWidget::OnReselectClick()
{
    emit goToSelectWidget();
}
