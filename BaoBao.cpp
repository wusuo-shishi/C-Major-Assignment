#include "BaoBao.h"

BaoBao::BaoBao()
{

}

QString BaoBao::getName() const
{
    return m_name;
}

int BaoBao::getHp() const
{
    return m_hp;
}

int BaoBao::getAtk() const
{
    return m_atk;
}

QString BaoBao::getSkillDesc() const
{
    return m_skillDesc;
}
