#include "GameWidget.h"
#include <QtMath>
#include <QDebug>
#include <QTimerEvent>
#include <QPainterPath>
#include <cmath>  // 用于fabs函数

GameWidget::GameWidget(QWidget *parent)
    : QWidget{parent}
{
    setMouseTracking(true);  // 开启鼠标跟踪，才能接收move事件

    QLabel *label = new QLabel("游戏界面", this);
    label->setAlignment(Qt::AlignCenter);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);

    // 初始化中心矩形（面积）
    m_centerRect = QRect(160, 100, 1200, 700);


    // 初始化6个豹豹
    initBaobaos();

    // 启动物理定时器（每16ms约60fps）
    startTimer(16);
}

// 初始化6个豹豹
void GameWidget::initBaobaos()
{
    QPixmap originalPixmap(":/images/start_page.jpg");

    if(!hasrun)
    {
        hasrun = true;
        for (int i = 0; i < 6; i++) {

            bao[i].hp = 45;
            bao[i].atk = 5;
        }
    }
    for (int i = 0; i < 6; i++) {
        //阵营设置
        if(i<3)bao[i].camp = order;
        qDebug("order切换");        //camp为true时，视为行动方阵营，不会因为被撞击而扣血
        if(i>2)bao[i].camp = !order;


        // 位置：横向排列（使用浮点）
        bao[i].center = QPointF(300 + i * 200, 450);
        bao[i].collisionRect = QRect(qRound(bao[i].center.x() - 40), qRound(bao[i].center.y() - 40), 80, 80);
        bao[i].velocityF = QPointF(0, 0);
        bao[i].decorationRotation = 0;
        bao[i].remainingDistance = 0;

        // 创建圆形图片
        QPixmap circlePixmap(80, 80);
        circlePixmap.fill(Qt::transparent);
        QPainter painter(&circlePixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addEllipse(0, 0, 80, 80);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, 80, 80, originalPixmap);
        painter.end();

        // 创建标签
        // ========== 血量显示标签 ==========
        bao[i].hpLabel = new QLabel(this);
        bao[i].hpLabel->setAlignment(Qt::AlignCenter);
        bao[i].hpLabel->setStyleSheet(
            "QLabel {"
            "   color: white;"
            "   font-size: 14px;"
            "   font-weight: bold;"
            "   background-color: rgba(0, 0, 0, 150);"
            "   border-radius: 10px;"
            "   padding: 2px 5px;"
            "}"
            );
        bao[i].hpLabel->setText(QString("%1").arg(bao[i].hp));
        bao[i].hpLabel->adjustSize();
        bao[i].hpLabel->move(bao[i].collisionRect.x() + 10, bao[i].collisionRect.y() + 85);
        bao[i].hpLabel->show();

        // ========== 攻击力显示标签 ==========
        bao[i].atkLabel = new QLabel(this);
        bao[i].atkLabel->setAlignment(Qt::AlignCenter);
        bao[i].atkLabel->setStyleSheet(
            "QLabel {"
            "   color: #FF6B6B;"
            "   font-size: 12px;"
            "   font-weight: bold;"
            "   background-color: rgba(0, 0, 0, 150);"
            "   border-radius: 8px;"
            "   padding: 2px 5px;"
            "}"
            );
        bao[i].atkLabel->setText(QString("攻击力: %1").arg(bao[i].atk));
        bao[i].atkLabel->adjustSize();
        bao[i].atkLabel->move(bao[i].collisionRect.x() + 10, bao[i].collisionRect.y() + 110);
        bao[i].atkLabel->show();

        //贴图显示标签
        bao[i].label = new QLabel(this);
        bao[i].label->setPixmap(circlePixmap);
        bao[i].label->setFixedSize(80, 80);
        bao[i].label->move(bao[i].collisionRect.x(), bao[i].collisionRect.y());
        bao[i].label->setAttribute(Qt::WA_TranslucentBackground);
        bao[i].label->show();

        m_baobaos.append(bao[i]);
    }
}

