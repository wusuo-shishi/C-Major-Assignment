#ifndef GAMEWIDGET_H
#define GAMEWIDGET_H
#include <QWidget>
#include <QPushButton>
#include <QVBoxLayout>
#include <QLabel>
#include <QPixmap>
#include <QPainter>
#include <QPaintEvent>
#include <QVector>
#include <QMouseEvent>
#include <QPushButton>
#include "BaoBaoType.h"
#include <QThread>

// CollisionSoundPlayer — 碰撞音效播放器
// 核心设计：驻留在独立QThread中，使用MCI多通道轮转播放WAV音效
// - MCI通道数CHANNELS=4，支持同时播放4个碰撞音效（alias: cs_0~cs_3）
// - m_slot轮转索引确保多次音效请求不互相覆盖
// - 主线程通过QueuedConnection发射requestCollisionSound信号 → 瞬间返回，零阻塞

class CollisionSoundPlayer : public QObject {
    Q_OBJECT
public:
    void setPath(const std::wstring& path) { m_path = path; }
public slots:
    void init();  // 加载WAV文件并注册MCI别名（在音频线程中调用）
    void play();  // 轮转播放碰撞音效（在音频线程中调用）
private:
    std::wstring m_path;          // 碰撞音效WAV文件绝对路径
    int m_slot = 0;               // 当前轮转槽位（0~3循环）
    static constexpr int CHANNELS = 4;  // MCI通道数，支持4个同时播放
};

// BaoBaoObject — 单个豹豹实体的完整状态数据
// 每个豹豹包含：外观显示(QLabel)、物理状态(位置/速度/行动值)、
// 战斗属性(HP/ATK)、技能标记、碰撞自转、显示缓存(性能优化)

class BaoBaoObject {
public:
    // 碰撞标记
    bool justCollided = false;   // 防止同一帧内重复碰撞处理
    int collisionCount = 0;      // 本次行动中的累计碰撞次数（停止时清零）

    // 显示组件（QLabel）
    QLabel *label;           // 豹豹圆形头像图片
    QLabel *hpLabel;         // HP数值标签（显示在碰撞箱下方）
    QLabel *atkLabel;        // ATK数值标签（显示在HP下方）

    // 物理状态
    QRect collisionRect;     // 碰撞箱矩形（显示用，从center推导）
    QPointF center;          // 圆心坐标（浮点精度，物理计算核心）
    QPointF velocityF;       // 当前速度向量（像素/帧）
    qreal remainingDistance; // 剩余行动距离（控制何时停下）
    qreal totalDistance;     // 本次行动总距离（用于缓动进度计算 t = 1 - remaining/total）
    bool isMoving = false;   // 是否正在移动中
    bool hasActed = false;   // 本轮是否已出手（出手后不可再次拖动）

    // 战斗属性
    int hp;
    int atk;
    bool bengdaiBuffed = false;  // 绷带海豹的低血量加攻buff是否已触发

    // 显示缓存（性能优化：避免每帧重复setStyleSheet/setText/move）
    int lastDispHp = -1;         // 上次渲染的HP值（-1=强制刷新）
    int lastDispAtk = -1;        // 上次渲染的ATK值
    int lastHpFontSize = 0;      // 上次HP标签字号
    int lastAtkFontSize = 0;     // 上次ATK标签字号
    QPoint lastLabelPos;         // 上次HP标签位置（跳过冗余move调用）

    // 阵营与视觉
    bool camp;                    // 阵营：true=当前行动方(红色轮廓), false=等待方(蓝色轮廓)
    qreal decorationRotation = 0;  // 待机装饰圆环旋转角度（每帧+1度）
    qreal spinAngle = 0;           // 碰撞自转累计角度（度）
    qreal spinVelocity = 0;        // 自转角速度（度/帧），每帧衰减8%，<0.05时停止

    // 类型
    BaoBaoType type;         // 豹豹类型（决定HP/ATK/技能/图片）
};

