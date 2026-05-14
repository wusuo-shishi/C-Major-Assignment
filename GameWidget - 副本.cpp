// =============================================================================
// GameWidget.cpp — 游戏核心逻辑实现
//
// 整体架构：定时器驱动 → updatePhysics() 每16ms循环:
//   1. 更新旋转动画 + 自转衰减
//   2. 移动所有isMoving豹豹（带边界反弹 + 非线性缓动）
//   3. handleCollisions() 检测豹豹间碰撞 → 伤害/技能/物理反弹
//   4. 轮次切换判定（当前阵营全部停止后交换出手权）
//   5. 屏幕震动衰减 + 边界安全网 + 显示刷新
//
// 拖拽系统：mousePress(选中) → mouseMove(拖拽向量) → mouseRelease(发射)
//   发射公式: speed = ratio × 20, totalDistance = effectiveDist² / 12.5
//   运动手感: 四段缓动(爆发2.5x→衰减→匀速1x→缓停)
//
// 碰撞系统：圆形碰撞检测(半径30) + 镜面反射 + 速度/行动值衰减
//   主动方(速度大/正在移动)对被动方造成伤害和弹开效果
//   每种豹豹类型有独特技能（橡胶+ATK/天使回血/大力+ATK/红温200%/绷带+10ATK/嘭嘭水弹）
//
// 音效系统：CollisionSoundPlayer独立QThread + MCI 4通道轮转
//   主线程 emit requestCollisionSound → QueuedConnection → 音频线程 play
// =============================================================================

#include "GameWidget.h"
#include <QtMath>
#include <QDebug>
#include <QTimerEvent>
#include <QPainterPath>
#include <QPolygonF>
#include <QRandomGenerator>
#include <QMessageBox>
#include <QThread>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <windows.h>    // GetFileAttributesW（检测文件存在）, mciSendStringW（MCI音效）
#include <mmsystem.h>   // PlaySoundW（SND_ASYNC备用方案）
#pragma comment(lib, "winmm.lib")  // 链接winmm.lib以使用MCI/Wave API
#include <cmath>

// =============================================================================
// 豹豹基础属性数据
// =============================================================================
struct BaoBaoStats {
    int hp;
    int atk;
};

// 根据豹豹类型返回基础HP和ATK
BaoBaoStats getBaoBaoStats(BaoBaoType type) {
    switch (type) {
    case BaoBaoType::Xiangjiao:
        return {40, 5};
    case BaoBaoType::Tianshi:
        return {45, 6};
    case BaoBaoType::Dali:
        return {35, 6};
    case BaoBaoType::Hongwen:
        return {35, 8};
    case BaoBaoType::Bengdai:
        return {35, 5};
    case BaoBaoType::Pengpeng:
        return {40, 6};
    default:
        return {45, 5};
    }
}

// 根据豹豹类型返回对应头像图片的qrc资源路径
QString getBaoBaoImagePath(BaoBaoType type) {
    switch (type) {
    case BaoBaoType::Xiangjiao:
        return ":/images/xiangjiao_baobao.jpg";
    case BaoBaoType::Tianshi:
        return ":/images/tianshi_baobao.jpg";
    case BaoBaoType::Dali:
        return ":/images/dali_baobao.jpg";
    case BaoBaoType::Hongwen:
        return ":/images/hongwen_baobao.jpg";
    case BaoBaoType::Bengdai:
        return ":/images/bengdai_baobao.jpg";
    case BaoBaoType::Pengpeng:
        return ":/images/pengpeng_baobao.jpg";
    default:
        return ":/images/xiangjiao_baobao.jpg";
    }
}

// =============================================================================
// 构造函数：初始化游戏界面的所有组件
// 执行顺序：背景→梯形边界(预计算法线)→豹豹→头像→得分标签→音效→返回按钮→轮次标签→定时器
// =============================================================================
GameWidget::GameWidget(QWidget *parent)
    : QWidget{parent}
{
    setMouseTracking(true);  // 需要持续接收mouseMoveEvent来更新拖拽预览线

    // 加载并缩放背景图至1600×900
    m_bgPixmap = QPixmap(":/images/game_background.jpeg");
    m_bgPixmap = m_bgPixmap.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QLabel *label = new QLabel("游戏界面", this);
    label->setAlignment(Qt::AlignCenter);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);

    // ---- 初始化梯形碰撞区域（百分比坐标 → 像素坐标）-------------------------
    m_boundaryVerts[0] = QPointF(24.48 / 100.0 * 1600, 20.65 / 100.0 * 900);   // 左上角
    m_boundaryVerts[1] = QPointF(74.95 / 100.0 * 1600, 21.57 / 100.0 * 900);   // 右上角
    m_boundaryVerts[2] = QPointF(81.72 / 100.0 * 1600, 86.11 / 100.0 * 900);   // 右下角
    m_boundaryVerts[3] = QPointF(17.97 / 100.0 * 1600, 85.83 / 100.0 * 900);   // 左下角

    // 预计算四条边的内侧法线（顺时针多边形，法线指向内部）和边长倒数
    // 这些值在整个游戏期间不变，预计算可避免每帧重复std::hypot
    for (int e = 0; e < 4; e++) {
        QPointF edgeDir = m_boundaryVerts[(e + 1) % 4] - m_boundaryVerts[e];
        qreal len = std::hypot(edgeDir.x(), edgeDir.y());
        m_boundaryInvLen[e] = 1.0 / len;
        // 顺时针 → 内侧法线 = (-edgeDir.y, edgeDir.x) 归一化（顺时针转90°指向内部）
        m_boundaryInNormals[e] = QPointF(-edgeDir.y() * m_boundaryInvLen[e],
                                          edgeDir.x() * m_boundaryInvLen[e]);
    }


    initBaobaos();     // 创建6个豹豹的显示组件（QLabel + 圆形裁剪头像）
    initTouxiang();    // 创建1P/2P头像标签（从大图指定区域裁剪）
    initSlLabels();    // 创建SL/SR得分标签（顶部两侧对称排列）
    initSounds();      // 创建音频线程 + 注册MCI多通道

    // ---- 全透明返回按钮 ----------------------------------------------------
    // 位置：像素坐标(104, 18, 36×30)，覆盖背景图左上角的返回箭头区域
    m_backButton = new QPushButton(this);
    m_backButton->setGeometry(104, 18, 36, 30);
    m_backButton->setFlat(true);
    m_backButton->setCursor(Qt::PointingHandCursor);
    m_backButton->setStyleSheet(
        "QPushButton { background: transparent; border: none; }"
        "QPushButton:hover { background: rgba(255, 255, 255, 30); }");
    connect(m_backButton, &QPushButton::clicked, this, &GameWidget::onBackButtonClicked);

    // ---- 轮次标签 ----------------------------------------------------------
    // 位置：(50.21%, 3.15%) → 像素(803, 28)，水平居中
    m_roundLabel = new QLabel("第1轮", this);
    m_roundLabel->setAlignment(Qt::AlignCenter);
    m_roundLabel->setStyleSheet(
        "QLabel { color: white; font-size: 20px; font-weight: bold;"
        " background-color: rgba(0, 0, 0, 100); border-radius: 8px; padding: 4px 16px; }");
    m_roundLabel->adjustSize();
    m_roundLabel->move(803 - m_roundLabel->width() / 2, 28);
    m_roundLabel->show();

    // 启动物理定时器（16ms ≈ 62.5fps 的物理更新频率）
    startTimer(16);
}

// =============================================================================
// 初始化6个豹豹的显示组件（使用start_page.jpg中的占位头像作为临时图片）
// 实际的豹豹类型图片在setSelectedTypes中才会替换
// =============================================================================
void GameWidget::initBaobaos()
{
    QPixmap originalPixmap(":/images/start_page.jpg");

    m_baobaos.clear();

    for (int i = 0; i < 6; i++) {
        BaoBaoObject bao;
        bao.camp = (i < 3) ? order : !order;  // 根据索引初始化阵营

        // 默认位置：红方(0~2)在右侧，蓝方(3~5)在左侧（百分比坐标）
        QPointF defaultPositions[6] = {
            QPointF(70.99 / 100.0 * 1600, 32.96 / 100.0 * 900),  // 红方1 右上
            QPointF(66.56 / 100.0 * 1600, 48.33 / 100.0 * 900),  // 红方2 右中
            QPointF(73.44 / 100.0 * 1600, 65.28 / 100.0 * 900),  // 红方3 右下
            QPointF(29.64 / 100.0 * 1600, 33.15 / 100.0 * 900),  // 蓝方1 左上
            QPointF(34.22 / 100.0 * 1600, 48.15 / 100.0 * 900),  // 蓝方2 左中
            QPointF(27.03 / 100.0 * 1600, 65.28 / 100.0 * 900),  // 蓝方3 左下
        };
        bao.center = defaultPositions[i];
        bao.collisionRect = QRect(qRound(bao.center.x() - 30), qRound(bao.center.y() - 30), 60, 60);
        bao.velocityF = QPointF(0, 0);
        bao.decorationRotation = 0;
        bao.remainingDistance = 0;
        bao.totalDistance = 0;

        // 创建60×60圆形裁剪头像（从start_page.jpg裁剪）
        QPixmap circlePixmap(60, 60);
        circlePixmap.fill(Qt::transparent);
        QPainter painter(&circlePixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addEllipse(0, 0, 60, 60);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, 60, 60, originalPixmap);
        painter.end();

        // HP标签：位于碰撞箱右下角外侧，显示当前血量
        bao.hpLabel = new QLabel(this);
        bao.hpLabel->setAlignment(Qt::AlignCenter);
        bao.hpLabel->show();

        // ATK标签：位于HP标签下方，显示当前攻击力
        bao.atkLabel = new QLabel(this);
        bao.atkLabel->setAlignment(Qt::AlignCenter);
        bao.atkLabel->show();

        // 头像图片：60×60圆形QLabel
        bao.label = new QLabel(this);
        bao.label->setPixmap(circlePixmap);
        bao.label->setFixedSize(60, 60);
        bao.label->move(bao.collisionRect.x(), bao.collisionRect.y());
        bao.label->setAttribute(Qt::WA_TranslucentBackground);
        bao.label->show();

        m_baobaos.append(bao);
    }
    // 注意：不在initBaobaos中设置HP/ATK属性
    // 实际情况是setSelectedTypes在之后统一设置类型→属性→刷新
}

