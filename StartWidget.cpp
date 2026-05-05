#include "StartWidget.h"
#include <QPixmap>
#include <QPalette>

StartWidget::StartWidget(QWidget *parent)
    : QWidget{parent}
{
    // 设置窗口固定大小
    setFixedSize(1600, 900);

    // 背景标签
    QLabel *bgLabel = new QLabel(this);
    QPixmap pixmap;
    pixmap.load(":/images/start_page.jpg");
    pixmap = pixmap.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    bgLabel->setPixmap(pixmap);
    bgLabel->setGeometry(0, 0, 1600, 900);
    bgLabel->lower();

    // 创建按钮
    StartButton = new QPushButton(this);
    ExitButton = new QPushButton(this);

    StartButton->setText("");
    ExitButton->setText("");

    StartButton->setFixedSize(200, 80);
    ExitButton->setFixedSize(200, 80);

    // ========== 主布局 ==========
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);

    // ========== 顶部区域：退出按钮在右上角 ==========
    QHBoxLayout *topLayout = new QHBoxLayout();
    topLayout->addStretch();                    // 左侧弹性空间，把按钮推到右边
    topLayout->addWidget(ExitButton);           // 退出按钮在右上角
    topLayout->setContentsMargins(0, 20, 30, 0); // 上边距20，右边距30

    // ========== 开始按钮区域 ==========
    QVBoxLayout *startLayout = new QVBoxLayout();
    startLayout->addSpacing(800);               // 距离顶部
    startLayout->addStretch();                  // 底部弹性空间
    startLayout->addWidget(StartButton);           //
    startLayout->setContentsMargins(1020, 200, 0, 250);

    // 将顶部和开始区域加入主布局
    mainLayout->addLayout(topLayout);           // 顶部区域（退出按钮）
    mainLayout->addLayout(startLayout);         // 开始按钮区域

    this->setLayout(mainLayout);

    // 连接信号
    connect(StartButton, &QPushButton::clicked, this, &StartWidget::OnStartClick);
    connect(ExitButton, &QPushButton::clicked, this, &StartWidget::OnExitClick);
}

// 点击开始按钮
void StartWidget::OnStartClick()
{
    emit goToSelectWidget();
}

// 点击退出按钮
void StartWidget::OnExitClick()
{
    qApp->closeAllWindows();
    qApp->quit();
}