// 鼠标按下：检测点中了哪个豹豹
void GameWidget::mousePressEvent(QMouseEvent *event)
{
    QPointF clickPos = event->pos();  // 浮点坐标


    // 倒序检测（后画的在上面，优先检测）
    for (int i = 5; i >= 0; i--) {
        // 计算点击位置到圆心的距离
        QPointF delta = clickPos - m_baobaos[i].center;
        qreal distSq = delta.x() * delta.x() + delta.y() * delta.y();

        // 如果距离小于半径，说明点中了
        bool isCurrentTurn = (order && i < 3) || (!order && i > 2);
        if (!isCurrentTurn) continue;
        if (distSq <= 40 * 40) {
            if (m_baobaos[i].isMoving) {
                return;
            }
            m_isDragging = true;
            m_draggedIndex = i;
            m_dragStartPos = clickPos;
            m_currentMousePos = clickPos;
            m_rawDragVector = QPointF(0, 0);  // 初始化原始拖拽向量

            update();  // 触发重绘，显示拖动线
            return;
        }
    }
}

// 鼠标移动：更新拖动线
void GameWidget::mouseMoveEvent(QMouseEvent *event)
{
    if (m_isDragging) {
        // 【关键】始终基于原始起点计算拖拽向量，不修改 m_dragStartPos
        QPointF currentPos = event->pos();
        QPointF rawVector = currentPos - m_dragStartPos;
        qreal rawDistance = qSqrt(rawVector.x() * rawVector.x() + rawVector.y() * rawVector.y());

            qreal maxDrag = 200;

        // 存储原始拖拽向量（用于预览红线和弹射）
        m_rawDragVector = rawVector;

        if (rawDistance > maxDrag) {
            // 只限制鼠标显示位置，不改变拖拽向量的方向
            qreal ratio = maxDrag / rawDistance;
            m_currentMousePos = m_dragStartPos + QPointF(rawVector.x() * ratio, rawVector.y() * ratio);
        } else {
            m_currentMousePos = currentPos;
        }

        update();  // 重绘显示拖动线
    }
}

// 鼠标释放：计算弹射
void GameWidget::mouseReleaseEvent(QMouseEvent *)
{
    if (!m_isDragging || m_draggedIndex == -1) return;

    // 【关键】使用原始拖拽向量（m_rawDragVector）而不是被限制的 m_currentMousePos
    QPointF dragVector = m_rawDragVector;
    qreal dragDistance = qSqrt(dragVector.x() * dragVector.x() + dragVector.y() * dragVector.y());
    qreal maxDrag = 200;

    if (dragDistance < 1) {  // 防止除零
        m_isDragging = false;
        m_draggedIndex = -1;
        m_rawDragVector = QPointF(0, 0);
        return;
    }

    // 限制最大拖拽距离（力度封顶）
    qreal effectiveDistance = qMin(dragDistance, maxDrag);
    qreal ratio = effectiveDistance / maxDrag;
    qreal speed = ratio * 30;

    // 归一化方向向量并取反（反方向弹射）
    QPointF dir(-dragVector.x() / dragDistance, -dragVector.y() / dragDistance);
    QPointF velocityF = dir * speed;

    // 直接赋值浮点速度（不经过整数转换）
    m_baobaos[m_draggedIndex].velocityF = velocityF;
    m_baobaos[m_draggedIndex].remainingDistance = effectiveDistance * effectiveDistance  / 12.5;  // 行动距离=拖动平方除以12.5
    m_baobaos[m_draggedIndex].isMoving = true;

    // 重置拖动状态
    m_isDragging = false;
    m_draggedIndex = -1;
    m_rawDragVector = QPointF(0, 0);
    update();
}

// 定时器：更新物理
void GameWidget::timerEvent(QTimerEvent *)
{
    updatePhysics();
}