// =============================================================================
// 从背景图和豹豹头像大图中裁剪1P/2P头像缩略图
//
// 原理：背景图中预留了6个头像位（L1~L6），每个宽xi像素、等间距排列
//       X区域（1P/红方）：从baobao_touxiang.jpeg提取3个头像
//       Y区域（2P/蓝方）：从baobao_touxiang.jpeg对应位置提取3个头像
//       未使用的位（索引3~5）用背景图填充，以保持视觉一致性
// =============================================================================
void GameWidget::initTouxiang()
{
    // ---- 1P红方头像裁剪区域（X区域）----------------------------------------
    // A(23.80%, 7.59%) ~ D(30.21%, 16.76%)：第一个头像的包围矩形
    // B(30.57%, 7.59%)：第二个头像的起始位置
    // 相邻头像间距 X = B.x - A.x
    qreal ax = 23.80 / 100.0 * 1600;
    qreal ay = 7.59 / 100.0 * 900;
    qreal dx = 30.21 / 100.0 * 1600;
    qreal dy = 16.76 / 100.0 * 900;
    qreal bx = 30.57 / 100.0 * 1600;
    qreal X = bx - ax;
    int xi = qRound(X);             // 头像宽度（像素）
    qreal tw = dx - ax;             // 单个裁剪宽度
    qreal th = dy - ay;             // 单个裁剪高度
    int txi = qRound(ax);           // 裁剪区域左上角X
    int tyi = qRound(ay);           // 裁剪区域左上角Y
    int twi = qRound(tw);           // 裁剪宽度（取整）
    int thi = qRound(th);           // 裁剪高度（取整）

    // ---- 2P蓝方头像裁剪区域（Y区域）----------------------------------------
    // E(62.50%, 7.31%) ~ F(55.89%, 16.57%)：第一头像在URL另一侧
    qreal ex = 62.50 / 100.0 * 1600;
    qreal ey = 7.31 / 100.0 * 900;
    qreal fx = 55.89 / 100.0 * 1600;
    qreal fy = 16.57 / 100.0 * 900;
    qreal yLeft = qMin(ex, fx);
    qreal yTop = qMin(ey, fy);
    qreal yw = qAbs(ex - fx);
    qreal yh = qAbs(ey - fy);
    int yxi = qRound(yLeft);
    int yyi = qRound(yTop);
    int ywi = qRound(yw);
    int yhi = qRound(yh);

    // 加载并缩放源图片到1600×900
    QPixmap baoFull(":/images/baobao_touxiang.jpeg");
    baoFull = baoFull.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPixmap bgFull(":/images/game_background.jpeg");
    bgFull = bgFull.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // 1P头像：索引0~2从大头像图裁剪，索引3~5用背景图填充（保持格子视觉）
    m_touxiangPixmaps[0] = baoFull.copy(txi, tyi, twi, thi);
    m_touxiangPixmaps[1] = baoFull.copy(txi + xi, tyi, twi, thi);
    m_touxiangPixmaps[2] = baoFull.copy(txi + 2 * xi, tyi, twi, thi);
    m_touxiangPixmaps[3] = bgFull.copy(txi, tyi, twi, thi);
    m_touxiangPixmaps[4] = bgFull.copy(txi + xi, tyi, twi, thi);
    m_touxiangPixmaps[5] = bgFull.copy(txi + 2 * xi, tyi, twi, thi);

    // 2P头像：同理，从Y区域裁剪
    m_touxiangPixmaps2[0] = baoFull.copy(yxi, yyi, ywi, yhi);
    m_touxiangPixmaps2[1] = baoFull.copy(yxi + xi, yyi, ywi, yhi);
    m_touxiangPixmaps2[2] = baoFull.copy(yxi + 2 * xi, yyi, ywi, yhi);
    m_touxiangPixmaps2[3] = bgFull.copy(yxi, yyi, ywi, yhi);
    m_touxiangPixmaps2[4] = bgFull.copy(yxi + xi, yyi, ywi, yhi);
    m_touxiangPixmaps2[5] = bgFull.copy(yxi + 2 * xi, yyi, ywi, yhi);

    // 创建QLabel并放置在计算好的位置
    for (int i = 0; i < 3; i++) {
        m_touxiangLabels[i] = new QLabel(this);
        m_touxiangLabels[i]->setFixedSize(xi, thi);
        m_touxiangLabels[i]->move(txi + i * xi, tyi);
        m_touxiangLabels[i]->setScaledContents(true);
        m_touxiangLabels[i]->setStyleSheet("QLabel { border: none; background: transparent; }");

        m_touxiangLabels2[i] = new QLabel(this);
        m_touxiangLabels2[i]->setFixedSize(xi, yhi);
        m_touxiangLabels2[i]->move(yxi + i * xi, yyi);
        m_touxiangLabels2[i]->setScaledContents(true);
        m_touxiangLabels2[i]->setStyleSheet("QLabel { border: none; background: transparent; }");
    }
}

// =============================================================================
// 初始化SL/SR得分标签
//
// SL标签（左侧）：蓝色方击败红色方豹豹时显现，从左往右排列SL1~SL6
// SR标签（右侧）：红色方击败蓝色方豹豹时显现，从右往左排列SR1~SR6
// SL和SR关于中轴线(x=800)对称
//
// 坐标计算：
//   A(28.96%, 5.19%) ~ G(30.78%, 1.94%) 定义单个图标的包围矩形
//   5个相邻点AB→BC→CD→DE→EF 的平均距离R = 相邻图标水平间距
// =============================================================================
void GameWidget::initSlLabels()
{
    // 加载得分图并缩放至1600×900
    QPixmap defenFull(":/images/defen.jpeg");
    defenFull = defenFull.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // 对顶角矩形：A(28.96%,5.19%)和G(30.78%,1.94%) 定义SL图标裁剪区域
    qreal ax = 28.96 / 100.0 * 1600;
    qreal ay = 5.19 / 100.0 * 900;
    qreal gx = 30.78 / 100.0 * 1600;
    qreal gy = 1.94 / 100.0 * 900;

    qreal slX = qMin(ax, gx);
    qreal slY = qMin(ay, gy);
    qreal slW = qAbs(gx - ax);
    qreal slH = qAbs(gy - ay);

    int slXi = qRound(slX);   // ≈463
    int slYi = qRound(slY);   // ≈17
    int slWi = qRound(slW);   // ≈29
    int slHi = qRound(slH);   // ≈29

    // 裁剪SL图标（29×29px的一个得分标记）
    m_slPixmap = defenFull.copy(slXi, slYi, slWi, slHi);

    // 计算6个得分位点间的平均间距R（百分比坐标差取平均）
    qreal distAB = std::hypot(31.04 - 28.96, 5.19 - 5.19);
    qreal distBC = std::hypot(33.23 - 31.04, 5.00 - 5.19);
    qreal distCD = std::hypot(35.31 - 33.23, 5.00 - 5.00);
    qreal distDE = std::hypot(37.34 - 35.31, 5.28 - 5.00);
    qreal distEF = std::hypot(39.43 - 37.34, 5.28 - 5.28);
    qreal R_percent = (distAB + distBC + distCD + distDE + distEF) / 5.0;  // ≈2.0995%
    qreal R_px = R_percent / 100.0 * 1600;  // ≈33.59px

    // 创建SL1~SL6标签（左侧，从左往右水平排列，初始全部隐藏）
    for (int i = 0; i < 6; i++) {
        m_slLabels[i] = new QLabel(this);
        m_slLabels[i]->setPixmap(m_slPixmap);
        m_slLabels[i]->setFixedSize(slWi, slHi);
        m_slLabels[i]->setScaledContents(true);
        m_slLabels[i]->move(qRound(slX + i * R_px), slYi);
        m_slLabels[i]->setStyleSheet("QLabel { border: none; background: transparent; }");
        m_slLabels[i]->hide();
    }

    // 裁剪SR图标：从原图中轴线对称位置提取（x' = 1600 - x - w）
    int srXi = 1600 - slXi - slWi;  // 关于x=800对称
    m_srPixmap = defenFull.copy(srXi, slYi, slWi, slHi);

    // 创建SR1~SR6标签（右侧，从右往左排列，与SL镜像对称）
    // SR[i] 的 x = 1600 - (slX + i*R_px + slWi)，使得SL[i]与SR[i]关于x=800对称
    qreal srBaseX = 1600 - slX - slWi;
    for (int i = 0; i < 6; i++) {
        m_srLabels[i] = new QLabel(this);
        m_srLabels[i]->setPixmap(m_srPixmap);
        m_srLabels[i]->setFixedSize(slWi, slHi);
        m_srLabels[i]->setScaledContents(true);
        m_srLabels[i]->move(qRound(srBaseX - i * R_px), slYi);
        m_srLabels[i]->setStyleSheet("QLabel { border: none; background: transparent; }");
        m_srLabels[i]->hide();
    }
}

// =============================================================================
// 根据红色方累计死亡数更新SL标签
// 蓝色方每击败一个红色方豹豹，从左往右依次点亮SL1→SL2→...→SL6
// =============================================================================
void GameWidget::updateSlLabels()
{
    for (int i = 0; i < 6; i++) {
        if (i < m_redDeaths) {
            m_slLabels[i]->show();
        } else {
            m_slLabels[i]->hide();
        }
    }
}

// =============================================================================
// 根据蓝色方累计死亡数更新SR标签
// 红色方每击败一个蓝色方豹豹，从右往左依次点亮SR1→SR2→...→SR6
// =============================================================================
void GameWidget::updateSrLabels()
{
    for (int i = 0; i < 6; i++) {
        if (i < m_blueDeaths) {
            m_srLabels[i]->show();
        } else {
            m_srLabels[i]->hide();
        }
    }
}

// =============================================================================
// CollisionSoundPlayer 实现
//   在音频线程中用MCI打开4个独立通道（cs_0~cs_3），play时轮转使用
//   MCI alias机制允许多个通道同时播放同一WAV文件
// =============================================================================
void CollisionSoundPlayer::init()
{
    // 检测文件是否存在（避免MCI阻塞等待不存在的文件）
    if (GetFileAttributesW(m_path.c_str()) == INVALID_FILE_ATTRIBUTES) return;
    for (int i = 0; i < CHANNELS; i++) {
        std::wstring alias = L"cs_" + std::to_wstring(i);
        // 以type waveaudio打开WAV文件并注册别名
        mciSendStringW((L"open \"" + m_path + L"\" type waveaudio alias " + alias).c_str(),
                       NULL, 0, NULL);
        // 设置音量为80%（范围0~1000）
        mciSendStringW((L"setaudio " + alias + L" volume to 800").c_str(), NULL, 0, NULL);
    }
}

// 轮转播放：每次使用不同别名，使得前一次播放的通道不会被覆盖
void CollisionSoundPlayer::play()
{
    std::wstring alias = L"cs_" + std::to_wstring(m_slot);
    m_slot = (m_slot + 1) % CHANNELS;  // 循环0→1→2→3→0→...
    mciSendStringW((L"play " + alias + L" from 0").c_str(), NULL, 0, NULL);
}

// =============================================================================
// 音效系统初始化：创建独立音频线程 + MCI多通道注册
// 关键设计：
//   1. m_soundPlayer moveToThread(m_soundThread) → play在音频线程执行
//   2. Qt::QueuedConnection → emit信号不会阻塞主线程
//   3. MCI open必须在play所在线程调用，使用invokeMethod入队
//   4. 线程finished时自动deleteLater所有对象 → 无内存泄漏
// =============================================================================
void GameWidget::initSounds()
{
    // 音效路径自动解析：
    //   Release: <exe目录>/sounds/collision.wav
    //   Dev:     从build目录退回3级到项目根目录/sounds/collision.wav
    QString exeDir = QCoreApplication::applicationDirPath();
    QString collisionFile = exeDir + "/sounds/collision.wav";
    if (!QFile::exists(collisionFile)) {
        QDir dir(exeDir);
        dir.cdUp(); dir.cdUp(); dir.cdUp();
        collisionFile = dir.absolutePath() + "/sounds/collision.wav";
    }
    std::wstring collisionPath = collisionFile.toStdWString();

    // 播放器和线程均无parent → 由finished信号自动deleteLater（线程安全）
    m_soundPlayer = new CollisionSoundPlayer();
    m_soundThread = new QThread();

    // 在moveToThread之前设置路径（主线程写入，音频线程启动后只读，安全）
    m_soundPlayer->setPath(collisionPath);
    m_soundPlayer->moveToThread(m_soundThread);

    // 跨线程信号绑定：主线程 emit requestCollisionSound → 音频线程 play()
    connect(this, &GameWidget::requestCollisionSound,
            m_soundPlayer, &CollisionSoundPlayer::play,
            Qt::QueuedConnection);

    // 线程结束时自动清理：播放器 + 线程自身均deleteLater
    connect(m_soundThread, &QThread::finished,
            m_soundPlayer, &QObject::deleteLater);
    connect(m_soundThread, &QThread::finished,
            m_soundThread, &QObject::deleteLater);

    m_soundThread->start();  // 启动音频线程（进入事件循环）

    // MCI open必须与play在同一线程中调用
    // 使用invokeMethod将init()入队到音频线程的事件队列
    QMetaObject::invokeMethod(m_soundPlayer, "init", Qt::QueuedConnection);
}