//
// GameWidget — 游戏主界面
// 核心职责：
//   1. 物理模拟   — updatePhysics() 每16ms更新位置/速度/缓动
//   2. 拖拽发射   — mousePress→Move→Release 实现弹弓式拖拽
//   3. 碰撞检测   — handleCollisions() 处理豹豹间碰撞 + 伤害 + 技能
//   4. 边界约束   — 梯形区域碰撞反弹 + clampAllToBoundary() 安全网
//   5. 轮次管理   — order标志控制红/蓝方交替出手，全部出手后轮次+1
//   6. 音效系统   — CollisionSoundPlayer独立线程 + MCI多通道
//   7. 游戏结算   — 任一阵营死亡≥6时emit goToResultWidget
//
class GameWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);
    ~GameWidget() {
        // 先停止音频线程，防止"thread destroyed while running"
        if (m_soundThread) {
            m_soundThread->quit();
            m_soundThread->wait(3000);
        }
        for (auto &bao : m_baobaos) {
            delete bao.label;
            delete bao.hpLabel;
            delete bao.atkLabel;
        }
        for (int i = 0; i < 3; i++) {
            delete m_touxiangLabels[i];
            delete m_touxiangLabels2[i];
        }
        for (int i = 0; i < 6; i++) {
            delete m_slLabels[i];
            delete m_srLabels[i];
        }
        delete m_roundLabel;
    }
    void setSelectedTypes(const QList<BaoBaoType>& p1Types, const QList<BaoBaoType>& p2Types);
    void resetGame();
    void setMuted(bool muted) { m_muted = muted; }  // 设置静音状态（由MainWindow静音按钮调用）
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;    // 鼠标按下
    void mouseMoveEvent(QMouseEvent *event) override;     // 鼠标移动
    void mouseReleaseEvent(QMouseEvent *event) override;  // 鼠标释放
    void timerEvent(QTimerEvent *event) override;         // 定时器，用于动画
    bool collisionDetection(BaoBaoObject &A ,BaoBaoObject &B);

