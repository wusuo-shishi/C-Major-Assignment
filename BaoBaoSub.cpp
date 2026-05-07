#include "BaoBaoSub.h"

TianshiBao::TianshiBao()
{
    m_name = "天使海豹";
    m_hp = 45;
    m_atk = 6;
    m_skillDesc = "按摩擒拿手：行动中碰到的己方海豹回复天使海豹攻击力相应的生命值";
}

void TianshiBao::skill()
{
}

void TianshiBao::move()
{
}

XiangjiaoBao::XiangjiaoBao()
{
    m_name = "橡胶海豹";
    m_hp = 40;
    m_atk = 5;
    m_skillDesc = "弹簧助推器：行动中，每次碰撞后提升2点攻击力，持续至本轮结束";
}

void XiangjiaoBao::skill()
{
}

void XiangjiaoBao::move()
{
}

daliBao::daliBao()
{
    m_name = "大力海豹";
    m_hp = 35;
    m_atk = 6;
    m_skillDesc = "友情接力棒：行动中碰到的己方小海豹提升3点攻击力，持续至本轮结束";
}

void daliBao::skill()
{
}

void daliBao::move()
{
}