// 发射碰撞音效信号（主线程调用，瞬间返回不阻塞）
// 如果处于静音状态则直接跳过，不发射信号
void GameWidget::playCollisionSound()
{
    if (m_muted) return;
    emit requestCollisionSound();
}

// 触发屏幕震动：碰撞时调用，intensity根据相对速度计算，逐帧衰减
void GameWidget::triggerShake(qreal intensity, int duration)
{
    m_shakeIntensity = intensity;
    m_shakeFrames = duration;
}

// =============================================================================
// 四段非线性缓动函数：模拟"爆发→匀速→缓停"的运动手感
// 输入 t ∈ [0, 1]（运动进度 = 1 - remaining/total）
// 返回：当前帧的速度倍率（乘以基础速度得到实际位移）
//
// 阶段划分：
//   [0, 15%]   爆发段 → 倍率2.5（快速起步，模拟弹射初始加速度）
//   [15%, 40%] 过渡段 → 线性衰减 2.5→1.0（平滑过渡到匀速）
//   [40%, 70%] 匀速段 → 倍率1.0（稳定巡航）
//   [70%, 100%] 缓停段 → 二次曲线衰减 1.0→0.15（逐渐减速停下）
// =============================================================================
qreal GameWidget::getEasingMultiplier(qreal t)
{
    if (t < 0.15) return 2.5;
    if (t < 0.4)  return 3.4 - 6.0 * t;          // 线性插值: 2.5 - 1.5*(t-0.15)/0.25
    if (t < 0.7)  return 1.0;
    qreal local = (t - 0.7) * 3.33333333333333;   // 归一化到[0,1]: 1.0/0.3 ≈ 3.333
    return 1.0 - 0.85 * local * local;            // 二次缓出: 1.0 - 0.85*t²
}

// =============================================================================
// 边界安全网：每帧将所有豹豹圆心强制钳制在梯形边界内侧
//
// 原理：对每条边计算圆心到边的有符号距离signedDist
//       signedDist < r(豹豹半径30) → 向外推出到距离=r处
//
// 使用预计算法线（m_boundaryInNormals），避免每帧std::hypot调用
// =============================================================================
void GameWidget::clampAllToBoundary()
{
    const qreal r = 30.0;  // 豹豹碰撞半径
    for (int i = 0; i < 6; i++) {
        QPointF& c = m_baobaos[i].center;
        for (int e = 0; e < 4; e++) {
            // 有符号距离 = 圆心到边起点的向量 · 内侧法线
            qreal signedDist = (c.x() - m_boundaryVerts[e].x()) * m_boundaryInNormals[e].x()
                             + (c.y() - m_boundaryVerts[e].y()) * m_boundaryInNormals[e].y();
            if (signedDist < r) {
                // 将圆心沿法线向外推，使距离恢复到r
                c.setX(c.x() + m_boundaryInNormals[e].x() * (r - signedDist));
                c.setY(c.y() + m_boundaryInNormals[e].y() * (r - signedDist));
            }
        }
    }
}

// 获取类型对应的头像索引（0~5），用于从m_touxiangPixmaps数组中选择正确的裁剪头像
int GameWidget::getTouxiangIndex(BaoBaoType type)
{
    switch (type) {
    case BaoBaoType::Xiangjiao: return 0;
    case BaoBaoType::Tianshi:   return 1;
    case BaoBaoType::Dali:      return 2;
    case BaoBaoType::Hongwen:   return 3;
    case BaoBaoType::Bengdai:   return 4;
    case BaoBaoType::Pengpeng:  return 5;
    default:                    return -1;
    }
}

// =============================================================================
// 鼠标按下事件：检测点击是否命中当前可操作的豹豹
// 条件：①当前阵营的豹豹(order匹配) ②未在移动 ③本轮未行动过
// 命中后进入拖拽态，记录起始位置并触发重绘（显示拖拽预览线）
// =============================================================================
void GameWidget::mousePressEvent(QMouseEvent *event)
{
    QPointF clickPos = event->pos();

    // 倒序检测（后画的在上层，优先选中视觉上最靠前的豹豹）
    for (int i = 5; i >= 0; i--) {
        QPointF delta = clickPos - m_baobaos[i].center;
        qreal distSq = delta.x() * delta.x() + delta.y() * delta.y();

        // 检查是否为当前行动方的豹豹
        bool isCurrentTurn = (order && i < 3) || (!order && i > 2);
        if (!isCurrentTurn) continue;

        if (distSq <= 30 * 30) {  // 点击在圆形碰撞半径内
            if (m_baobaos[i].isMoving) return;   // 正在移动 → 不可选中
            if (m_baobaos[i].hasActed) return;   // 已出手 → 不可选中

            m_isDragging = true;
            m_draggedIndex = i;
            m_dragStartPos = clickPos;
            m_currentMousePos = clickPos;
            m_rawDragVector = QPointF(0, 0);     // 清空拖拽向量

            update();  // 触发paintEvent绘制拖动预览线
            return;
        }
    }
}

// =============================================================================
// 鼠标移动事件：更新拖拽向量（弹弓式拖拽预览）
//
// 拖拽逻辑：
//   1. 计算原始拖拽向量 rawVector = currentPos - dragStartPos
//   2. minDrag(40px)死区 → 小于此距离的拖动无效，防止误触
//   3. 扣除死区后的有效向量 = rawVector × (len - minDrag) / len
//   4. 有效距离 > maxDrag(200px) → 钳制到200px
//   5. m_currentMousePos 跟随鼠标但不超过maxDrag范围
// =============================================================================
void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        QPointF currentPos = event->pos();
        QPointF rawVector = currentPos - m_dragStartPos;
        qreal rawDistance = qSqrt(rawVector.x() * rawVector.x() + rawVector.y() * rawVector.y());

        qreal maxDrag = 200;  // 最大拖拽距离（像素）
        qreal minDrag = 40;   // 最小拖拽距离（死区，防止误触）

        if (rawDistance < minDrag) {
            // 拖拽距离不足 → 清空拖拽向量
            m_rawDragVector = QPointF(0, 0);
            m_currentMousePos = m_dragStartPos;
            update();
            return;
        }

        // 扣除死区后的有效距离和向量
        qreal effectiveDistance = rawDistance - minDrag;
        QPointF effectiveVector = rawVector * (effectiveDistance / rawDistance);

        m_rawDragVector = effectiveVector;

        // 钳制鼠标显示位置到maxDrag以内
        if (effectiveDistance > maxDrag) {
            qreal ratio = maxDrag / effectiveDistance;
            m_currentMousePos = m_dragStartPos + effectiveVector * ratio;
        } else {
            m_currentMousePos = m_dragStartPos + effectiveVector;
        }

        update();
    }
}

// =============================================================================
// 鼠标释放事件：根据拖拽向量计算豹豹发射参数
//
// 发射公式：
//   effectiveDistance = min(dragLength, maxDrag) 即已钳制的距离
//   ratio = effectiveDistance / maxDrag           即拖拽力度 0~1
//   speed = ratio × 20                            像素/帧（最大初速20）
//   velocityF = dir × speed                      速度向量
//   totalDistance = effectiveDistance² / 12.5    行动总距离（距离与拖拽长度的平方成正比）
//
// 这样拖得越远 → 速度越大，总距离越长（平方关系），手感更好
// =============================================================================
void GameWidget::mouseReleaseEvent(QMouseEvent *)
{
    if (!m_isDragging || m_draggedIndex == -1) return;

    QPointF dragVector = m_rawDragVector;
    qreal dragDistance = qSqrt(dragVector.x() * dragVector.x() + dragVector.y() * dragVector.y());
    qreal maxDrag = 200;

    // 拖拽距离太短 → 取消发射
    if (dragDistance < 1) {
        m_isDragging = false;
        m_draggedIndex = -1;
        m_rawDragVector = QPointF(0, 0);
        return;
    }

    // 钳制有效距离并计算发射参数
    qreal effectiveDistance = qMin(dragDistance, maxDrag);
    qreal ratio = effectiveDistance / maxDrag;
    qreal speed = ratio * 20;  // 基础速度 = 拖拽力度 × 20

    // 发射方向 = 拖拽方向的反方向（向后拖 → 向前发射，弹弓原理）
    QPointF dir(-dragVector.x() / dragDistance, -dragVector.y() / dragDistance);
    QPointF velocityF = dir * speed;

    // 设置豹豹的运动状态
    m_baobaos[m_draggedIndex].velocityF = velocityF;
    m_baobaos[m_draggedIndex].remainingDistance = effectiveDistance * effectiveDistance / 12.5;
    m_baobaos[m_draggedIndex].totalDistance = m_baobaos[m_draggedIndex].remainingDistance;
    m_baobaos[m_draggedIndex].isMoving = true;
    m_baobaos[m_draggedIndex].hasActed = true;

    // 发射音效已暂时停用

    // ---- 嘭嘭海豹技能：高压洒水车 ------------------------------------------
    // 行动时随机发射2个水弹攻击敌方豹豹，每个造成4点伤害
    if (m_baobaos[m_draggedIndex].type == BaoBaoType::Pengpeng) {
        // 确定敌方阵营索引列表
        bool isRedTeam = (m_draggedIndex < 3);
        QList<int> enemyIndices;
        for (int i = 0; i < 6; i++) {
            if (i != m_draggedIndex && ((i < 3) != isRedTeam)) {
                enemyIndices.append(i);
            }
        }
        // 随机选择1~2个敌人造成伤害
        if (enemyIndices.size() >= 1) {
            int firstTarget = enemyIndices[rand() % enemyIndices.size()];
            m_baobaos[firstTarget].hp -= 4;
            if (enemyIndices.size() >= 2) {
                int secondTarget = firstTarget;
                while (secondTarget == firstTarget) {
                    secondTarget = enemyIndices[rand() % enemyIndices.size()];
                }
                m_baobaos[secondTarget].hp -= 4;
            }
        }
    }

    m_isDragging = false;
    m_draggedIndex = -1;
    m_rawDragVector = QPointF(0, 0);
    update();
}

// 返回按钮：弹出确认框（确认后emit goToSelectWidget跳转到选择界面）
void GameWidget::onBackButtonClicked()
{
    QMessageBox msgBox(this);
    msgBox.setWindowTitle("返回选择");
    msgBox.setText("确定要返回选择界面吗？\n当前对局进度将会丢失。");
    msgBox.setIcon(QMessageBox::Question);
    msgBox.setStandardButtons(QMessageBox::Yes | QMessageBox::No);
    msgBox.setDefaultButton(QMessageBox::No);
    msgBox.button(QMessageBox::Yes)->setText("确定返回");
    msgBox.button(QMessageBox::No)->setText("继续游戏");

    if (msgBox.exec() == QMessageBox::Yes) {
        emit goToSelectWidget();
    }
}

// =============================================================================
// 定时器事件：每16ms触发一次物理更新（≈62.5fps）
// =============================================================================
void GameWidget::timerEvent(QTimerEvent *)
{
    updatePhysics();
}