private:

    void resetBaoBaoState(int index);  // 重置指定豹豹的状态（死亡后复活至初始位置）

    // 计算反射路径（拖拽预览用）：从起点沿方向发射射线，考虑边界反射和豹豹碰撞
    QVector<QPointF> calculateReflectionPath(const QPointF& start, const QPointF& direction, qreal maxLength);

    // 射线与圆的交点检测（用于反射路径计算）
    bool intersectWithCircle(const QPointF& rayStart, const QPointF& rayDir, const QPointF& circleCenter, qreal radius, qreal& t);

    // 射线与矩形边界的交点（用于反射路径计算）
    QPointF intersectWithRect(const QPointF& start, const QPointF& dir, const QRect& rect);

    // 豹豹容器：索引0~2=红色方(右侧), 索引3~5=蓝色方(左侧)
    QVector<BaoBaoObject> m_baobaos;

    void handleCollisions();       // 处理豹豹之间的碰撞（伤害/技能/物理反弹）

    // 轮次与阵营管理
    bool order = true;               // 出手顺序：true=红色方(右侧)行动, false=蓝色方(左侧)行动
    int m_redDeaths = 0;             // 红色方累计死亡数（用于得分标签显示+游戏结束判定）
    int m_blueDeaths = 0;            // 蓝色方累计死亡数
    QList<BaoBaoType> m_p1Types;     // 保存1P(红色方)选择的3个豹豹类型
    QList<BaoBaoType> m_p2Types;     // 保存2P(蓝色方)选择的3个豹豹类型

    // 拖拽系统（弹弓式发射）
    bool m_isDragging = false;        // 是否正在拖拽
    int m_draggedIndex = -1;          // 正在拖拽第几个豹豹（0~5，-1=无效）
    QPointF m_dragStartPos;           // 拖拽起始位置（浮点，即豹豹圆心）
    QPointF m_currentMousePos;        // 当前鼠标位置（浮点，用于计算拖拽向量）
    QPointF m_rawDragVector;          // 原始拖拽向量（扣除minDrag之后，不超过maxDrag）

    // 梯形边界（碰撞区域）
    QPointF m_boundaryVerts[4];        // 0=左上 1=右上 2=右下 3=左下（顺时针排列）
    QPointF m_boundaryInNormals[4];    // 预计算的边内侧单位法线（指向多边形内部）
    qreal   m_boundaryInvLen[4];       // 预计算的边长倒数 1.0/edgeLen（优化std::hypot调用）

    // 射线与线段交点检测
    bool intersectWithSegment(const QPointF& rayStart, const QPointF& rayDir, const QPointF& segA, const QPointF& segB, qreal& t);

    bool m_hasMovingBaoBao = false;    // 是否有豹豹正在移动（控制装饰圆环显隐）

    QPixmap m_bgPixmap;               // 游戏背景图（1600×900）

    void initBaobaos();               // 初始化6个豹豹的显示组件和默认位置
    void initTouxiang();              // 预裁剪头像图片并创建1P/2P头像标签
    void updatePhysics();             // 核心物理循环：移动+边界碰撞+轮次切换+震动衰减
    void refreshBaoBaoLabels(int index);  // 刷新指定豹豹的HP/ATK标签（带显示缓存优化）
    void drawRotatingDecoration(QPainter& painter, BaoBaoObject& bao);  // 绘制待机旋转装饰（绿色双环+箭头）
    int getTouxiangIndex(BaoBaoType type);  // 获取类型对应的头像索引（0~5）

    // 头像标签（1P红色方，3个位置）
    QLabel *m_touxiangLabels[3];      // L1 L2 L3 头像标签（1P，位于界面左下区域）
    QLabel *m_touxiangLabels2[3];     // L4 L5 L6 头像标签（2P，位于界面右下区域）
    QPixmap m_touxiangPixmaps[6];     // 1P预裁剪头像：0=橡胶 1=天使 2=大力 3=红温 4=绷带 5=嘭嘭
    QPixmap m_touxiangPixmaps2[6];    // 2P预裁剪头像（从背景图不同区域提取）

    // SL得分标签（蓝色方击败红色方豹豹时显示）
    QLabel *m_slLabels[6];            // SL1~SL6，位于界面顶部左侧
    QPixmap m_slPixmap;               // SL图标的裁剪图像（29×29px）
    void initSlLabels();              // 初始化SL/SR得分标签布局
    void updateSlLabels();            // 根据红色方死亡数更新SL显示/隐藏

    // SR得分标签（红色方击败蓝色方豹豹时显示，与SL关于中轴线对称）
    QLabel *m_srLabels[6];            // SR1~SR6，位于界面顶部右侧
    QPixmap m_srPixmap;               // SR图标的裁剪图像
    void updateSrLabels();            // 根据蓝色方死亡数更新SR显示/隐藏

    // 屏幕震动
    QPointF m_shakeOffset;            // 当前帧的震动偏移量（加到painter.translate中）
    int m_shakeFrames = 0;            // 剩余震动帧数
    qreal m_shakeIntensity = 0;       // 当前震动强度（每帧衰减）
    void triggerShake(qreal intensity = 10.0, int duration = 8);  // 触发屏幕震动

    // 音效系统
    CollisionSoundPlayer *m_soundPlayer;  // 音频播放器（驻留独立QThread）
    QThread *m_soundThread;               // 音频线程（不与主线程竞争CPU）
    bool m_muted = false;                  // 静音状态：true时跳过碰撞音效
    void initSounds();                    // 创建音频线程 + 注册MCI通道 + 绑定信号槽
    void playCollisionSound();            // 发射跨线程信号（瞬间返回，不阻塞主线程）

    // 缓动函数
    qreal getEasingMultiplier(qreal t);   // 根据进度t(0→1)返回速度倍率（四段式缓动）

    // 边界安全网
    void clampAllToBoundary();            // 每帧强制将圆心钳制在梯形边界内侧（防飞出/卡死）

    QPushButton *m_backButton;      // 全透明返回按钮（左上角，点击弹出确认框）

    // 轮次显示
    int m_roundNumber = 1;          // 当前轮次（初始=第1轮）
    QLabel *m_roundLabel;            // "第n轮"标签，位置(50.21%, 3.15%)
    bool m_gameOver = false;         // 游戏是否已结束（阻止定时器继续运行物理模拟）

signals:
    void goToResultWidget(bool redWins);
    void goToSelectWidget();            // 返回选择界面
    void requestCollisionSound();       // 请求播放碰撞音效（跨线程信号）

private slots:
    void onBackButtonClicked();  // 返回按钮点击：弹确认框后返回


};

#endif
