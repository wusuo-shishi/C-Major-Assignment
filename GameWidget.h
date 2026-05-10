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
#include "BaoBaoType.h"

class BaoBaoObject {
public:
    bool justCollided = false;   // 防止同一帧内重复碰撞
    int collisionCount = 0;      // 记录碰撞次数（用于衰减）
    QLabel *label;           // 显示的图片
    QLabel *hpLabel;         // 血量显示标签
    QLabel *atkLabel;        // 攻击力显示标签
    QRect collisionRect;     // 碰撞箱（显示用）
    QPointF center;          // 圆心（浮点，物理计算用）
    QPointF velocityF;       // 速度向量
    qreal remainingDistance; // 剩余行动距离（浮点）
    bool isMoving = false;   // 是否正在移动
    bool hasActed = false;   // 是否已经行动过
    bool bengdaiBuffed = false;  // 绷带海豹是否已获得buff
    int hp;
    int atk;
    bool camp;               //阵营
    qreal decorationRotation = 0;  // 装饰圆环旋转角度
    BaoBaoType type;         // 豹豹类型
};

class GameWidget : public QWidget
{
    Q_OBJECT
public:
    explicit GameWidget(QWidget *parent = nullptr);
    ~GameWidget() {
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
    }
    void setSelectedTypes(const QList<BaoBaoType>& p1Types, const QList<BaoBaoType>& p2Types);
    void resetGame();
protected:
    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;    // 鼠标按下
    void mouseMoveEvent(QMouseEvent *event) override;     // 鼠标移动
    void mouseReleaseEvent(QMouseEvent *event) override;  // 鼠标释放
    void timerEvent(QTimerEvent *event) override;         // 定时器，用于动画
    bool collisionDetection(BaoBaoObject &A ,BaoBaoObject &B);

private:

    void resetBaoBaoState(int index);  // 重置指定豹豹的状态
    // 计算反射路径（包含边界和豹豹碰撞）
    QVector<QPointF> calculateReflectionPath(const QPointF& start, const QPointF& direction, qreal maxLength);

    // 射线与圆的交点检测
    bool intersectWithCircle(const QPointF& rayStart, const QPointF& rayDir, const QPointF& circleCenter, qreal radius, qreal& t);

    // 射线与矩形边界的交点
    QPointF intersectWithRect(const QPointF& start, const QPointF& dir, const QRect& rect);

    QVector<BaoBaoObject> m_baobaos;

    void handleCollisions();       // 处理豹豹之间的碰撞

    // 拖动相关
    bool hasrun = false;
    BaoBaoObject bao[6];
    bool order = true;               //出手顺序，ture时左方行动，false时右方行动
    int m_redDeaths = 0;             //红色方累计死亡数
    int m_blueDeaths = 0;            //蓝色方累计死亡数
    QList<BaoBaoType> m_p1Types;     //保存1P选择的类型
    QList<BaoBaoType> m_p2Types;     //保存2P选择的类型
    bool m_isDragging = false;        // 是否正在拖动
    int m_draggedIndex = -1;          // 正在拖动第几个豹豹
    QPointF m_dragStartPos;           // 拖动开始位置（浮点）
    QPointF m_currentMousePos;        // 当前鼠标位置（浮点）
    QPointF m_rawDragVector;          // 【新增】存储原始拖拽向量（不受maxDrag限制）

    // 碰撞区域（梯形，顺时针四个顶点）
    QPointF m_boundaryVerts[4];        // 0=左上 1=右上 2=右下 3=左下

    // 射线与线段交点检测
    bool intersectWithSegment(const QPointF& rayStart, const QPointF& rayDir, const QPointF& segA, const QPointF& segB, qreal& t);

    // 是否有豹豹正在移动
    bool m_hasMovingBaoBao = false;

    QPixmap m_bgPixmap;               // 背景图

    void initBaobaos();               // 初始化6个豹豹
    void initTouxiang();              // 预裁剪头像并创建标签
    void updatePhysics();             // 更新物理（移动+碰撞）
    void refreshBaoBaoLabels(int index);  // 刷新指定豹豹的标签位置和颜色
    void drawRotatingDecoration(QPainter& painter, BaoBaoObject& bao);  // 绘制旋转装饰
    int getTouxiangIndex(BaoBaoType type);  // 获取类型对应的头像索引

    QLabel *m_touxiangLabels[3];      // L1 L2 L3 头像标签 (1P)
    QLabel *m_touxiangLabels2[3];     // L4 L5 L6 头像标签 (2P)
    QPixmap m_touxiangPixmaps[6];     // 1P: 0=xiangjiao 1=tianshi 2=dali 3=hongwen 4=bengdai 5=pengpeng
    QPixmap m_touxiangPixmaps2[6];    // 2P: 同上，使用Y区域裁剪

    // SL得分标签（蓝色方每击败红色方一个豹豹，依次显现SL1~SL6）
    QLabel *m_slLabels[6];            // SL1 ~ SL6 得分标签
    QPixmap m_slPixmap;               // SL标签的裁剪图像
    void initSlLabels();              // 初始化SL得分标签
    void updateSlLabels();            // 根据红色方死亡数更新SL标签显示

    // SR得分标签（红色方每击败蓝色方一个豹豹，依次显现SR1~SR6，与SL关于中轴线对称）
    QLabel *m_srLabels[6];            // SR1 ~ SR6 得分标签
    QPixmap m_srPixmap;               // SR标签的裁剪图像（从原图对称位置提取）
    void updateSrLabels();            // 根据蓝色方死亡数更新SR标签显示

signals:
    void goToResultWidget(bool redWins);


};

#endif
