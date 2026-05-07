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
    }
    void setSelectedTypes(const QList<BaoBaoType>& p1Types, const QList<BaoBaoType>& p2Types);
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
    bool m_isDragging = false;        // 是否正在拖动
    int m_draggedIndex = -1;          // 正在拖动第几个豹豹
    QPointF m_dragStartPos;           // 拖动开始位置（浮点）
    QPointF m_currentMousePos;        // 当前鼠标位置（浮点）
    QPointF m_rawDragVector;          // 【新增】存储原始拖拽向量（不受maxDrag限制）

    // 中心矩形
    QRect m_centerRect;

    // 是否有豹豹正在移动
    bool m_hasMovingBaoBao = false;

    void initBaobaos();               // 初始化6个豹豹
    void updatePhysics();             // 更新物理（移动+碰撞）
    void drawRotatingDecoration(QPainter& painter, BaoBaoObject& bao);  // 绘制旋转装饰

signals:
    void goToResultWidget();


};

#endif