// =============================================================================
// 核心物理更新循环（每16ms执行一次）
//
// 执行流程：
//   第1步：更新装饰旋转 + 碰撞自转衰减
//   第2步：遍历所有isMoving豹豹 → 边界碰撞检测 + 位置更新 + 缓动计算
//   第3步：handleCollisions() 检测豹豹间碰撞（伤害/技能/物理反弹/音效）
//   第4步：轮次切换判定（当前阵营全部停止 → 交换出手权 或 下一轮）
//   第5步：屏幕震动衰减 + 边界安全网 + 刷新所有显示
//
// 轮次规则：
//   - 当前阵营的豹豹全部停止移动后 → 交换order
//   - 如果6个豹豹全部已行动 → 重置hasActed → 进入下一轮
//   - 每次轮次切换重置绷带豹豹buff和所有豹豹ATK
// =============================================================================
void GameWidget::updatePhysics()
{
    // 游戏已结束 → 停止所有物理模拟（防止定时器继续运行干扰页面跳转）
    if (m_gameOver) return;

    // ====== 第1步：更新旋转动画 + 碰撞自转衰减 ======
    for (int i = 0; i < 6; i++) {
        m_baobaos[i].decorationRotation += 1.0;      // 待机圆环每帧旋转1度
        if (m_baobaos[i].decorationRotation >= 360)
            m_baobaos[i].decorationRotation -= 360;

        // 碰撞自转：角度累加后速度衰减8%
        if (m_baobaos[i].spinVelocity > 0.05) {
            m_baobaos[i].spinAngle += m_baobaos[i].spinVelocity;
            m_baobaos[i].spinVelocity *= 0.92;       // 每帧衰减8%
        } else {
            m_baobaos[i].spinVelocity = 0;            // 衰减到阈值以下 → 停止自转
        }
    }

    bool currentTurnMoving = false;  // 当前行动方是否有豹豹在移动（用于轮次判定）
    bool hasMovingBaoBao = false;    // 是否有任何豹豹在移动（用于装饰显隐）

    // ====== 第2步：移动所有运动中的豹豹 ======
    for (int i = 0; i < 6; i++)
    {
        m_baobaos[i].justCollided = false;  // 清除上一帧的碰撞标记
        auto &bao = m_baobaos[i];

        if (!bao.isMoving || bao.remainingDistance <= 0) continue;

        currentTurnMoving = true;
        hasMovingBaoBao = true;

        // 计算本帧基础移动距离
        qreal moveDistance = qSqrt(bao.velocityF.x() * bao.velocityF.x() + bao.velocityF.y() * bao.velocityF.y());

        if (moveDistance < 0.001) {  // 速度几乎为零 → 停止
            bao.isMoving = false;
            continue;
        }

        // 如果剩余距离不够本帧完整移动 → 按比例缩减速度
        if (moveDistance > bao.remainingDistance) {
            qreal scale = bao.remainingDistance / moveDistance;
            bao.velocityF *= scale;
            moveDistance = bao.remainingDistance;
        }

        // 非线性缓动：根据运动进度调整速度倍率（四段缓动）
        qreal progress = 1.0 - bao.remainingDistance / bao.totalDistance;
        qreal easingMult = getEasingMultiplier(progress);
        qreal easedMove = moveDistance * easingMult;
        QPointF easedVelocity = bao.velocityF * easingMult;

        // 更新位置（浮点精度，避免累积误差）
        bao.center += easedVelocity;
        bao.remainingDistance -= easedMove;

        // ---- 梯形边界碰撞检测 --------------------------------------------
        int radius = 30;
        bool bounced = false;

        for (int e = 0; e < 4; e++) {
            QPointF v0 = m_boundaryVerts[e];
            QPointF v1 = m_boundaryVerts[(e + 1) % 4];
            QPointF edgeDir = v1 - v0;
            qreal edgeLenSq = QPointF::dotProduct(edgeDir, edgeDir);
            if (edgeLenSq < 0.001) continue;

            // 投影到边上求最近点
            qreal t = QPointF::dotProduct(bao.center - v0, edgeDir) / edgeLenSq;
            QPointF closest;
            if (t <= 0) closest = v0;
            else if (t >= 1) closest = v1;
            else closest = v0 + edgeDir * t;

            QPointF delta = bao.center - closest;
            qreal dist = std::hypot(delta.x(), delta.y());

            // 圆心穿透了边界 → 推出 + 镜面反射速度
            if (dist < radius && dist > 0.001) {
                QPointF normal = delta / dist;          // 从边界指向圆心的法线
                bao.center = closest + normal * radius; // 推到刚好接触

                qreal vn = QPointF::dotProduct(bao.velocityF, normal);
                if (vn < 0) {  // 速度指向边界内部 → 需要反射
                    // 镜面反射: v' = v - 2*(v·n)*n
                    bao.velocityF = bao.velocityF - normal * (2 * vn);
                }
                bounced = true;
            }
        }

        // 边界碰撞后损耗剩余距离（模拟与墙壁的摩擦）
        if (bounced) {
            bao.remainingDistance *= 0.8;  // 损耗20%
        }

        // 行动值耗尽 → 停止
        if (bao.remainingDistance < 0.5)
        {
            bao.isMoving = false;
            bao.velocityF = QPointF(0, 0);
            bao.collisionCount = 0;  // 停止时重置碰撞次数
        }

        // 更新碰撞箱矩形位置（从圆心推导）
        bao.collisionRect.moveCenter(bao.center.toPoint());
        bao.label->move(bao.collisionRect.x(), bao.collisionRect.y());
    }

    // ====== 第3步：豹豹间碰撞检测与处理 ======
    handleCollisions();

    // ====== 第4步：轮次切换判定 ======
    static bool lastFrameHadMovement = false;  // 上一帧是否有运动（跨帧状态）

    if (!currentTurnMoving) {
        // 当前阵营所有豹豹已停止 → 如果上一帧还在运动，这一帧刚停
        if (lastFrameHadMovement) {
            // 检查6个豹豹是否全部已行动
            bool allActed = true;
            for (int i = 0; i < 6; i++) {
                if (!m_baobaos[i].hasActed) {
                    allActed = false;
                    break;
                }
            }

            if (allActed) {
                // ---- 全部出手完毕 → 进入下一轮 ---------------------------------
                order = true;
                m_roundNumber++;
                m_roundLabel->setText(QString("第%1轮").arg(m_roundNumber));
                m_roundLabel->adjustSize();
                m_roundLabel->move(803 - m_roundLabel->width() / 2, 28);

                // 重置所有豹豹的出手状态
                for (int i = 0; i < 6; i++) {
                    m_baobaos[i].hasActed = false;
                }
                // 重置阵营为初始顺序（红色方先手）
                for (int i = 0; i < 3; i++) {
                    m_baobaos[i].camp = order;
                }
                for (int i = 3; i < 6; i++) {
                    m_baobaos[i].camp = !order;
                }

                // 重置ATK和绷带技能buff
                for (int i = 0; i < 6; i++) {
                    m_baobaos[i].atk = getBaoBaoStats(m_baobaos[i].type).atk;
                    if (m_baobaos[i].type == BaoBaoType::Bengdai) m_baobaos[i].bengdaiBuffed = false;
                    refreshBaoBaoLabels(i);
                }
            } else {
                // ---- 当前阵营出手完毕但对方还有未出手 → 交换出手权 ------------
                order = !order;
                for (int i = 0; i < 3; i++) {
                    m_baobaos[i].camp = order;
                }
                for (int i = 3; i < 6; i++) {
                    m_baobaos[i].camp = !order;
                }
            }

            lastFrameHadMovement = false;
            update();
        }
    } else {
        lastFrameHadMovement = true;
    }

    // ====== 第5步：屏幕震动衰减 ======
    if (m_shakeFrames > 0) {
        m_shakeFrames--;
        qreal decay = qreal(m_shakeFrames) / qMax(1, m_shakeFrames + 1);
        m_shakeIntensity *= decay;
        // 每帧生成(-1~1)随机偏移，乘以当前强度
        m_shakeOffset = QPointF(
            (QRandomGenerator::global()->bounded(200) - 100) / 100.0 * m_shakeIntensity,
            (QRandomGenerator::global()->bounded(200) - 100) / 100.0 * m_shakeIntensity);
    } else {
        m_shakeOffset = QPointF(0, 0);
    }

    // 边界安全网：防止任何豹豹被推出梯形区域后卡死在外面
    clampAllToBoundary();

    // ====== 刷新所有豹豹的显示位置和标签 ======
    for (int i = 0; i < 6; i++) {
        auto &bao = m_baobaos[i];

        bao.collisionRect.moveCenter(bao.center.toPoint());
        bao.label->move(bao.collisionRect.x(), bao.collisionRect.y());

        refreshBaoBaoLabels(i);
    }

    m_hasMovingBaoBao = hasMovingBaoBao;

    update();  // 触发paintEvent刷新画面
}

