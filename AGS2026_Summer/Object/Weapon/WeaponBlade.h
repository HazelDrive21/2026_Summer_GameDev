#pragma once
#include "WeaponBase.h"

class WeaponBlade : public WeaponBase
{
public:
    WeaponBlade(const std::string& name, int reloadFrame, int damage, int activeFrame, float bladeRange = 40.0f);

    // ブレード用Fire：bulletListには判定用の「剣撃エフェクト弾」的なものを入れるか、
    // ここで直接プレイヤーの攻撃判定リストを操作してもOKです。
    virtual void Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList, bool isEnemy = false) override;

    virtual void Update(void) override;

private:
    int damage_;
    int activeFrame_;    // 剣を振っている最中の持続時間
    int swingTimer_ = 0; // 現在の振りの進行状態
};