// 物理更新
void GameWidget::updatePhysics()
{
    // 更新装饰旋转动画
    for (int i = 0; i < 6; i++) {
        m_baobaos[i].decorationRotation += 1.0;
        if (m_baobaos[i].decorationRotation >= 360) {
            m_baobaos[i].decorationRotation -= 360;
        }
    }

    bool currentTurnMoving = false;  // 当前阵营是否有豹豹在移动
    bool hasMovingBaoBao = false;  // 是否有任何豹豹在移动

    for (int i = 0; i < 6; i++)
    {
        m_baobaos[i].justCollided = false;
        auto &bao = m_baobaos[i];

        if (!bao.isMoving || bao.remainingDistance <= 0) continue;

        currentTurnMoving = true;
        hasMovingBaoBao = true;

        // 计算移动距离
        qreal moveDistance = qSqrt(bao.velocityF.x() * bao.velocityF.x() + bao.velocityF.y() * bao.velocityF.y());

        if (moveDistance < 0.001) {  // 防止除零
            bao.isMoving = false;
            continue;
        }

        // 如果剩余距离不够，按比例缩减速度
        if (moveDistance > bao.remainingDistance) {
            qreal scale = bao.remainingDistance / moveDistance;
            bao.velocityF *= scale;
            moveDistance = bao.remainingDistance;
        }

        // 更新位置（浮点计算）
        bao.center += bao.velocityF;
        bao.remainingDistance -= moveDistance;

        // 边界碰撞检测
        int radius = 40;
        qreal left = m_centerRect.left() + radius;
        qreal right = m_centerRect.right() - radius;
        qreal top = m_centerRect.top() + radius;
        qreal bottom = m_centerRect.bottom() - radius;
        bool bounced = false;

        // 左/右边界
        if (bao.center.x() < left) {
            bao.center.setX(left);
            bao.velocityF.setX(-bao.velocityF.x());
            bounced = true;
        } else if (bao.center.x() > right) {
            bao.center.setX(right);
            bao.velocityF.setX(-bao.velocityF.x());
            bounced = true;
        }

        // 上/下边界
        if (bao.center.y() < top) {
            bao.center.setY(top);
            bao.velocityF.setY(-bao.velocityF.y());
            bounced = true;
        } else if (bao.center.y() > bottom) {
            bao.center.setY(bottom);
            bao.velocityF.setY(-bao.velocityF.y());
            bounced = true;
        }

        // 碰撞后损耗（可以调整系数）
        if (bounced) {
            bao.remainingDistance *= 0.8;  // 损耗20%
        }

        // 停止条件
        if (bao.remainingDistance < 0.5)
        {
            bao.isMoving = false;
            bao.velocityF = QPointF(0, 0);
            bao.collisionCount = 0;  // 停止时重置碰撞次数
        }

        // 更新显示（只在显示时转为整数）
        bao.collisionRect.moveCenter(bao.center.toPoint());
        bao.label->move(bao.collisionRect.x(), bao.collisionRect.y());
    }

    handleCollisions();

    // 使用 static 变量记录上一帧是否有移动
    static bool lastFrameHadMovement = false;

    if (!currentTurnMoving) {
        // 当前帧没有移动的豹豹
        if (lastFrameHadMovement) {
            // 上一帧有移动，这一帧没有了 → 切换阵营
            order = !order;

            //更新camp状态
            for(int i=0; i <3; i++)
            {
                m_baobaos[i].camp = order;
            }
            for(int i=3; i <6; i++)
            {
                m_baobaos[i].camp = !order;
            }

            lastFrameHadMovement = false;  // 重置标记
            update();
        }
    } else {
        // 当前帧有移动的豹豹
        lastFrameHadMovement = true;
    }


    // 更新所有豹豹的显示位置
    for (int i = 0; i < 6; i++) {
        auto &bao = m_baobaos[i];

        // 更新图片位置
        bao.collisionRect.moveCenter(bao.center.toPoint());
        bao.label->move(bao.collisionRect.x(), bao.collisionRect.y());

        // 更新血量标签位置
        if (bao.hpLabel) {
            bao.hpLabel->move(bao.collisionRect.x() + 10, bao.collisionRect.y() + 85);
            // 实时更新血量显示
            bao.hpLabel->setText(QString("%1").arg(bao.hp));
            bao.hpLabel->adjustSize();
        }

        // 更新攻击力标签位置
        if (bao.atkLabel) {
            bao.atkLabel->move(bao.collisionRect.x() + 10, bao.collisionRect.y() + 110);
            bao.atkLabel->adjustSize();
        }
    }
    // 更新是否有豹豹移动的状态
    m_hasMovingBaoBao = hasMovingBaoBao;

    // 刷新画面（始终刷新以显示旋转动画）
    update();
}