// =============================================================================
// 豹豹间碰撞检测与处理（C(6,2)=15对遍历）
//
// 处理流程（对每个碰撞对）：
//   1. 圆形碰撞检测（distance < 60, 半径30+30）
//   2. 位置修正：推开重叠的豹豹，并钳制到梯形边界内
//   3. 伤害判定：只有主动方（移动方）才能造成伤害
//   4. 技能效果：每种豹豹类型有独特碰撞技能
//      - 橡胶海豹：弹簧助推器 → 碰撞+2ATK
//      - 天使海豹：按摩擒拿手 → 碰队友回复HP
//      - 大力海豹：友情接力棒 → 碰队友+3ATK
//      - 红温海豹：急性高血压 → 200%伤害+立即停下
//      - 绷带海豹：防御包扎 → 低血量+10ATK（在refreshBaoBaoLabels中处理）
//      - 嘭嘭海豹：高压洒水车 → 发射时触发（在mouseReleaseEvent中处理）
//   5. 物理反弹：主动方镜面反射+速度/行动值衰减
//   6. 被动方弹开：获得主动方30%的速度和剩余距离
//   7. 死亡判定：HP<1→复活至初始位置+死亡数+1
//   8. 游戏结束：任一阵营死亡≥6→emit goToResultWidget
// =============================================================================
void GameWidget::handleCollisions()
{
    for (int i = 0; i < 6; i++) {
        for (int j = i + 1; j < 6; j++) {
            auto &baoA = m_baobaos[i];
            auto &baoB = m_baobaos[j];

            // 两个都静止 → 跳过
            if (!baoA.isMoving && !baoB.isMoving) continue;

            // ---- 圆形碰撞检测 -------------------------------------------------
            QPointF delta = baoA.center - baoB.center;
            qreal distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
            qreal minDistance = 60.0;  // 半径之和（30+30）

            if (distance < minDistance) {
                // 防止同一对在同一帧被重复处理
                if (baoA.justCollided && baoB.justCollided) continue;

                baoA.justCollided = true;
                baoB.justCollided = true;

                // ---- 屏幕震动：强度正比于相对速度 ---------------------------------
                qreal relSpeed = std::sqrt(
                    (baoA.velocityF.x() - baoB.velocityF.x()) * (baoA.velocityF.x() - baoB.velocityF.x()) +
                    (baoA.velocityF.y() - baoB.velocityF.y()) * (baoA.velocityF.y() - baoB.velocityF.y()));
                triggerShake(qMin(relSpeed * 0.8, 15.0), 6);  // 强度上限15

                // ---- 碰撞自转：冲击越大转越快 ---------------------------------
                qreal spinBoost = qMin(relSpeed * 1.2, 25.0);  // 转速上限25°/帧
                if (baoA.isMoving) baoA.spinVelocity = qMax(baoA.spinVelocity, spinBoost);
                if (baoB.isMoving) baoB.spinVelocity = qMax(baoB.spinVelocity, spinBoost);

                // 播放碰撞音效（跨线程，不阻塞主线程）
                playCollisionSound();

                // 碰撞次数记录
                if (baoA.isMoving) baoA.collisionCount++;
                if (baoB.isMoving) baoB.collisionCount++;

                // ---- 位置修正 -------------------------------------------------------
                qreal overlap = minDistance - distance;

                // 计算碰撞法线（从B指向A）
                QPointF normal;
                if (distance < 0.001) {
                    normal = QPointF(1, 0);  // 防止两圆重合时除零
                } else {
                    normal = delta / distance;
                }

                // 切线方向（垂直于法线，用于反射计算）
                QPointF tangent(-normal.y(), normal.x());

                // 各推一半重叠距离
                QPointF correction = normal * (overlap / 2.0);
                if (baoA.isMoving) {
                    baoA.center += correction;
                }
                if (baoB.isMoving) {
                    baoB.center -= correction;
                }

                // 边界约束：防止碰撞修正把豹豹推出梯形区域
                auto clampOne = [&](QPointF& c) {
                    for (int e = 0; e < 4; e++) {
                        qreal sd = (c.x() - m_boundaryVerts[e].x()) * m_boundaryInNormals[e].x()
                                 + (c.y() - m_boundaryVerts[e].y()) * m_boundaryInNormals[e].y();
                        if (sd < 30.0) {
                            c.setX(c.x() + m_boundaryInNormals[e].x() * (30.0 - sd));
                            c.setY(c.y() + m_boundaryInNormals[e].y() * (30.0 - sd));
                        }
                    }
                };
                clampOne(baoA.center);
                clampOne(baoB.center);

                // ---- 伤害判定：只有移动方才能造成伤害（防止静止重叠反复扣血）----
                if(baoA.camp == true && baoB.camp == false && baoA.isMoving)
                {
                    baoB.hp -= baoA.atk;
                }
                else if(baoB.camp == true && baoA.camp == false && baoB.isMoving)
                {
                    baoA.hp -= baoB.atk;
                }

                // ---- 技能效果处理 -------------------------------------------------
                // 确定主动方和被动方（用于技能逻辑）
                BaoBaoObject* activeSkill = nullptr;
                BaoBaoObject* passiveSkill = nullptr;
                if (baoA.isMoving && !baoB.isMoving) {
                    activeSkill = &baoA;  passiveSkill = &baoB;
                } else if (!baoA.isMoving && baoB.isMoving) {
                    activeSkill = &baoB;  passiveSkill = &baoA;
                } else if (baoA.isMoving && baoB.isMoving) {
                    // 两个都在移动 → 速度大的为主动
                    qreal speedA = std::sqrt(baoA.velocityF.x() * baoA.velocityF.x() +
                                             baoA.velocityF.y() * baoA.velocityF.y());
                    qreal speedB = std::sqrt(baoB.velocityF.x() * baoB.velocityF.x() +
                                             baoB.velocityF.y() * baoB.velocityF.y());
                    if (speedA >= speedB) {
                        activeSkill = &baoA;  passiveSkill = &baoB;
                    } else {
                        activeSkill = &baoB;  passiveSkill = &baoA;
                    }
                }

                // 橡胶海豹：弹簧助推器 — 每次碰撞后提升2点攻击力
                if (activeSkill && activeSkill->type == BaoBaoType::Xiangjiao) {
                    activeSkill->atk += 2;
                }

                // 天使海豹：按摩擒拿手 — 碰到己方队友时回复（数值=攻击力）的血量
                if (activeSkill && passiveSkill &&
                    activeSkill->type == BaoBaoType::Tianshi &&
                    activeSkill->camp == passiveSkill->camp) {
                    passiveSkill->hp += activeSkill->atk;
                }

                // 大力海豹：友情接力棒 — 碰到己方队友时提升3点攻击力
                if (activeSkill && passiveSkill &&
                    activeSkill->type == BaoBaoType::Dali &&
                    activeSkill->camp == passiveSkill->camp) {
                    passiveSkill->atk += 3;
                }

                // 红温海豹：急性高血压 — 碰到首个敌方豹豹造成200%伤害并立刻停下
                // 额外检查camp==true确保只有当前行动方才触发技能
                if (activeSkill && passiveSkill &&
                    activeSkill->type == BaoBaoType::Hongwen &&
                    activeSkill->camp == true &&
                    activeSkill->camp != passiveSkill->camp) {
                    passiveSkill->hp -= activeSkill->atk * 2;
                    // 强制推开防止重叠 → 后续帧不会重复判定
                    QPointF sepDelta = passiveSkill->center - activeSkill->center;
                    qreal sepDist = std::hypot(sepDelta.x(), sepDelta.y());
                    if (sepDist < 60.0 && sepDist > 0.001) {
                        QPointF sepNormal = sepDelta / sepDist;
                        qreal sepOverlap = 60.0 - sepDist;
                        passiveSkill->center += sepNormal * sepOverlap;
                    }
                    activeSkill->remainingDistance = 0;
                    activeSkill->isMoving = false;
                    activeSkill->velocityF = QPointF(0, 0);
                }

                // ---- 确定主动方和被动方（物理反弹用）------------------------------
                BaoBaoObject* active = nullptr;   // 主动碰撞方
                BaoBaoObject* passive = nullptr;  // 被动被撞方

                if (baoA.isMoving && !baoB.isMoving) {
                    active = &baoA;  passive = &baoB;
                } else if (!baoA.isMoving && baoB.isMoving) {
                    active = &baoB;  passive = &baoA;
                } else if (baoA.isMoving && baoB.isMoving) {
                    qreal speedA = std::sqrt(baoA.velocityF.x() * baoA.velocityF.x() +
                                             baoA.velocityF.y() * baoA.velocityF.y());
                    qreal speedB = std::sqrt(baoB.velocityF.x() * baoB.velocityF.x() +
                                             baoB.velocityF.y() * baoB.velocityF.y());
                    if (speedA >= speedB) {
                        active = &baoA;  passive = &baoB;
                    } else {
                        active = &baoB;  passive = &baoA;
                    }
                }

                // ---- 镜面反射物理（只改变主动方速度方向）-------------------
                if (active != nullptr) {
                    QPointF activeVelocity = active->velocityF;

                    // 分解速度到法线方向和切线方向
                    qreal vn = activeVelocity.x() * normal.x() + activeVelocity.y() * normal.y();
                    qreal vt = activeVelocity.x() * tangent.x() + activeVelocity.y() * tangent.y();

                    // 镜面反射：法线分量取反，切线分量不变
                    QPointF newVelocity = tangent * vt - normal * vn;

                    active->velocityF = newVelocity;

                    // 主动方衰减：速度-15%，行动距离-10%
                    active->velocityF *= 0.85;
                    active->remainingDistance *= 0.9;

                    // ---- 被动方弹开（模拟崩铁原版的碰撞弹开效果）-----------
                    if (passive != nullptr && !passive->isMoving) {
                        // 被动方静止状态下：获得主动方30%的速度和行动距离
                        passive->remainingDistance = active->remainingDistance * 0.30;
                        passive->isMoving = true;

                        QPointF activeDirection = active->velocityF;
                        qreal activeSpeed = std::sqrt(activeVelocity.x() * activeVelocity.x() +
                                                      activeVelocity.y() * activeVelocity.y());
                        if (activeSpeed > 0.01) {
                            activeDirection /= activeSpeed;  // 归一化方向
                        }
                        // 沿主动方运动方向的反方向弹出
                        passive->velocityF = -activeDirection * (activeSpeed * 0.30);
                        passive->collisionCount = 0;
                    } else if (passive != nullptr && passive->isMoving) {
                        // 被动方也在移动：法线分量取绝对值（防止穿模），轻微衰减
                        qreal passiveVn = passive->velocityF.x() * normal.x() + passive->velocityF.y() * normal.y();
                        qreal passiveVt = passive->velocityF.x() * tangent.x() + passive->velocityF.y() * tangent.y();
                        QPointF newPassiveVelocity = tangent * passiveVt + normal * std::abs(passiveVn);
                        passive->velocityF = newPassiveVelocity * 0.95;
                        passive->remainingDistance *= 0.95;
                    }
                } else {
                    // 没有明确主动方（两个都静止但重叠 → 各自弹开）
                    if (baoA.isMoving) {
                        QPointF newVel = baoA.velocityF - normal * (baoA.velocityF.x() * normal.x() + baoA.velocityF.y() * normal.y()) * 2;
                        baoA.velocityF = newVel * 0.9;
                        baoA.remainingDistance *= 0.9;
                    }
                    if (baoB.isMoving) {
                        QPointF newVel = baoB.velocityF - normal * (baoB.velocityF.x() * normal.x() + baoB.velocityF.y() * normal.y()) * 2;
                        baoB.velocityF = newVel * 0.9;
                        baoB.remainingDistance *= 0.9;
                    }
                }
            } else {
                // 没有碰撞 → 清除碰撞标记
                baoA.justCollided = false;
                baoB.justCollided = false;
            }

            // ---- 死亡判定：HP归零 → 复活至初始位置 ------------------------
            if(baoA.hp<1)
            {
                if (i < 3) m_redDeaths++; else m_blueDeaths++;
                updateSlLabels();  // 更新SL得分标签
                updateSrLabels();  // 更新SR得分标签
                if (m_redDeaths >= 6) { m_gameOver = true; emit goToResultWidget(false); return; }
                if (m_blueDeaths >= 6) { m_gameOver = true; emit goToResultWidget(true); return; }
                GameWidget::resetBaoBaoState(i);
            }
            if(baoB.hp<1)
            {
                if (j < 3) m_redDeaths++; else m_blueDeaths++;
                updateSlLabels();  // 更新SL得分标签
                updateSrLabels();  // 更新SR得分标签
                if (m_redDeaths >= 6) { m_gameOver = true; emit goToResultWidget(false); return; }
                if (m_blueDeaths >= 6) { m_gameOver = true; emit goToResultWidget(true); return; }
                GameWidget::resetBaoBaoState(j);
            }
        }
    }
}

