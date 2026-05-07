#ifndef BAOBAO_H
#define BAOBAO_H

#include <Qstring>
#include <QLabel>
#include <QPointF>
class BaoBao
{
public:
    BaoBao();
    //虚析构
    virtual ~BaoBao() = default;

    virtual void skill() = 0;    // 技能
    virtual void move() = 0;     // 移动

    bool justCollided = false;   // 防止同一帧内重复碰撞
    int collisionCount = 0;      // 记录碰撞次数（用于衰减）
    QLabel *label;           // 显示的图片
    QRect collisionRect;     // 碰撞箱（显示用）
    QPointF center;          // 圆心（浮点，物理计算用）
    QPointF velocityF;       // 速度向量
    qreal remainingDistance; // 剩余行动距离（浮点）
    bool isMoving = false;   // 是否正在移动

    QString getName() const;
    int getHp() const;
    int getAtk() const;
    QString getSkillDesc() const;
protected:
    QString m_name;          //名字
    int m_hp;               //血量
    int m_speed;            //速度
    int m_atk;              //攻击力
    QString m_skillDesc;    //技能描述
};

#endif // BAOBAO_H
