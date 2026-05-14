#ifndef BAOBAOTYPE_H
#define BAOBAOTYPE_H

enum class BaoBaoType {
    Xiangjiao,    // 橡胶海豹：弹簧助推器 — 碰撞后+2ATK
    Tianshi,      // 天使海豹：按摩擒拿手 — 碰队友回复HP(值=ATK)
    Dali,         // 大力海豹：友情接力棒 — 碰队友+3ATK
    Hongwen,      // 红温海豹：急性高血压 — 首个敌人200%伤害+立即停下
    Bengdai,      // 绷带海豹：防御包扎 — HP<25时ATK+10（被动）
    Pengpeng      // 嘭嘭海豹：高压洒水车 — 发射时随机2个水弹各4伤
};

#endif // BAOBAOTYPE_H