// =============================================================================
// 渲染回调：绘制游戏画面的所有图层
//
// 绘制顺序（从底到顶）：
//   1. 背景图（应用屏幕震动偏移）
//   2. 梯形碰撞边界（白色外框 + 蓝色半透明填充）
//   3. 拖拽预览线（白色，带反射预测）
//   4. 6个豹豹碰撞箱（红/蓝轮廓 + 自转旋转 + 剩余距离文本）
//   5. 待机旋转装饰（绿色双环+箭头，仅未出手的当前方可见）
// =============================================================================
void GameWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 应用屏幕震动偏移（整个画面一起震动）
    painter.save();
    painter.translate(m_shakeOffset);

    painter.drawPixmap(0, 0, m_bgPixmap);

    // ---- 绘制梯形碰撞区域（调试/视觉参考）---------------------------------
    QPolygonF boundaryPoly;
    for (int i = 0; i < 4; i++) boundaryPoly << m_boundaryVerts[i];
    QPen rectPen(Qt::white);
    rectPen.setWidth(4);
    painter.setPen(rectPen);
    painter.setBrush(QBrush(QColor(0, 0, 255, 20)));
    painter.drawPolygon(boundaryPoly);

    // ---- 绘制拖拽预览线（弹弓发射方向预测）------------------------------
    if (m_isDragging && m_draggedIndex != -1) {
        QPointF center = m_baobaos[m_draggedIndex].center;

        // 白色预览线：显示带反射的预测弹道
        if (m_rawDragVector.x() != 0 || m_rawDragVector.y() != 0) {
            QPointF dragVector = m_rawDragVector;
            qreal dragLength = std::hypot(dragVector.x(), dragVector.y());
            qreal maxDrag = 200;

            qreal effectiveDistance = qMin(dragLength, maxDrag);
            qreal ratio = effectiveDistance * effectiveDistance / 12.5;

            QPointF direction = dragVector / dragLength;
            QPointF launchDir = -direction;  // 发射方向 = 拖拽的反方向

            // 计算预测弹道（含反射）
            QVector<QPointF> path = calculateReflectionPath(center, launchDir, ratio);

            QPen redPen(Qt::white);
            redPen.setWidth(3);
            painter.setPen(redPen);

            for (int i = 0; i < path.size() - 1; ++i) {
                painter.drawLine(path[i].toPoint(), path[i + 1].toPoint());
            }
        }
    }

    // ---- 绘制6个豹豹的碰撞箱（带自转效果）-------------------------------
    for (int i = 0; i < 6; i++) {
        painter.save();

        QPointF c = m_baobaos[i].center;
        painter.translate(c);
        // 如果有自转速度 → 旋转整个碰撞箱视觉
        if (m_baobaos[i].spinVelocity > 0.05)
            painter.rotate(m_baobaos[i].spinAngle);

        // 红色方 → 红色轮廓，蓝色方 → 蓝色轮廓
        if (i < 3) {
            QPen pen(Qt::red); pen.setWidth(8); painter.setPen(pen);
        } else {
            QPen pen(QColor(30, 144, 255)); pen.setWidth(10); painter.setPen(pen);
        }

        painter.setBrush(QBrush(QColor(255, 255, 0, 50)));
        painter.drawEllipse(QPointF(0, 0), 30, 30);

        // 移动中的豹豹显示剩余行动距离
        if (m_baobaos[i].isMoving) {
            painter.setPen(Qt::blue);
            painter.drawText(QPointF(0, 0),
                             QString::number(m_baobaos[i].remainingDistance, 'f', 1));
        }
        painter.restore();
    }

    // ---- 绘制待机旋转装饰（仅在无豹豹移动时显示）-------------------------
    // 只有当前行动方、且未出手的豹豹显示旋转装饰
    if (!m_hasMovingBaoBao) {
        for (int i = 0; i < 6; i++) {
            bool isCurrentTurn = (order && i < 3) || (!order && i > 2);
            if (isCurrentTurn && !m_baobaos[i].hasActed) {
                drawRotatingDecoration(painter, m_baobaos[i]);
            }
        }
    }

    painter.restore();
}

// =============================================================================
// 射线与线段交点检测（2D叉积法）
// 射线：P = rayStart + t * rayDir, t > 0
// 线段：Q = segA + u * segDir, u ∈ [0, 1]
// 联立求解 t 和 u，ret t = 射线参数
// =============================================================================
bool GameWidget::intersectWithSegment(const QPointF& rayStart, const QPointF& rayDir, const QPointF& segA, const QPointF& segB, qreal& t)
{
    QPointF segDir = segB - segA;
    qreal denom = rayDir.x() * segDir.y() - rayDir.y() * segDir.x();   // rayDir × segDir
    if (qAbs(denom) < 0.0001) return false;

    QPointF d = rayStart - segA;                                        // = -(segA - rayStart)
    qreal tRay = -(d.x() * segDir.y() - d.y() * segDir.x()) / denom;   // (segA - rayStart)×segDir / (rayDir×segDir)
    qreal tSeg = -(d.x() * rayDir.y() - d.y() * rayDir.x()) / denom;   // (segA - rayStart)×rayDir / (rayDir×segDir)

    if (tRay > 0 && tSeg >= 0 && tSeg <= 1) {
        t = tRay;
        return true;
    }
    return false;
}

// =============================================================================
// 射线与AABB矩形交点：分别检查四条边，取最近的交点
// 用于拖拽预览中的简单矩形碰撞（此处已不再使用，保留备用）
// =============================================================================
QPointF GameWidget::intersectWithRect(const QPointF& start, const QPointF& dir, const QRect& rect)
{
    qreal t = -1;

    // 检查四条边的交点
    // 左边
    if (dir.x() < 0) {
        qreal tx = (rect.left() - start.x()) / dir.x();
        qreal y = start.y() + tx * dir.y();
        if (tx > 0 && y >= rect.top() && y <= rect.bottom()) {
            if (t < 0 || tx < t) t = tx;
        }
    }
    // 右边
    if (dir.x() > 0) {
        qreal tx = (rect.right() - start.x()) / dir.x();
        qreal y = start.y() + tx * dir.y();
        if (tx > 0 && y >= rect.top() && y <= rect.bottom()) {
            if (t < 0 || tx < t) t = tx;
        }
    }
    // 上边
    if (dir.y() < 0) {
        qreal ty = (rect.top() - start.y()) / dir.y();
        qreal x = start.x() + ty * dir.x();
        if (ty > 0 && x >= rect.left() && x <= rect.right()) {
            if (t < 0 || ty < t) t = ty;
        }
    }
    // 下边
    if (dir.y() > 0) {
        qreal ty = (rect.bottom() - start.y()) / dir.y();
        qreal x = start.x() + ty * dir.x();
        if (ty > 0 && x >= rect.left() && x <= rect.right()) {
            if (t < 0 || ty < t) t = ty;
        }
    }

    if (t >= 0) {
        return start + dir * t;
    }
    return start + dir; // 没有交点，返回终点方向
}

// =============================================================================
// 射线与圆的交点检测（一元二次方程求解）
// 方程：|rayStart + t*rayDir - circleCenter|² = radius²
// 展开为 at² + bt + c = 0
// 返回最小正数解t
// =============================================================================
bool GameWidget::intersectWithCircle(const QPointF& rayStart, const QPointF& rayDir, const QPointF& circleCenter, qreal radius, qreal& t)
{
    // 射线: P = rayStart + t * rayDir (t > 0)
    // 圆: |P - circleCenter|^2 = radius^2

    QPointF oc = rayStart - circleCenter;
    qreal a = QPointF::dotProduct(rayDir, rayDir);
    qreal b = 2.0 * QPointF::dotProduct(oc, rayDir);
    qreal c = QPointF::dotProduct(oc, oc) - radius * radius;

    qreal discriminant = b * b - 4 * a * c;

    if (discriminant < 0) return false;

    qreal sqrtD = std::sqrt(discriminant);
    qreal t1 = (-b - sqrtD) / (2 * a);
    qreal t2 = (-b + sqrtD) / (2 * a);

    // 取最小的正数解
    if (t1 > 0.001) {
        t = t1;
        return true;
    }
    if (t2 > 0.001) {
        t = t2;
        return true;
    }

    return false;
}

// =============================================================================
// 拖拽预览的反射路径计算（射线追踪法）
//
// 算法：从起点沿方向发射射线，寻找最近的碰撞点（边界或豹豹）
//       到达碰撞点后计算镜面反射方向，继续追踪
//       最多追踪2段（初始段+1次反射）
//
// 边界检测：每条边向内偏移半径30 → 偏移后线段与射线求交
// 豹豹检测：圆心半径=60（双倍半径，因为两个豹豹的半径叠加）→ 圆-射线求交
// =============================================================================
QVector<QPointF> GameWidget::calculateReflectionPath(const QPointF& start, const QPointF& direction, qreal maxLength)
{
    QVector<QPointF> points;
    points.append(start);

    qreal radius = 30.0;
    QPointF currentPos = start;              // 射线从中心点开始，代表豹豹中心点的真实运动轨迹
    QPointF currentDir = direction;
    qreal remainingLength = maxLength;
    int maxReflections = 2;  // 仅反弹一次（初始段+一段反射）

    for (int reflection = 0; reflection < maxReflections && remainingLength > 0; ++reflection) {
        qreal minT = remainingLength;
        QPointF hitPoint;
        int hitType = -1;  // 0=边界, 1=豹豹
        int hitIndex = -1;

        // 1. 检查与梯形边界的交点（每条边向内偏移半径）
        for (int e = 0; e < 4; e++) {
            QPointF v0 = m_boundaryVerts[e];
            QPointF v1 = m_boundaryVerts[(e + 1) % 4];
            QPointF edgeDir = v1 - v0;
            qreal edgeLen = std::hypot(edgeDir.x(), edgeDir.y());
            if (edgeLen < 0.001) continue;
            // 向内法线：顺时针顶点排列，法线指向左侧（多边形内部）
            QPointF inNormal(-edgeDir.y() / edgeLen, edgeDir.x() / edgeLen);
            QPointF ov0 = v0 + inNormal * radius;
            QPointF ov1 = v1 + inNormal * radius;

            qreal tEdge;
            if (intersectWithSegment(currentPos, currentDir, ov0, ov1, tEdge)) {
                if (tEdge > 0.001 && tEdge < minT) {
                    minT = tEdge;
                    hitPoint = currentPos + currentDir * tEdge;
                    hitType = 0;
                }
            }
        }

        // 2. 检查与其他豹豹的交点（排除自己）
        for (int i = 0; i < m_baobaos.size(); ++i) {
            if (i == m_draggedIndex) continue;  // 跳过自己

            qreal tCircle;
            if (intersectWithCircle(currentPos, currentDir, m_baobaos[i].center, 60, tCircle)) {
                if (tCircle > 0.001 && tCircle < minT) {
                    minT = tCircle;
                    hitPoint = currentPos + currentDir * tCircle;
                    hitType = 1;
                    hitIndex = i;
                }
            }
        }

        if (minT >= remainingLength - 0.001) {
            // 没有碰撞或碰撞点在剩余长度之外
            QPointF end = currentPos + currentDir * remainingLength;
            points.append(end);
            break;
        }

        // 记录碰撞点
        points.append(hitPoint);
        remainingLength -= minT;

        // 根据碰撞类型计算反射方向
        if (hitType == 0) {
            // 边界反射 - 找到被击中的边并计算反射法线
            QPointF normal;
            qreal minEdgeDist = 1e9;
            for (int e = 0; e < 4; e++) {
                QPointF v0 = m_boundaryVerts[e];
                QPointF v1 = m_boundaryVerts[(e + 1) % 4];
                QPointF edgeDir = v1 - v0;
                qreal edgeLenSq = QPointF::dotProduct(edgeDir, edgeDir);
                qreal t = (edgeLenSq > 0.001) ? QPointF::dotProduct(hitPoint - v0, edgeDir) / edgeLenSq : 0;
                t = qBound(0.0, t, 1.0);
                QPointF closest = v0 + edgeDir * t;
                qreal dist = std::hypot(hitPoint.x() - closest.x(), hitPoint.y() - closest.y());
                if (dist < minEdgeDist) {
                    minEdgeDist = dist;
                    normal = QPointF(-edgeDir.y(), edgeDir.x());
                    qreal nLen = std::hypot(normal.x(), normal.y());
                    if (nLen > 0.001) normal /= nLen;
                }
            }

            // 反射公式: R = V - 2*(V·N)*N
            qreal dot = currentDir.x() * normal.x() + currentDir.y() * normal.y();
            currentDir = currentDir - normal * (2 * dot);

            // 归一化保持方向稳定
            qreal len = std::hypot(currentDir.x(), currentDir.y());
            if (len > 0.001) {
                currentDir /= len;
            }
            remainingLength *= 0.8;
        }
        else if (hitType == 1) {
            // 豹豹反射（弹性碰撞）
            QPointF normal = (hitPoint - m_baobaos[hitIndex].center);
            qreal normLen = std::hypot(normal.x(), normal.y());
            if (normLen > 0.001) {
                normal /= normLen;
            }

            // 反射公式: R = V - 2*(V·N)*N
            qreal dot = currentDir.x() * normal.x() + currentDir.y() * normal.y();
            currentDir = currentDir - normal * (2 * dot);

            remainingLength *= 0.9;
        }

        // 从碰撞点稍微偏移，避免卡在碰撞点上
        currentPos = hitPoint + currentDir * 0.1;
    }

    return points;
}

