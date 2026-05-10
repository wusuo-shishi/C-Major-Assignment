#include "GameWidget.h"
#include <QtMath>
#include <QDebug>
#include <QTimerEvent>
#include <QPainterPath>
#include <QPolygonF>
#include <cmath>

struct BaoBaoStats {
    int hp;
    int atk;
};

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

QString getBaoBaoImagePath(BaoBaoType type) {
    switch (type) {
    case BaoBaoType::Xiangjiao:
        return "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\xiangjiao_baobao.jpg";
    case BaoBaoType::Tianshi:
        return "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\tianshi_baobao.jpg";
    case BaoBaoType::Dali:
        return "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\dali_baobao.jpg";
    case BaoBaoType::Hongwen:
        return "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\hongwen_baobao.jpg";
    case BaoBaoType::Bengdai:
        return "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\bengdai_baobao.jpg";
    case BaoBaoType::Pengpeng:
        return "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\pengpeng_baobao.jpg";
    default:
        return "D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\xiangjiao_baobao.jpg";
    }
}

GameWidget::GameWidget(QWidget *parent)
    : QWidget{parent}
{
    setMouseTracking(true);  // 开启鼠标跟踪，才能接收move事件

    m_bgPixmap = QPixmap("D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\game_background.jpeg");
    m_bgPixmap = m_bgPixmap.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    QLabel *label = new QLabel("游戏界面", this);
    label->setAlignment(Qt::AlignCenter);
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(label);

    // 初始化梯形碰撞区域（顺时针四个顶点）
    m_boundaryVerts[0] = QPointF(24.48 / 100.0 * 1600, 20.65 / 100.0 * 900);   // 左上角
    m_boundaryVerts[1] = QPointF(74.95 / 100.0 * 1600, 21.57 / 100.0 * 900);   // 右上角
    m_boundaryVerts[2] = QPointF(81.72 / 100.0 * 1600, 86.11 / 100.0 * 900);   // 右下角
    m_boundaryVerts[3] = QPointF(17.97 / 100.0 * 1600, 85.83 / 100.0 * 900);   // 左下角


    // 初始化6个豹豹
    initBaobaos();

    // 预裁剪头像并创建标签
    initTouxiang();

    // 初始化SL得分标签
    initSlLabels();

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


        // 位置：百分比坐标
        QPointF defaultPositions[6] = {
            QPointF(70.99 / 100.0 * 1600, 32.96 / 100.0 * 900),   // 豹豹1
            QPointF(66.56 / 100.0 * 1600, 48.33 / 100.0 * 900),   // 豹豹2
            QPointF(73.44 / 100.0 * 1600, 65.28 / 100.0 * 900),   // 豹豹3
            QPointF(29.64 / 100.0 * 1600, 33.15 / 100.0 * 900),   // 豹豹4
            QPointF(34.22 / 100.0 * 1600, 48.15 / 100.0 * 900),   // 豹豹5
            QPointF(27.03 / 100.0 * 1600, 65.28 / 100.0 * 900),   // 豹豹6
        };
        bao[i].center = defaultPositions[i];
        bao[i].collisionRect = QRect(qRound(bao[i].center.x() - 30), qRound(bao[i].center.y() - 30), 60, 60);
        bao[i].velocityF = QPointF(0, 0);
        bao[i].decorationRotation = 0;
        bao[i].remainingDistance = 0;

        // 创建圆形图片
        QPixmap circlePixmap(60, 60);
        circlePixmap.fill(Qt::transparent);
        QPainter painter(&circlePixmap);
        painter.setRenderHint(QPainter::Antialiasing);
        QPainterPath path;
        path.addEllipse(0, 0, 60, 60);
        painter.setClipPath(path);
        painter.drawPixmap(0, 0, 60, 60, originalPixmap);
        painter.end();

        // 创建标签
        // ========== 血量显示标签 ==========
        bao[i].hpLabel = new QLabel(this);
        bao[i].hpLabel->setAlignment(Qt::AlignCenter);
        bao[i].hpLabel->show();

        // ========== 攻击力显示标签 ==========
        bao[i].atkLabel = new QLabel(this);
        bao[i].atkLabel->setAlignment(Qt::AlignCenter);
        bao[i].atkLabel->show();

        //贴图显示标签
        bao[i].label = new QLabel(this);
        bao[i].label->setPixmap(circlePixmap);
        bao[i].label->setFixedSize(60, 60);
        bao[i].label->move(bao[i].collisionRect.x(), bao[i].collisionRect.y());
        bao[i].label->setAttribute(Qt::WA_TranslucentBackground);
        bao[i].label->show();

        m_baobaos.append(bao[i]);
        refreshBaoBaoLabels(i);
    }
}

