#ifndef BAOBAOSUB_H
#define BAOBAOSUB_H

#include <BaoBao.h>

class TianshiBao : public BaoBao
{
public:
    TianshiBao();

protected:
    QString m_name = "天使海豹";
    int m_hp = 45;
    int m_ack = 6;
};

//---------------------------------------

class XiangjiaoBao : public BaoBao
{
public:
    XiangjiaoBao();

protected:
    QString m_name = "橡胶海豹";
    int m_hp = 40;
    int m_ack = 5;
};

//---------------------------------------

class daliBao : public BaoBao
{
public:
    daliBao();

protected:
    QString m_name = "大力海豹";
    int m_hp = 35;
    int m_ack = 6;
};

//---------------------------------------

#endif // BAOBAOSUB_H