// =============================================================================
// 重置指定豹豹到初始状态（死亡后复活）
// 操作：恢复默认位置 + 清空物理状态 + 重置HP/ATK + 清除显示缓存
//       然后与所有其他豹豹做一次碰撞检测防止与现有点重叠
// =============================================================================
void GameWidget::resetBaoBaoState(int index)
{
    if (index < 0 || index >= m_baobaos.size()) return;

    auto &bao = m_baobaos[index];

    // 根据阵营和索引重置初始位置（百分比坐标）
        if (index == 0) bao.center = QPointF(70.99 / 100.0 * 1600, 32.96 / 100.0 * 900);
        else if (index == 1) bao.center = QPointF(66.56 / 100.0 * 1600, 48.33 / 100.0 * 900);
        else if (index == 2) bao.center = QPointF(73.44 / 100.0 * 1600, 65.28 / 100.0 * 900);
        else if (index == 3) bao.center = QPointF(29.64 / 100.0 * 1600, 33.15 / 100.0 * 900);
        else if (index == 4) bao.center = QPointF(34.22 / 100.0 * 1600, 48.15 / 100.0 * 900);
        else if (index == 5) bao.center = QPointF(27.03 / 100.0 * 1600, 65.28 / 100.0 * 900);


    // 重置物理状态
    bao.velocityF = QPointF(0, 0);
    bao.remainingDistance = 0;
    bao.totalDistance = 0;        // 重置缓动总距离
    bao.isMoving = false;
    bao.justCollided = false;
    bao.collisionCount = 0;
    bao.decorationRotation = 0;
    bao.spinAngle = 0;
    bao.spinVelocity = 0;
    bao.bengdaiBuffed = false;
    // 根据豹豹类型重置血量和攻击力
    BaoBaoStats stats = getBaoBaoStats(bao.type);
    bao.hp = stats.hp;
    bao.atk = stats.atk;
    // 清空显示缓存，强制下一帧刷新标签
    bao.lastDispHp = -1;
    bao.lastDispAtk = -1;
    bao.lastHpFontSize = 0;
    bao.lastAtkFontSize = 0;
    bao.lastLabelPos = QPoint(-999, -999);  // 强制move

    // 更新碰撞箱位置
    bao.collisionRect.moveCenter(bao.center.toPoint());

    // 更新显示标签
    if (bao.label) {
        bao.label->move(bao.collisionRect.x(), bao.collisionRect.y());
    }

    // 刷新标签显示
    refreshBaoBaoLabels(index);

    for(int i=0; i<index; i++)
    {
        collisionDetection(m_baobaos[index], m_baobaos[i]);
    }

    for(int i=index+1; i<6; i++)
    {
        collisionDetection(m_baobaos[index], m_baobaos[i]);
    }

}


// =============================================================================
// 简化的碰撞检测（用于复活后排查重叠）
// 仅做位置修正（各推一半），不处理物理反弹、伤害和技能
// =============================================================================
bool GameWidget::collisionDetection(BaoBaoObject &baoA ,BaoBaoObject &baoB)
{
    // 计算两个圆心之间的距离
    QPointF delta = baoA.center - baoB.center;
    qreal distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    qreal minDistance = 60.0;  // 半径之和 (30 + 30)

    // 发生碰撞
    if (distance < minDistance)
    {
        // 防止同一帧内重复处理
        if (baoA.justCollided && baoB.justCollided) return false;

        // 标记已碰撞
        baoA.justCollided = true;
        baoB.justCollided = true;

        //计算重叠深度
        qreal overlap = minDistance - distance;

        // 归一化碰撞法线方向（从B指向A）
        QPointF normal;
        if (distance < 0.001)
        {
            normal = QPointF(1, 0);  // 防止除零
        }
        else
        {
            normal = delta / distance;
        }

        // 切线方向（垂直于法线）
        QPointF tangent(-normal.y(), normal.x());

        // ========== 1. 位置修正：推开两个豹豹 ==========
        QPointF correction = normal * (overlap / 2.0);
        baoA.center += correction;
        baoB.center -= correction;

    }
    return true;
}


// =============================================================================
// 绘制待机旋转装饰：两个对称绿色半圆环 + 四个箭头
// 实现"未出手的当前方可行动"的视觉提示
// 装饰围绕豹豹圆心旋转，旋转角每帧+1（updatePhysics中更新）
// =============================================================================
void GameWidget::drawRotatingDecoration(QPainter& painter, BaoBaoObject& bao) {
    painter.save();
    QPointF center = bao.center;
    qreal innerRadius = 35;
    qreal outerRadius = 45;
    qreal ringWidth = outerRadius - innerRadius;  // 环宽 = 10px

    // 先平移到豹豹圆心，旋转，再平移回来（绕圆心旋转）
    painter.translate(center);
    painter.rotate(bao.decorationRotation);
    painter.translate(-center);

    // ---- 两个半圆环（绿色，150透明度）--------------------------------------
    QColor ringColor(0, 255, 0, 150);
    QPen ringPen(ringColor);
    ringPen.setWidth(ringWidth);
    ringPen.setCapStyle(Qt::FlatCap);
    painter.setPen(ringPen);
    painter.setBrush(Qt::NoBrush);

    QRectF arcRect(center.x() - outerRadius, center.y() - outerRadius,
                   outerRadius * 2, outerRadius * 2);

    // 上半圆弧 10°→170°（160°跨度）
    int start1 = 10 * 16;
    int span1 = 160 * 16;
    painter.drawArc(arcRect, start1, span1);

    // 下半圆弧 190°→350°（160°跨度）
    int start2 = 190 * 16;
    int span2 = 160 * 16;
    painter.drawArc(arcRect, start2, span2);

    // ---- 四个箭头（在弧的两端，指示旋转方向）-------------------------------
    QPen arrowPen(QColor(0, 220, 0, 200));
    arrowPen.setWidth(3);
    painter.setPen(arrowPen);
    painter.setBrush(QColor(0, 220, 0, 180));

    auto drawArrow = [&](qreal angleDeg) {
        qreal rad = qDegreesToRadians(angleDeg);
        // 箭头底端在外圆弧上
        QPointF arrowBase = center + QPointF(qCos(rad) * outerRadius,
                                             qSin(rad) * outerRadius);
        // 箭头沿切线方向（逆时针）
        qreal tangentRad = rad + M_PI / 2;
        QPointF tangentDir(qCos(tangentRad), qSin(tangentRad));
        qreal arrowLen = 12;
        QPointF arrowTip = arrowBase + tangentDir * arrowLen;
        painter.drawLine(arrowBase, arrowTip);

        // 三角形箭头头部
        qreal headLen = 6;
        qreal headAngle1 = tangentRad + M_PI * 0.8;
        qreal headAngle2 = tangentRad - M_PI * 0.8;
        QPainterPath arrowHead;
        arrowHead.moveTo(arrowTip);
        arrowHead.lineTo(arrowTip + QPointF(qCos(headAngle1) * headLen,
                                            qSin(headAngle1) * headLen));
        arrowHead.lineTo(arrowTip + QPointF(qCos(headAngle2) * headLen,
                                            qSin(headAngle2) * headLen));
        arrowHead.closeSubpath();
        painter.drawPath(arrowHead);
    };

    drawArrow(170);   // 上半弧右端
    drawArrow(350);   // 下半弧右端

    painter.restore();
}

// =============================================================================
// 刷新指定豹豹的HP/ATK显示标签（带缓存优化）
//
// 性能优化策略（避免每帧不必要的QPaint操作）：
//   1. 位置缓存：仅center变化导致标签位置改变时才调用move()
//   2. 样式缓存：仅字号变化时才调用setStyleSheet()
//   3. 文本缓存：仅数值变化时才调用setText()
//
// 动态效果：
//   - HP<10时 → 红色18px大字
//   - HP>基础值(增益) → 绿色18px大字
//   - ATK>基础值 → 绿色16px大字
// =============================================================================
void GameWidget::refreshBaoBaoLabels(int index)
{
    auto& bao = m_baobaos[index];
    if (!bao.hpLabel || !bao.atkLabel) return;

    // ---- 绷带海豹：防御包扎 — HP低于25时ATK+10（一次性） -----------------
    if (bao.type == BaoBaoType::Bengdai) {
        if (bao.hp < 25 && !bao.bengdaiBuffed) {
            bao.atk += 10; bao.bengdaiBuffed = true;
        } else if (bao.hp >= 25 && bao.bengdaiBuffed) {
            bao.atk -= 10; bao.bengdaiBuffed = false;  // HP恢复后撤销
        }
    }

    // 标签位置：HP在碰撞箱右下角外侧（+8, +56），ATK在HP下方（+72）
    int newHpX = bao.collisionRect.x() + 8;
    int newHpY = bao.collisionRect.y() + 56;
    if (newHpX != bao.lastLabelPos.x() || newHpY != bao.lastLabelPos.y()) {
        bao.hpLabel->move(newHpX, newHpY);
        bao.atkLabel->move(newHpX, bao.collisionRect.y() + 72);
        bao.lastLabelPos = QPoint(newHpX, newHpY);
    }

    BaoBaoStats defStats = getBaoBaoStats(bao.type);

    // 根据数值状态确定字号
    int hpFontSize = 14;
    if (bao.hp < 10) hpFontSize = 18;           // 危险血量 → 大字
    else if (bao.hp > defStats.hp) hpFontSize = 18;  // 增益血量 → 大字

    int atkFontSize = 12;
    if (bao.atk > defStats.atk) atkFontSize = 16;    // 增益攻 → 大字

    // 仅字号变化时才更新styleSheet（缓存优化）
    if (hpFontSize != bao.lastHpFontSize) {
        bao.hpLabel->setStyleSheet(
            QString("QLabel { color: %1; font-size: %2px; font-weight: bold;"
                    " background-color: transparent; border-radius: 10px; padding: 2px 5px; }")
                .arg(bao.hp < 10 ? "#ff4444" : (bao.hp > defStats.hp ? "#00ff00" : "white"))
                .arg(hpFontSize));
        bao.lastHpFontSize = hpFontSize;
    }

    if (atkFontSize != bao.lastAtkFontSize) {
        bao.atkLabel->setStyleSheet(
            QString("QLabel { color: %1; font-size: %2px; font-weight: bold;"
                    " background-color: transparent; border-radius: 8px; padding: 2px 5px; }")
                .arg(bao.atk > defStats.atk ? "#00ff00" : "white")
                .arg(atkFontSize));
        bao.lastAtkFontSize = atkFontSize;
    }

    // 仅数值变化时才更新text（缓存优化）
    if (bao.hp != bao.lastDispHp) {
        bao.hpLabel->setText(QString::number(bao.hp));
        bao.hpLabel->adjustSize();
        bao.lastDispHp = bao.hp;
    }

    if (bao.atk != bao.lastDispAtk) {
        bao.atkLabel->setText(QString("攻击力: %1").arg(bao.atk));
        bao.atkLabel->adjustSize();
        bao.lastDispAtk = bao.atk;
    }
}

