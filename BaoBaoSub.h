#ifndef BAOBAOSUB_H
#define BAOBAOSUB_H

#include <BaoBao.h>

class TianshiBao : public BaoBao
{
public:
    TianshiBao();
    void skill() override;
    void move() override;

protected:
    QString m_name = "天使海豹";
    int m_hp = 45;
    int m_atk = 6;
    QString m_skillDesc = "按摩擒拿手：行动中碰到的己方海豹回复天使海豹攻击力相应的生命值";
};

//---------------------------------------

class XiangjiaoBao : public BaoBao
{
public:
    XiangjiaoBao();
    void skill() override;
    void move() override;

protected:
    QString m_name = "橡胶海豹";
    int m_hp = 40;
    int m_atk = 5;
    QString m_skillDesc = "弹簧助推器：行动中，每次碰撞后提升2点攻击力，持续至本轮结束";
};

//---------------------------------------

class daliBao : public BaoBao
{
public:
    daliBao();
    void skill() override;
    void move() override;

protected:
    QString m_name = "大力海豹";
    int m_hp = 35;
    int m_atk = 6;
    QString m_skillDesc = "友情接力棒：行动中碰到的己方小海豹提升3点攻击力，持续至本轮结束";
};

//---------------------------------------

#endif // BAOBAOSUB_H
