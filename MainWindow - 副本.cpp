// =============================================================================
// MainWindow.cpp — 主窗口实现：页面创建与信号槽绑定（导航逻辑）
//
// 页面导航流程：
//   开始界面 → (click开始) → 选择界面 → (click PLAY) → 游戏界面
//                            选择界面 → (click返回) → 开始界面
//   游戏界面 → (游戏结束) → 结算界面 → (再来一局) → 游戏界面
//   游戏界面 → (返回按钮) → 选择界面 → (结算界面返回) → 选择界面
// =============================================================================

#include "MainWindow.h"
#include "BaoBaoType.h"
#include <QPainter>
#include <QPainterPath>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)

{
    setFixedSize(1600, 900);    // 固定窗口大小1600×900（匹配背景图分辨率）

    // 创建四个界面实例（均以MainWindow为parent，生命周期由Qt管理）
    startPage = new StartWidget(this);
    selectPage = new SelectWidget(this);
    gamePage = new GameWidget(this);
    resultPage = new ResultWidget(this);

    // QStackedWidget管理多页面切换，初始显示index 0=开始界面
    QStackedWidget *stack = new QStackedWidget(this);
    stack->addWidget(startPage);   // index 0: 开始界面
    stack->addWidget(selectPage);  // index 1: 选择界面
    stack->addWidget(gamePage);    // index 2: 游戏界面
    stack->addWidget(resultPage); // index 3: 结算界面

    setCentralWidget(stack);

    // ---- 页面导航信号绑定 ---------------------------------------------------

    // 开始界面 → 选择界面
    connect(startPage, &StartWidget::goToSelectWidget, [stack](){
        stack->setCurrentIndex(1);
    });

    // 选择界面 → 游戏界面（传递选中的豹豹类型）
    connect(selectPage, &SelectWidget::goToGameWidget, [this, stack](){
            QList<BaoBaoType> p1Types = selectPage->getP1SelectedTypes();
            QList<BaoBaoType> p2Types = selectPage->getP2SelectedTypes();
            gamePage->setSelectedTypes(p1Types, p2Types);
            stack->setCurrentIndex(2);
        });

    // 选择界面 → 开始界面（返回）
    connect(selectPage, &SelectWidget::goToStartWidget, [stack](){
        stack->setCurrentIndex(0);
    });

    // 游戏界面 → 结算界面（携带胜负信息）
    connect(gamePage, &GameWidget::goToResultWidget, [this, stack](bool redWins){
        resultPage->setWinnerText(redWins);
        stack->setCurrentIndex(3);
    });

    // 结算界面 → 游戏界面（再来一局：用已有选择重新开始）
    connect(resultPage, &ResultWidget::goToGameWidget, [this, stack](){
        gamePage->resetGame();
        stack->setCurrentIndex(2);
    });

    // 结算界面 → 选择界面
    connect(resultPage, &ResultWidget::goToSelectWidget, [stack](){
        stack->setCurrentIndex(1);
    });

    // 游戏界面 → 选择界面（返回按钮）
    connect(gamePage, &GameWidget::goToSelectWidget, [stack](){
        stack->setCurrentIndex(1);
    });

    // ---- 静音按钮（圆形覆盖按钮，位于所有界面上层）---------------------------
    // 坐标计算：96.44%,9.78%→(1543,88), 99.08%,13.23%→(1585,119)
    // 取较大维度42px为直径，中心(1564,104)，左上角(1543,83)
    m_muteButton = new QPushButton(this);
    m_muteButton->setGeometry(1543, 83, 42, 42);       // 42×42圆形按钮
    m_muteButton->setCursor(Qt::PointingHandCursor);    // 手型光标提示可点击
    m_muteButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; border-radius: 21px; }"
        "QPushButton:hover { background: rgba(255, 255, 255, 40); }");
    m_muteButton->setIconSize(QSize(42, 42));           // 图标填满整个按钮

    // 中心裁剪为圆形：缩放图片使短边填满目标尺寸，裁剪中心正方形后用圆形路径裁剪边角
    auto centerCropToCircle = [](const QPixmap& source, int size) -> QPixmap {
        QPixmap result(size, size);
        result.fill(Qt::transparent);
        QPainter painter(&result);
        painter.setRenderHint(QPainter::Antialiasing);
        // 按扩展比例缩放（确保两个维度都≥size）
        QPixmap scaled = source.scaled(size, size, Qt::KeepAspectRatioByExpanding,
                                       Qt::SmoothTransformation);
        // 裁剪中心区域
        int x = (scaled.width() - size) / 2;
        int y = (scaled.height() - size) / 2;
        // 用圆形路径裁剪，使四个边角透明
        QPainterPath clipPath;
        clipPath.addEllipse(0, 0, size, size);
        painter.setClipPath(clipPath);
        painter.drawPixmap(0, 0, scaled, x, y, size, size);
        painter.end();
        return result;
    };

    // 加载并裁剪音量图标（圆形裁剪）
    m_volOnPixmap = centerCropToCircle(QPixmap(":/images/yinliang.jpg"), 42);
    m_mutePixmap  = centerCropToCircle(QPixmap(":/images/jingyin.jpg"), 42);

    // 默认显示音量开启图标
    m_muteButton->setIcon(QIcon(m_volOnPixmap));

    // 提升到最上层（确保在所有页面之上可见）
    m_muteButton->raise();

    connect(m_muteButton, &QPushButton::clicked, this, &MainWindow::onMuteToggled);
}

MainWindow::~MainWindow()
{
    delete startPage;
    delete selectPage;
    delete gamePage;
    delete resultPage;
}

// 静音按钮点击：切换图标和音量状态
void MainWindow::onMuteToggled()
{
    m_muted = !m_muted;
    if (m_muted) {
        m_muteButton->setIcon(QIcon(m_mutePixmap));       // 显示静音图标
    } else {
        m_muteButton->setIcon(QIcon(m_volOnPixmap));      // 显示音量图标
    }
    gamePage->setMuted(m_muted);  // 通知游戏界面更新静音状态
}
