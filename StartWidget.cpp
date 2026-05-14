#include "StartWidget.h"
#include <QPixmap>
#include <QPalette>

StartWidget::StartWidget(QWidget *parent)
    : QWidget{parent}
{
    setFixedSize(1600, 900);

    // 背景图：从qrc资源加载start_page.jpg并缩放至1600×900
    QLabel *bgLabel = new QLabel(this);
    QPixmap pixmap;
    pixmap.load(":/images/start_page.jpg");
    pixmap = pixmap.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    bgLabel->setPixmap(pixmap);
    bgLabel->setGeometry(0, 0, 1600, 900);
    bgLabel->lower();  // 背景放最底层

    // 创建透明按钮（覆盖背景图中对应区域，按钮本身无文字/无色）
    StartButton = new QPushButton(this);
    ExitButton = new QPushButton(this);

    StartButton->setText("");
    ExitButton->setText("");

    StartButton->setFixedSize(200, 80);
    ExitButton->setFixedSize(80, 80);

    // 主布局
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // 顶部区域：退出按钮在右上角
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addStretch();                     // 左侧弹性空间 → 按钮推到右边
    topLayout->addWidget(ExitButton);            // 退出按钮在右上角
    topLayout->setContentsMargins(0, 20, 10, 0); // 上边距20px，右边距10px

    // 开始按钮区域：大间距推到右下位置
    QVBoxLayout *startLayout = new QVBoxLayout();
    startLayout->addSpacing(800);                // 距离顶部800px → 推到底部
    startLayout->addStretch();
    startLayout->addWidget(StartButton);
    startLayout->setContentsMargins(1020, 200, 0, 250);  // 水平推到右侧

    // 组合布局
    mainLayout->addLayout(topLayout);
    mainLayout->addLayout(startLayout);

    this->setLayout(mainLayout);

    connect(StartButton, &QPushButton::clicked, this, &StartWidget::OnStartClick);
    connect(ExitButton, &QPushButton::clicked, this, &StartWidget::OnExitClick);
}

void StartWidget::OnStartClick()
{
    emit goToSelectWidget();
}

void StartWidget::OnExitClick()
{
    qApp->closeAllWindows();
    qApp->quit();
}
