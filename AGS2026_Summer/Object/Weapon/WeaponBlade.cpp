#include "WeaponBlade.h"
#include "BladeBullet.h"

WeaponBlade::WeaponBlade(const std::string& name, int reloadFrame, int damage, int activeFrame, float bladeRange)
// ⚡ 5つの引数をすべて基底クラスに手動で流し込む！
    : WeaponBase(name, 999, reloadFrame, bladeRange, FCS::SITE_TYPE::STANDARD)
    , damage_(damage)
    , activeFrame_(activeFrame)
{
}

void WeaponBlade::Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList, bool isEnemy)
{
    if (!IsReady()) return;

    // プレイヤーの正面（muzzlePos から少し進んだ位置）に判定弾を生成
    // 速度はゼロでOK（その場に判定が出るため）
    VECTOR bladePos = muzzlePos;
    VECTOR velocity = VGet(0, 0, 0);

    // 判定用の弾をリストに追加
    bulletList.push_back(new BladeBullet(bladePos, velocity, damage_));

    // リロード開始
    ResetReloadTimer();
}

void WeaponBlade::Update(void)
{
    WeaponBase::Update(); // 既存のリロードタイマー更新
}