void GameWidget::initTouxiang()
{
    qreal ax = 23.80 / 100.0 * 1600;
    qreal ay = 7.59 / 100.0 * 900;
    qreal dx = 30.21 / 100.0 * 1600;
    qreal dy = 16.76 / 100.0 * 900;
    qreal bx = 30.57 / 100.0 * 1600;
    qreal X = bx - ax;
    int xi = qRound(X);
    qreal tw = dx - ax;
    qreal th = dy - ay;
    int txi = qRound(ax);
    int tyi = qRound(ay);
    int twi = qRound(tw);
    int thi = qRound(th);

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

    QPixmap baoFull("D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\baobao_touxiang.jpeg");
    baoFull = baoFull.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
    QPixmap bgFull("D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\game_background.jpeg");
    bgFull = bgFull.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    m_touxiangPixmaps[0] = baoFull.copy(txi, tyi, twi, thi);
    m_touxiangPixmaps[1] = baoFull.copy(txi + xi, tyi, twi, thi);
    m_touxiangPixmaps[2] = baoFull.copy(txi + 2 * xi, tyi, twi, thi);
    m_touxiangPixmaps[3] = bgFull.copy(txi, tyi, twi, thi);
    m_touxiangPixmaps[4] = bgFull.copy(txi + xi, tyi, twi, thi);
    m_touxiangPixmaps[5] = bgFull.copy(txi + 2 * xi, tyi, twi, thi);

    m_touxiangPixmaps2[0] = baoFull.copy(yxi, yyi, ywi, yhi);
    m_touxiangPixmaps2[1] = baoFull.copy(yxi + xi, yyi, ywi, yhi);
    m_touxiangPixmaps2[2] = baoFull.copy(yxi + 2 * xi, yyi, ywi, yhi);
    m_touxiangPixmaps2[3] = bgFull.copy(yxi, yyi, ywi, yhi);
    m_touxiangPixmaps2[4] = bgFull.copy(yxi + xi, yyi, ywi, yhi);
    m_touxiangPixmaps2[5] = bgFull.copy(yxi + 2 * xi, yyi, ywi, yhi);

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

// 初始化SL得分标签
void GameWidget::initSlLabels()
{
    // 加载得分图并缩放至游戏界面尺寸
    QPixmap defenFull("D:\\MyCode\\QtCreator\\First_Major_Assignment\\images\\defen.jpeg");
    defenFull = defenFull.scaled(1600, 900, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);

    // A点(28.96%, 5.19%)和G点(30.78%, 1.94%)为对顶角，矩形四边水平或竖直
    qreal ax = 28.96 / 100.0 * 1600;
    qreal ay = 5.19 / 100.0 * 900;
    qreal gx = 30.78 / 100.0 * 1600;
    qreal gy = 1.94 / 100.0 * 900;

    qreal slX = qMin(ax, gx);
    qreal slY = qMin(ay, gy);
    qreal slW = qAbs(gx - ax);
    qreal slH = qAbs(gy - ay);

    int slXi = qRound(slX);   // 463
    int slYi = qRound(slY);   // 17
    int slWi = qRound(slW);   // 29
    int slHi = qRound(slH);   // 29

    // 裁剪SL标签图像
    m_slPixmap = defenFull.copy(slXi, slYi, slWi, slHi);

    // 计算ABCDEF相邻点平均距离R
    // A:28.96 B:31.04 C:33.23 D:35.31 E:37.34 F:39.43 (百分比坐标)
    qreal distAB = std::hypot(31.04 - 28.96, 5.19 - 5.19);
    qreal distBC = std::hypot(33.23 - 31.04, 5.00 - 5.19);
    qreal distCD = std::hypot(35.31 - 33.23, 5.00 - 5.00);
    qreal distDE = std::hypot(37.34 - 35.31, 5.28 - 5.00);
    qreal distEF = std::hypot(39.43 - 37.34, 5.28 - 5.28);
    qreal R_percent = (distAB + distBC + distCD + distDE + distEF) / 5.0;  // 约2.0995%
    qreal R_px = R_percent / 100.0 * 1600;  // 约33.59px

    // 创建SL1~SL6标签，水平依次间隔R_px（左侧，从左往右排列）
    for (int i = 0; i < 6; i++) {
        m_slLabels[i] = new QLabel(this);
        m_slLabels[i]->setPixmap(m_slPixmap);
        m_slLabels[i]->setFixedSize(slWi, slHi);
        m_slLabels[i]->setScaledContents(true);
        m_slLabels[i]->move(qRound(slX + i * R_px), slYi);
        m_slLabels[i]->setStyleSheet("QLabel { border: none; background: transparent; }");
        m_slLabels[i]->hide();  // 初始全部隐藏
    }

    // 裁剪SR标签图像：从原图中轴线对称位置提取
    int srXi = 1600 - slXi - slWi;  // 关于x=800对称
    m_srPixmap = defenFull.copy(srXi, slYi, slWi, slHi);

    // 创建SR1~SR6标签，与SL关于中轴线(x=800)对称（右侧，从右往左排列）
    // SR[i]的x = 1600 - slX - slWi - i*R_px，使得SL[i]和SR[i]互为镜像
    qreal srBaseX = 1600 - slX - slWi;
    for (int i = 0; i < 6; i++) {
        m_srLabels[i] = new QLabel(this);
        m_srLabels[i]->setPixmap(m_srPixmap);
        m_srLabels[i]->setFixedSize(slWi, slHi);
        m_srLabels[i]->setScaledContents(true);
        m_srLabels[i]->move(qRound(srBaseX - i * R_px), slYi);
        m_srLabels[i]->setStyleSheet("QLabel { border: none; background: transparent; }");
        m_srLabels[i]->hide();  // 初始全部隐藏
    }
}

// 根据红色方死亡数更新SL标签显示
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

// 根据蓝色方死亡数更新SR标签显示
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
        if (distSq <= 30 * 30) {
            if (m_baobaos[i].isMoving) {
                return;
            }
            if (m_baobaos[i].hasActed) {
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
        QPointF currentPos = event->pos();
        QPointF rawVector = currentPos - m_dragStartPos;
        qreal rawDistance = qSqrt(rawVector.x() * rawVector.x() + rawVector.y() * rawVector.y());

        qreal maxDrag = 200;
        qreal minDrag = 40;

        if (rawDistance < minDrag) {
            m_rawDragVector = QPointF(0, 0);
            m_currentMousePos = m_dragStartPos;
            update();
            return;
        }

        qreal effectiveDistance = rawDistance - minDrag;
        QPointF effectiveVector = rawVector * (effectiveDistance / rawDistance);

        m_rawDragVector = effectiveVector;

        if (effectiveDistance > maxDrag) {
            qreal ratio = maxDrag / effectiveDistance;
            m_currentMousePos = m_dragStartPos + effectiveVector * ratio;
        } else {
            m_currentMousePos = m_dragStartPos + effectiveVector;
        }

        update();
    }
}

// 鼠标释放：计算弹射
void GameWidget::mouseReleaseEvent(QMouseEvent *)
{
    if (!m_isDragging || m_draggedIndex == -1) return;

    QPointF dragVector = m_rawDragVector;
    qreal dragDistance = qSqrt(dragVector.x() * dragVector.x() + dragVector.y() * dragVector.y());
    qreal maxDrag = 200;

    if (dragDistance < 1) {
        m_isDragging = false;
        m_draggedIndex = -1;
        m_rawDragVector = QPointF(0, 0);
        return;
    }

    qreal effectiveDistance = qMin(dragDistance, maxDrag);
    qreal ratio = effectiveDistance / maxDrag;
    qreal speed = ratio * 30;

    QPointF dir(-dragVector.x() / dragDistance, -dragVector.y() / dragDistance);
    QPointF velocityF = dir * speed;

    m_baobaos[m_draggedIndex].velocityF = velocityF;
    m_baobaos[m_draggedIndex].remainingDistance = effectiveDistance * effectiveDistance  / 12.5;
    m_baobaos[m_draggedIndex].isMoving = true;
    m_baobaos[m_draggedIndex].hasActed = true;

    // 嘭嘭海豹技能：高压洒水车 - 行动时，发射2个水弹攻击随机敌方海豹，每个造成4点伤害
    if (m_baobaos[m_draggedIndex].type == BaoBaoType::Pengpeng) {
        bool isRedTeam = (m_draggedIndex < 3);
        QList<int> enemyIndices;
        for (int i = 0; i < 6; i++) {
            if (i != m_draggedIndex && ((i < 3) != isRedTeam)) {
                enemyIndices.append(i);
            }
        }
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

        // 梯形边界碰撞检测
        int radius = 30;
        bool bounced = false;

        for (int e = 0; e < 4; e++) {
            QPointF v0 = m_boundaryVerts[e];
            QPointF v1 = m_boundaryVerts[(e + 1) % 4];
            QPointF edgeDir = v1 - v0;
            qreal edgeLenSq = QPointF::dotProduct(edgeDir, edgeDir);
            if (edgeLenSq < 0.001) continue;

            qreal t = QPointF::dotProduct(bao.center - v0, edgeDir) / edgeLenSq;
            QPointF closest;
            if (t <= 0) closest = v0;
            else if (t >= 1) closest = v1;
            else closest = v0 + edgeDir * t;

            QPointF delta = bao.center - closest;
            qreal dist = std::hypot(delta.x(), delta.y());

            if (dist < radius && dist > 0.001) {
                QPointF normal = delta / dist;
                bao.center = closest + normal * radius;

                qreal vn = QPointF::dotProduct(bao.velocityF, normal);
                if (vn < 0) {
                    bao.velocityF = bao.velocityF - normal * (2 * vn);
                }
                bounced = true;
            }
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

    static bool lastFrameHadMovement = false;

    if (!currentTurnMoving) {
        if (lastFrameHadMovement) {
            bool allActed = true;
            for (int i = 0; i < 6; i++) {
                if (!m_baobaos[i].hasActed) {
                    allActed = false;
                    break;
                }
            }

            if (allActed) {
                order = true;
                for (int i = 0; i < 6; i++) {
                    m_baobaos[i].hasActed = false;
                }
                for (int i = 0; i < 3; i++) {
                    m_baobaos[i].camp = order;
                }
                for (int i = 3; i < 6; i++) {
                    m_baobaos[i].camp = !order;
                }

                for (int i = 0; i < 6; i++) {
                    m_baobaos[i].atk = getBaoBaoStats(m_baobaos[i].type).atk;
                    if (m_baobaos[i].type == BaoBaoType::Bengdai) m_baobaos[i].bengdaiBuffed = false;
                    refreshBaoBaoLabels(i);
                }
            } else {
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


    // 更新所有豹豹的显示位置
    for (int i = 0; i < 6; i++) {
        auto &bao = m_baobaos[i];

        // 更新图片位置
        bao.collisionRect.moveCenter(bao.center.toPoint());
        bao.label->move(bao.collisionRect.x(), bao.collisionRect.y());

        // 刷新标签位置和颜色
        refreshBaoBaoLabels(i);
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
            qreal minDistance = 60.0;  // 半径之和 (30 + 30)

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

                // 伤害判定：只有移动方（主动碰撞方）才能造成伤害，避免静止重叠反复扣血
                if(baoA.camp == true && baoB.camp == false && baoA.isMoving)
                {
                    baoB.hp -= baoA.atk;
                }
                else if(baoB.camp == true && baoA.camp == false && baoB.isMoving)
                {
                    baoA.hp -= baoB.atk;
                }

                // ========== 技能效果处理 ==========
                // 确定主动方和被动方（用于技能判断）
                BaoBaoObject* activeSkill = nullptr;
                BaoBaoObject* passiveSkill = nullptr;
                if (baoA.isMoving && !baoB.isMoving) {
                    activeSkill = &baoA;
                    passiveSkill = &baoB;
                } else if (!baoA.isMoving && baoB.isMoving) {
                    activeSkill = &baoB;
                    passiveSkill = &baoA;
                } else if (baoA.isMoving && baoB.isMoving) {
                    qreal speedA = std::sqrt(baoA.velocityF.x() * baoA.velocityF.x() +
                                             baoA.velocityF.y() * baoA.velocityF.y());
                    qreal speedB = std::sqrt(baoB.velocityF.x() * baoB.velocityF.x() +
                                             baoB.velocityF.y() * baoB.velocityF.y());
                    if (speedA >= speedB) {
                        activeSkill = &baoA;
                        passiveSkill = &baoB;
                    } else {
                        activeSkill = &baoB;
                        passiveSkill = &baoA;
                    }
                }

                // 橡胶海豹技能：弹簧助推器 - 每次碰撞后提升2点攻击力
                if (activeSkill && activeSkill->type == BaoBaoType::Xiangjiao) {
                    activeSkill->atk += 2;
                    int askIdx = activeSkill - &m_baobaos[0];
                    refreshBaoBaoLabels(askIdx);
                }

                // 天使海豹技能：按摩擒拿手 - 碰到己方队友时回复相当于攻击力的血量
                if (activeSkill && passiveSkill && 
                    activeSkill->type == BaoBaoType::Tianshi &&
                    activeSkill->camp == passiveSkill->camp) {
                    passiveSkill->hp += activeSkill->atk;
                    int pskIdx = passiveSkill - &m_baobaos[0];
                    refreshBaoBaoLabels(pskIdx);
                }

                // 大力海豹技能：友情接力棒 - 碰到己方队友时提升3点攻击力
                if (activeSkill && passiveSkill && 
                    activeSkill->type == BaoBaoType::Dali &&
                    activeSkill->camp == passiveSkill->camp) {
                    passiveSkill->atk += 3;
                    int pskIdx2 = passiveSkill - &m_baobaos[0];
                    refreshBaoBaoLabels(pskIdx2);
                }

                // 红温海豹技能：急性高血压 - 碰撞到首个敌方海豹时造成200%攻击力的伤害，并立刻停下
                // 增加camp==true条件，确保只有当前行动方的红温海豹才能触发技能，避免被推动的敌方红温海豹误伤
                if (activeSkill && passiveSkill && 
                    activeSkill->type == BaoBaoType::Hongwen &&
                    activeSkill->camp == true &&
                    activeSkill->camp != passiveSkill->camp) {
                    passiveSkill->hp -= activeSkill->atk * 2;
                    // 强制推开，避免重叠导致后续帧反复判定
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
                if (i < 3) m_redDeaths++; else m_blueDeaths++;
                updateSlLabels();  // 更新SL得分标签
                updateSrLabels();  // 更新SR得分标签
                if (m_redDeaths >= 6) { emit goToResultWidget(false); return; }
                if (m_blueDeaths >= 6) { emit goToResultWidget(true); return; }
                GameWidget::resetBaoBaoState(i);
            }
            if(baoB.hp<1)
            {
                if (j < 3) m_redDeaths++; else m_blueDeaths++;
                updateSlLabels();  // 更新SL得分标签
                updateSrLabels();  // 更新SR得分标签
                if (m_redDeaths >= 6) { emit goToResultWidget(false); return; }
                if (m_blueDeaths >= 6) { emit goToResultWidget(true); return; }
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

    painter.drawPixmap(0, 0, m_bgPixmap);

    // 绘制梯形碰撞区域
    QPolygonF boundaryPoly;
    for (int i = 0; i < 4; i++) boundaryPoly << m_boundaryVerts[i];
    QPen rectPen(Qt::white);
    rectPen.setWidth(4);
    painter.setPen(rectPen);
    painter.setBrush(QBrush(QColor(0, 0, 255, 20)));
    painter.drawPolygon(boundaryPoly);

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
            pen.setWidth(8);
            painter.setPen(pen);
        } else {
            QPen pen(QColor(30, 144, 255));
            pen.setWidth(10);
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
            if (isCurrentTurn && !m_baobaos[i].hasActed) {
                drawRotatingDecoration(painter, m_baobaos[i]);
            }
        }
    }
}

// 射线与线段交点检测
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

    qreal radius = 30.0;
    QPointF currentPos = start + direction * (radius + 1.0);
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
    bao.isMoving = false;
    bao.justCollided = false;
    bao.collisionCount = 0;
    bao.decorationRotation = 0;
    bao.bengdaiBuffed = false;
    // 根据豹豹类型重置血量和攻击力
    BaoBaoStats stats = getBaoBaoStats(bao.type);
    bao.hp = stats.hp;
    bao.atk = stats.atk;

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


void GameWidget::drawRotatingDecoration(QPainter& painter, BaoBaoObject& bao) {
    painter.save();
    QPointF center = bao.center;
    qreal innerRadius = 35;
    qreal outerRadius = 45;
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

void GameWidget::refreshBaoBaoLabels(int index)
{
    auto& bao = m_baobaos[index];
    if (!bao.hpLabel || !bao.atkLabel) return;

    // 绷带海豹动态buff：血量低于25获得+10攻击力，高于等于25则失去
    if (bao.type == BaoBaoType::Bengdai) {
        if (bao.hp < 25 && !bao.bengdaiBuffed) {
            bao.atk += 10;
            bao.bengdaiBuffed = true;
        } else if (bao.hp >= 25 && bao.bengdaiBuffed) {
            bao.atk -= 10;
            bao.bengdaiBuffed = false;
        }
    }

    // 紧贴豹豹底部放置标签（豹豹60x60）
    bao.hpLabel->move(bao.collisionRect.x() + 8, bao.collisionRect.y() + 56);
    bao.atkLabel->move(bao.collisionRect.x() + 8, bao.collisionRect.y() + 72);

    BaoBaoStats defStats = getBaoBaoStats(bao.type);

    // 血量颜色和字号：个位数红色18px，高于默认亮绿18px，否则白色14px
    QString hpColor = "white";
    int hpFontSize = 14;
    if (bao.hp < 10) { hpColor = "#ff4444"; hpFontSize = 18; }
    else if (bao.hp > defStats.hp) { hpColor = "#00ff00"; hpFontSize = 18; }

    // 攻击力颜色和字号：高于默认亮绿16px，否则浅红12px
    QString atkColor = "white";
    int atkFontSize = 12;
    if (bao.atk > defStats.atk) { atkColor = "#00ff00"; atkFontSize = 16; }

    bao.hpLabel->setStyleSheet(
        QString("QLabel { color: %1; font-size: %2px; font-weight: bold;"
                " background-color: transparent; border-radius: 10px; padding: 2px 5px; }")
            .arg(hpColor).arg(hpFontSize));

    bao.atkLabel->setStyleSheet(
        QString("QLabel { color: %1; font-size: %2px; font-weight: bold;"
                " background-color: transparent; border-radius: 8px; padding: 2px 5px; }")
            .arg(atkColor).arg(atkFontSize));

    bao.hpLabel->setText(QString("%1").arg(bao.hp));
    bao.hpLabel->adjustSize();
    bao.atkLabel->setText(QString("攻击力: %1").arg(bao.atk));
    bao.atkLabel->adjustSize();
}

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
            m_baobaos[i].bengdaiBuffed = false;
            m_baobaos[i].center = defaultPositions[i];
            m_baobaos[i].velocityF = QPointF(0, 0);
            m_baobaos[i].remainingDistance = 0;
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
            m_baobaos[i + 3].bengdaiBuffed = false;
            m_baobaos[i + 3].center = defaultPositions[i + 3];
            m_baobaos[i + 3].velocityF = QPointF(0, 0);
            m_baobaos[i + 3].remainingDistance = 0;
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

void GameWidget::resetGame()
{
    m_redDeaths = 0;
    m_blueDeaths = 0;
    order = true;

    for (int i = 0; i < 3 && i < m_p1Types.size(); i++) {
        m_baobaos[i].type = m_p1Types[i];
        BaoBaoStats stats = getBaoBaoStats(m_p1Types[i]);
        m_baobaos[i].hp = stats.hp;
        m_baobaos[i].atk = stats.atk;
    }
    for (int i = 0; i < 3 && i < m_p2Types.size(); i++) {
        m_baobaos[i + 3].type = m_p2Types[i];
        BaoBaoStats stats = getBaoBaoStats(m_p2Types[i]);
        m_baobaos[i + 3].hp = stats.hp;
        m_baobaos[i + 3].atk = stats.atk;
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