// 处理豹豹之间的碰撞
void GameWidget::handleCollisions()
{
    // 遍历所有豹豹对 (i, j)，其中 i < j
    for (int i = 0; i < 6; i++) {
        for (int j = i + 1; j < 6; j++) {
            auto &baoA = m_baobaos[i];
            auto &baoB = m_baobaos[j];

            // 两个豹豹都没有在移动，不需要处理碰撞
            if (!baoA.isMoving && !baoB.isMoving) continue;

            // 计算两个圆心之间的距离
            QPointF delta = baoA.center - baoB.center;
            qreal distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
            qreal minDistance = 80.0;  // 半径之和 (40 + 40)

            // 发生碰撞
            if (distance < minDistance) {
                // 防止同一帧内重复处理
                if (baoA.justCollided && baoB.justCollided) continue;

                // 标记已碰撞
                baoA.justCollided = true;
                baoB.justCollided = true;

                // 增加碰撞次数记录
                if (baoA.isMoving) baoA.collisionCount++;
                if (baoB.isMoving) baoB.collisionCount++;

                // 计算重叠深度
                qreal overlap = minDistance - distance;

                // 归一化碰撞法线方向（从B指向A）
                QPointF normal;
                if (distance < 0.001) {
                    normal = QPointF(1, 0);  // 防止除零
                } else {
                    normal = delta / distance;
                }

                // 切线方向（垂直于法线）
                QPointF tangent(-normal.y(), normal.x());

                // ========== 1. 位置修正：推开两个豹豹 ==========
                QPointF correction = normal * (overlap / 2.0);
                if (baoA.isMoving) {
                    baoA.center += correction;
                }
                if (baoB.isMoving) {
                    baoB.center -= correction;
                }

                if(baoA.camp == true && baoB.camp == false)
                {
                    baoB.hp -= baoA.atk;
                }
                else if(baoB.camp == true && baoA.camp == false)
                {
                    baoA.hp -= baoB.atk;
                }

                // ========== 2. 确定主动方和被动方 ==========
                BaoBaoObject* active = nullptr;   // 主动撞的豹豹
                BaoBaoObject* passive = nullptr;  // 被撞的豹豹


                if (baoA.isMoving && !baoB.isMoving) {
                    active = &baoA;
                    passive = &baoB;
                } else if (!baoA.isMoving && baoB.isMoving) {
                    active = &baoB;
                    passive = &baoA;
                } else if (baoA.isMoving && baoB.isMoving) {
                    // 两个都在移动：速度大的主动，速度小的被动
                    qreal speedA = std::sqrt(baoA.velocityF.x() * baoA.velocityF.x() +
                                             baoA.velocityF.y() * baoA.velocityF.y());
                    qreal speedB = std::sqrt(baoB.velocityF.x() * baoB.velocityF.x() +
                                             baoB.velocityF.y() * baoB.velocityF.y());
                    if (speedA >= speedB) {
                        active = &baoA;
                        passive = &baoB;
                    } else {
                        active = &baoB;
                        passive = &baoA;
                    }
                }

                // ========== 3. 计算镜面反射（只改变主动方的速度方向） ==========
                if (active != nullptr) {
                    QPointF activeVelocity = active->velocityF;

                    // 计算速度在法线方向的分量
                    qreal vn = activeVelocity.x() * normal.x() + activeVelocity.y() * normal.y();

                    // 计算速度在切线方向的分量
                    qreal vt = activeVelocity.x() * tangent.x() + activeVelocity.y() * tangent.y();

                    // 【镜面反射】法线方向取反，切线方向不变
                    QPointF newVelocity = tangent * vt - normal * vn;

                    // 应用新速度
                    active->velocityF = newVelocity;

                    // ========== 4. 速度衰减（主动方每次碰撞衰减15%） ==========
                    active->velocityF *= 0.85;

                    // ========== 5. 行动值衰减（主动方每次碰撞减少10%） ==========
                    active->remainingDistance *= 0.9;

                    // ========== 6. 被撞方获得少量行动值（被撞后弹开，获得移动能力） ==========
                    if (passive != nullptr && !passive->isMoving) {
                        // 被撞方获得行动值（主动方剩余行动值的20%）
                        passive->remainingDistance = active->remainingDistance * 0.2;
                        passive->isMoving = true;
                        // 被撞方获得初始速度（主动方速度的30%，沿法线方向弹出）
                        QPointF activeDirection = active->velocityF;
                        qreal activeSpeed = std::sqrt(activeVelocity.x() * activeVelocity.x() +
                                                      activeVelocity.y() * activeVelocity.y());
                        if (activeSpeed > 0.01) {
                            activeDirection /= activeSpeed;  // 归一化
                        }
                        passive->velocityF = -activeDirection * (activeSpeed * 0.3);
                        passive->collisionCount = 0;
                    } else if (passive != nullptr && passive->isMoving) {
                        // 被动方也在移动时，被动方也受到速度影响（较小）
                        QPointF passiveVelocity = passive->velocityF;
                        qreal passiveVn = passiveVelocity.x() * normal.x() + passiveVelocity.y() * normal.y();
                        qreal passiveVt = passiveVelocity.x() * tangent.x() + passiveVelocity.y() * tangent.y();
                        QPointF newPassiveVelocity = tangent * passiveVt + normal * std::abs(passiveVn);
                        passive->velocityF = newPassiveVelocity * 0.95;  // 轻微衰减
                        passive->remainingDistance *= 0.95;
                    }
                } else {
                    // 没有明确主动方的情况（两个都静止但重叠了，分别弹开）
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
                // 没有碰撞，清除碰撞标志（但保留碰撞次数用于调试，不重置）
                baoA.justCollided = false;
                baoB.justCollided = false;
            }
            if(baoA.hp<1)
            {
                GameWidget::resetBaoBaoState(i);
            }
            if(baoB.hp<1)
            {
                GameWidget::resetBaoBaoState(j);
            }
        }
    }
}

//预览方向绘制，碰撞箱处理
void GameWidget::paintEvent(QPaintEvent *event)
{
    QWidget::paintEvent(event);
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);

    // 绘制中心矩形
    QPen rectPen(Qt::white);
    rectPen.setWidth(4);
    painter.setPen(rectPen);
    painter.setBrush(QBrush(QColor(0, 0, 255, 20)));
    painter.drawRect(m_centerRect);

    // 绘制拖动预览（如果正在拖动）
    if (m_isDragging && m_draggedIndex != -1) {
        QPointF center = m_baobaos[m_draggedIndex].center;


        // 红线：弹射方向（带反射）
        if (m_rawDragVector.x() != 0 || m_rawDragVector.y() != 0) {
            QPointF dragVector = m_rawDragVector;
            qreal dragLength = std::hypot(dragVector.x(), dragVector.y());
            qreal maxDrag = 200;

            qreal effectiveDistance = qMin(dragLength, maxDrag);
            qreal ratio = effectiveDistance * effectiveDistance / 12.5;

            QPointF direction = dragVector / dragLength;
            QPointF launchDir = -direction;

            QVector<QPointF> path = calculateReflectionPath(center, launchDir, ratio);

            QPen redPen(Qt::white);
            redPen.setWidth(3);
            painter.setPen(redPen);

            for (int i = 0; i < path.size() - 1; ++i) {
                painter.drawLine(path[i].toPoint(), path[i + 1].toPoint());
            }

            //painter.setBrush(Qt::red);
            //painter.drawEllipse(path.last(), 5, 5);
        }
    }

    // 绘制6个豹豹的碰撞箱
    for (int i = 0; i < 6; i++) {
        if (i < 3) {
            QPen pen(Qt::red);
            pen.setWidth(4);
            painter.setPen(pen);
        } else {
            QPen pen(QColor(30, 144, 255));
            pen.setWidth(5);
            painter.setPen(pen);
        }

        painter.setBrush(QBrush(QColor(255, 255, 0, 50)));
        painter.drawEllipse(m_baobaos[i].collisionRect);

        if (m_baobaos[i].isMoving) {
            painter.setPen(Qt::blue);
            painter.drawText(m_baobaos[i].center.toPoint(),
                             QString::number(m_baobaos[i].remainingDistance, 'f', 1));
        }
    }

    // 绘制旋转装饰（只对当前行动方绘制，且没有豹豹在移动时）
    if (!m_hasMovingBaoBao) {
        for (int i = 0; i < 6; i++) {
            bool isCurrentTurn = (order && i < 3) || (!order && i > 2);
            if (isCurrentTurn) {
                drawRotatingDecoration(painter, m_baobaos[i]);
            }
        }
    }
}