// =============================================================================
// 从SelectWidget接收选中的豹豹类型并初始化游戏
// 不同于resetGame，这是首次进入游戏时的初始化
//
// 操作：
//   1. 为每个豹豹设置类型、HP/ATK、圆形头像图片
//   2. 重置所有位置/速度/缓存到初始状态
//   3. 重置轮次、死亡数、得分标签
//   4. 刷新头像标签（从预裁剪数组中选取对应类型）
// =============================================================================
void GameWidget::setSelectedTypes(const QList<BaoBaoType>& p1Types, const QList<BaoBaoType>& p2Types) {
    // 重置所有豹豹到默认位置（百分比坐标）
    QPointF defaultPositions[6] = {
        QPointF(70.99 / 100.0 * 1600, 32.96 / 100.0 * 900),   // 豹豹1
        QPointF(66.56 / 100.0 * 1600, 48.33 / 100.0 * 900),   // 豹豹2
        QPointF(73.44 / 100.0 * 1600, 65.28 / 100.0 * 900),   // 豹豹3
        QPointF(29.64 / 100.0 * 1600, 33.15 / 100.0 * 900),   // 豹豹4
        QPointF(34.22 / 100.0 * 1600, 48.15 / 100.0 * 900),   // 豹豹5
        QPointF(27.03 / 100.0 * 1600, 65.28 / 100.0 * 900),   // 豹豹6
    };
    
    for (int i = 0; i < 3; i++) {
        if (i < p1Types.size() && i < m_baobaos.size()) {
            m_baobaos[i].type = p1Types[i];
            BaoBaoStats stats = getBaoBaoStats(p1Types[i]);
            m_baobaos[i].hp = stats.hp;
            m_baobaos[i].atk = stats.atk;
            m_baobaos[i].lastDispHp = -1;   // 清空缓存强制刷新
            m_baobaos[i].lastDispAtk = -1;
            m_baobaos[i].lastHpFontSize = 0;
            m_baobaos[i].lastAtkFontSize = 0;
            m_baobaos[i].lastLabelPos = QPoint(-999, -999);  // 强制move
            m_baobaos[i].bengdaiBuffed = false;
            m_baobaos[i].center = defaultPositions[i];
            m_baobaos[i].velocityF = QPointF(0, 0);
            m_baobaos[i].remainingDistance = 0;
            m_baobaos[i].totalDistance = 0;
            m_baobaos[i].isMoving = false;
            m_baobaos[i].hasActed = false;
            
            QString imagePath = getBaoBaoImagePath(p1Types[i]);
            QPixmap originalPixmap(imagePath);
            QPixmap circlePixmap(60, 60);
            circlePixmap.fill(Qt::transparent);
            QPainter painter(&circlePixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addEllipse(0, 0, 60, 60);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, 60, 60, originalPixmap);
            painter.end();
            
            if (m_baobaos[i].label) {
                m_baobaos[i].label->setPixmap(circlePixmap);
            }
        }
    }
    for (int i = 0; i < 3; i++) {
        if (i < p2Types.size() && (i + 3) < m_baobaos.size()) {
            m_baobaos[i + 3].type = p2Types[i];
            BaoBaoStats stats = getBaoBaoStats(p2Types[i]);
            m_baobaos[i + 3].hp = stats.hp;
            m_baobaos[i + 3].atk = stats.atk;
            m_baobaos[i + 3].lastDispHp = -1;   // 清空缓存强制刷新
            m_baobaos[i + 3].lastDispAtk = -1;
            m_baobaos[i + 3].lastHpFontSize = 0;
            m_baobaos[i + 3].lastAtkFontSize = 0;
            m_baobaos[i + 3].lastLabelPos = QPoint(-999, -999);  // 强制move
            m_baobaos[i + 3].bengdaiBuffed = false;
            m_baobaos[i + 3].center = defaultPositions[i + 3];
            m_baobaos[i + 3].velocityF = QPointF(0, 0);
            m_baobaos[i + 3].remainingDistance = 0;
            m_baobaos[i + 3].totalDistance = 0;
            m_baobaos[i + 3].isMoving = false;
            m_baobaos[i + 3].hasActed = false;
            
            QString imagePath = getBaoBaoImagePath(p2Types[i]);
            QPixmap originalPixmap(imagePath);
            QPixmap circlePixmap(60, 60);
            circlePixmap.fill(Qt::transparent);
            QPainter painter(&circlePixmap);
            painter.setRenderHint(QPainter::Antialiasing);
            QPainterPath path;
            path.addEllipse(0, 0, 60, 60);
            painter.setClipPath(path);
            painter.drawPixmap(0, 0, 60, 60, originalPixmap);
            painter.end();
            
            if (m_baobaos[i + 3].label) {
                m_baobaos[i + 3].label->setPixmap(circlePixmap);
            }
        }
    }
    for (int i = 0; i < m_baobaos.size(); i++) {
        auto& bao = m_baobaos[i];
        // 更新碰撞箱和标签位置
        bao.collisionRect.moveCenter(bao.center.toPoint());
        if (bao.label) {
            bao.label->move(bao.collisionRect.x(), bao.collisionRect.y());
        }
        refreshBaoBaoLabels(i);
    }
    m_redDeaths = 0;
    m_blueDeaths = 0;
    order = true;
    m_gameOver = false;  // 重置游戏结束标志
    m_roundNumber = 1;  // 重置轮次
    m_roundLabel->setText("第1轮");
    m_roundLabel->adjustSize();
    m_roundLabel->move(803 - m_roundLabel->width() / 2, 28);
    m_p1Types = p1Types;
    m_p2Types = p2Types;

    updateSlLabels();  // 重置SL得分标签
    updateSrLabels();  // 重置SR得分标签

    for (int i = 0; i < 3 && i < p1Types.size(); i++) {
        int idx = getTouxiangIndex(p1Types[i]);
        if (idx >= 0 && idx < 6 && m_touxiangLabels[i]) {
            m_touxiangLabels[i]->setPixmap(m_touxiangPixmaps[idx]);
            m_touxiangLabels[i]->show();
        }
    }
    for (int i = 0; i < 3 && i < p2Types.size(); i++) {
        int idx = getTouxiangIndex(p2Types[i]);
        if (idx >= 0 && idx < 6 && m_touxiangLabels2[i]) {
            m_touxiangLabels2[i]->setPixmap(m_touxiangPixmaps2[idx]);
            m_touxiangLabels2[i]->show();
        }
    }
    updateSlLabels();  // 重置SL得分标签
    updateSrLabels();  // 重置SR得分标签
}

// =============================================================================
// 重新开始游戏（"再来一局"按钮触发，使用已保存的类型选择）
// 相比setSelectedTypes，不再修改豹豹类型，只重置状态
// =============================================================================
void GameWidget::resetGame()
{
    m_redDeaths = 0;
    m_blueDeaths = 0;
    order = true;
    m_gameOver = false;  // 重置游戏结束标志
    m_roundNumber = 1;  // 重置轮次
    m_roundLabel->setText("第1轮");
    m_roundLabel->adjustSize();
    m_roundLabel->move(803 - m_roundLabel->width() / 2, 28);

    for (int i = 0; i < 3 && i < m_p1Types.size(); i++) {
        m_baobaos[i].type = m_p1Types[i];
        BaoBaoStats stats = getBaoBaoStats(m_p1Types[i]);
        m_baobaos[i].hp = stats.hp;
        m_baobaos[i].atk = stats.atk;
        m_baobaos[i].lastDispHp = -1;   // 清空缓存强制刷新
        m_baobaos[i].lastDispAtk = -1;
        m_baobaos[i].lastHpFontSize = 0;
        m_baobaos[i].lastAtkFontSize = 0;
        m_baobaos[i].lastLabelPos = QPoint(-999, -999);
    }
    for (int i = 0; i < 3 && i < m_p2Types.size(); i++) {
        m_baobaos[i + 3].type = m_p2Types[i];
        BaoBaoStats stats = getBaoBaoStats(m_p2Types[i]);
        m_baobaos[i + 3].hp = stats.hp;
        m_baobaos[i + 3].atk = stats.atk;
        m_baobaos[i + 3].lastDispHp = -1;   // 清空缓存强制刷新
        m_baobaos[i + 3].lastDispAtk = -1;
        m_baobaos[i + 3].lastHpFontSize = 0;
        m_baobaos[i + 3].lastAtkFontSize = 0;
        m_baobaos[i + 3].lastLabelPos = QPoint(-999, -999);
    }

    for (int i = 0; i < 6; i++) {
        // 默认位置（百分比坐标）
        QPointF defaultPositions[6] = {
            QPointF(70.99 / 100.0 * 1600, 32.96 / 100.0 * 900),   // 豹豹1
            QPointF(66.56 / 100.0 * 1600, 48.33 / 100.0 * 900),   // 豹豹2
            QPointF(73.44 / 100.0 * 1600, 65.28 / 100.0 * 900),   // 豹豹3
            QPointF(29.64 / 100.0 * 1600, 33.15 / 100.0 * 900),   // 豹豹4
            QPointF(34.22 / 100.0 * 1600, 48.15 / 100.0 * 900),   // 豹豹5
            QPointF(27.03 / 100.0 * 1600, 65.28 / 100.0 * 900),   // 豹豹6
        };
        m_baobaos[i].center = defaultPositions[i];
        m_baobaos[i].velocityF = QPointF(0, 0);
        m_baobaos[i].remainingDistance = 0;
        m_baobaos[i].totalDistance = 0;
        m_baobaos[i].isMoving = false;
        m_baobaos[i].hasActed = false;
        m_baobaos[i].justCollided = false;
        m_baobaos[i].collisionCount = 0;
        m_baobaos[i].decorationRotation = 0;
        m_baobaos[i].bengdaiBuffed = false;
        m_baobaos[i].camp = (i < 3);

        m_baobaos[i].collisionRect.moveCenter(m_baobaos[i].center.toPoint());
        if (m_baobaos[i].label) {
            m_baobaos[i].label->move(m_baobaos[i].collisionRect.x(), m_baobaos[i].collisionRect.y());
        }
        refreshBaoBaoLabels(i);
    }

    for (int i = 0; i < 3 && i < m_p1Types.size(); i++) {
        QString imagePath = getBaoBaoImagePath(m_p1Types[i]);
        QPixmap originalPixmap(imagePath);
        QPixmap circlePixmap(60, 60);
        circlePixmap.fill(Qt::transparent);
        QPainter painter(&circlePixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addEllipse(0, 0, 60, 60);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, 60, 60, originalPixmap);
        painter.end();
        if (m_baobaos[i].label) {
            m_baobaos[i].label->setPixmap(circlePixmap);
        }
    }
    for (int i = 0; i < 3 && i < m_p2Types.size(); i++) {
        QString imagePath = getBaoBaoImagePath(m_p2Types[i]);
        QPixmap originalPixmap(imagePath);
        QPixmap circlePixmap(60, 60);
        circlePixmap.fill(Qt::transparent);
        QPainter painter(&circlePixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addEllipse(0, 0, 60, 60);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, 60, 60, originalPixmap);
        painter.end();
        if (m_baobaos[i + 3].label) {
            m_baobaos[i + 3].label->setPixmap(circlePixmap);
        }
    }
    for (int i = 0; i < 3 && i < m_p1Types.size(); i++) {
        int idx = getTouxiangIndex(m_p1Types[i]);
        if (idx >= 0 && idx < 6 && m_touxiangLabels[i]) {
            m_touxiangLabels[i]->setPixmap(m_touxiangPixmaps[idx]);
            m_touxiangLabels[i]->show();
        }
    }
    for (int i = 0; i < 3 && i < m_p2Types.size(); i++) {
        int idx = getTouxiangIndex(m_p2Types[i]);
        if (idx >= 0 && idx < 6 && m_touxiangLabels2[i]) {
            m_touxiangLabels2[i]->setPixmap(m_touxiangPixmaps2[idx]);
            m_touxiangLabels2[i]->show();
        }
    }
    updateSlLabels();  // 重置SL得分标签
    updateSrLabels();  // 重置SR得分标签
}