// 射线与矩形边界的交点计算
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

// 射线与圆的交点检测
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

// 计算反射路径（包含边界和豹豹碰撞）
QVector<QPointF> GameWidget::calculateReflectionPath(const QPointF& start, const QPointF& direction, qreal maxLength)
{
    QVector<QPointF> points;
    points.append(start);

    qreal radius = 40.0;
    QPointF currentPos = start + direction * (radius + 1.0);
    QPointF currentDir = direction;
    qreal remainingLength = maxLength;
    int maxReflections = 5;  // 最多反射5次，防止无限循环

    for (int reflection = 0; reflection < maxReflections && remainingLength > 0; ++reflection) {
        qreal minT = remainingLength;
        QPointF hitPoint;
        int hitType = -1;  // 0=边界, 1=豹豹
        int hitIndex = -1;

        // 1. 检查与边界矩形的交点
        QRect boundRect = m_centerRect.adjusted(radius, radius, -radius, -radius);
        QPointF boundHit = intersectWithRect(currentPos, currentDir, boundRect);
        qreal tBound = QPointF::dotProduct(boundHit - currentPos, currentDir);
        if (tBound > 0.001 && tBound < minT) {
            minT = tBound;
            hitPoint = boundHit;
            hitType = 0;
        }

        // 2. 检查与其他豹豹的交点（排除自己）
        for (int i = 0; i < m_baobaos.size(); ++i) {
            if (i == m_draggedIndex) continue;  // 跳过自己

            qreal tCircle;
            if (intersectWithCircle(currentPos, currentDir, m_baobaos[i].center, 80, tCircle)) {
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
            // 边界反射 - 使用法线向量计算
            QPointF normal;

            if (qAbs(hitPoint.x() - boundRect.left()) < 1.0) {
                normal = QPointF(-1, 0);  // 左边界法线
            } else if (qAbs(hitPoint.x() - boundRect.right()) < 1.0) {
                normal = QPointF(1, 0);   // 右边界法线
            } else if (qAbs(hitPoint.y() - boundRect.top()) < 1.0) {
                normal = QPointF(0, -1);  // 上边界法线
            } else if (qAbs(hitPoint.y() - boundRect.bottom()) < 1.0) {
                normal = QPointF(0, 1);   // 下边界法线
            }

            // 反射公式: R = V - 2*(V·N)*N
            qreal dot = currentDir.x() * normal.x() + currentDir.y() * normal.y();
            currentDir = currentDir - normal * (2 * dot);

            // 归一化保持方向稳定
            qreal len = std::hypot(currentDir.x(), currentDir.y());
            if (len > 0.001) {
                currentDir /= len;
            }
        }
        else if (hitType == 1) {
            // 豹豹反射（弹性碰撞）
            QPointF normal = (hitPoint - m_baobaos[hitIndex].center);
            qreal normLen = std::hypot(normal.x(), normal.y());
            if (normLen > 0.001) {
                normal /= normLen;  // 归一化
            }

            // 反射公式: R = V - 2*(V·N)*N
            qreal dot = currentDir.x() * normal.x() + currentDir.y() * normal.y();
            currentDir = currentDir - normal * (2 * dot);
        }

        // 从碰撞点稍微偏移，避免卡在碰撞点上
        currentPos = hitPoint + currentDir * 0.1;
    }

    return points;
}

void GameWidget::resetBaoBaoState(int index)
{
    if (index < 0 || index >= m_baobaos.size()) return;

    auto &bao = m_baobaos[index];

    // 根据阵营和索引重置初始位置
        if (index == 0) bao.center = QPointF(300, 450);
        else if (index == 1) bao.center = QPointF(500, 450);
        else if (index == 2) bao.center = QPointF(700, 450);
        else if (index == 3) bao.center = QPointF(900, 450);
        else if (index == 4) bao.center = QPointF(1100, 450);
        else if (index == 5) bao.center = QPointF(1300, 450);


    // 重置物理状态
    bao.velocityF = QPointF(0, 0);
    bao.remainingDistance = 0;
    bao.isMoving = false;
    bao.justCollided = false;
    bao.collisionCount = 0;
    bao.decorationRotation = 0;
    // 重置血量和攻击力（根据豹豹类型设置）
    // 这里需要根据你的实际配置来设置
    switch(index) {
    case 0:  // 天使海豹（左）
    case 3:  // 天使海豹（右）
        bao.hp = 45;
        bao.atk = 5;
        break;
    case 1:  // 橡胶海豹（左）
    case 4:  // 橡胶海豹（右）
        bao.hp = 45;
        bao.atk = 5;
        break;
    case 2:  // 大力海豹（左）
    case 5:  // 大力海豹（右）
        bao.hp = 45;
        bao.atk = 5;
        break;
    }

    // 更新碰撞箱位置
    bao.collisionRect.moveCenter(bao.center.toPoint());

    // 更新显示标签
    if (bao.label) {
        bao.label->move(bao.collisionRect.x(), bao.collisionRect.y());
    }

    // 更新血量显示
    if (bao.hpLabel) {
        bao.hpLabel->setText(QString("%1").arg(bao.hp));
        bao.hpLabel->adjustSize();
        bao.hpLabel->move(bao.collisionRect.x() + 10, bao.collisionRect.y() + 85);
    }

    // 更新攻击力显示
    if (bao.atkLabel) {
        bao.atkLabel->setText(QString("攻击力: %1").arg(bao.atk));
        bao.atkLabel->adjustSize();
        bao.atkLabel->move(bao.collisionRect.x() + 10, bao.collisionRect.y() + 110);
    }

    for(int i=0; i<index; i++)
    {
        collisionDetection(m_baobaos[index], m_baobaos[i]);
    }

    for(int i=index+1; i<6; i++)
    {
        collisionDetection(m_baobaos[index], m_baobaos[i]);
    }

}


bool GameWidget::collisionDetection(BaoBaoObject &baoA ,BaoBaoObject &baoB)
{
    // 计算两个圆心之间的距离
    QPointF delta = baoA.center - baoB.center;
    qreal distance = std::sqrt(delta.x() * delta.x() + delta.y() * delta.y());
    qreal minDistance = 80.0;  // 半径之和 (40 + 40)

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


void GameWidget::drawRotatingDecoration(QPainter& painter, BaoBaoObject& bao) {
    painter.save();
    QPointF center = bao.center;
    qreal innerRadius = 45;
    qreal outerRadius = 55;
    qreal ringWidth = outerRadius - innerRadius;
    // 移动到豹豹中心并旋转
    painter.translate(center);
    painter.rotate(bao.decorationRotation);
    painter.translate(-center);
    // ========== 绘制两个半圆环 ==========
    QColor ringColor(0, 255, 0, 150);
    QPen ringPen(ringColor);
    ringPen.setWidth(ringWidth);
    ringPen.setCapStyle(Qt::FlatCap);
    painter.setPen(ringPen);
    painter.setBrush(Qt::NoBrush);
    QRectF arcRect(center.x() - outerRadius, center.y() - outerRadius,
                   outerRadius * 2, outerRadius * 2);
    // 第一个圆环：10° - 170°
    int start1 = 10 * 16;
    int span1 = 160 * 16;
    painter.drawArc(arcRect, start1, span1);
    // 第二个圆环：190° - 350°
    int start2 = 190 * 16;
    int span2 = 160 * 16;
    painter.drawArc(arcRect, start2, span2);
    // ========== 绘制四个箭头 ==========
    QPen arrowPen(QColor(0, 220, 0, 200));
    arrowPen.setWidth(3);
    painter.setPen(arrowPen);
    painter.setBrush(QColor(0, 220, 0, 180));
    auto drawArrow = [&](qreal angleDeg) {
        qreal rad = qDegreesToRadians(angleDeg);
        QPointF arrowBase = center + QPointF(qCos(rad) * outerRadius,
                                             qSin(rad) * outerRadius);
        qreal tangentRad = rad + M_PI / 2;
        QPointF tangentDir(qCos(tangentRad), qSin(tangentRad));
        qreal arrowLen = 12;
        QPointF arrowTip = arrowBase + tangentDir * arrowLen;
        painter.drawLine(arrowBase, arrowTip);
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
    drawArrow(170);
    drawArrow(350);
    painter.restore();
}
