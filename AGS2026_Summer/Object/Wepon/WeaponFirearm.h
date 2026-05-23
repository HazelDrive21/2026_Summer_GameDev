#pragma once
#include "WeaponBase.h"
#include "Bullet.h"
#include "../FCS.h"

class WeaponFirearm : public WeaponBase
{
public:

	/// <summary>
	/// 
	/// </summary>
	/// <param name="name">武器の名前</param>
	/// <param name="maxAmmo">最大弾数</param>
	/// <param name="reloadFrame">リロードにかかるフレーム数</param>
	/// <param name="bulletSpeed">弾の速度</param>
	/// <param name="damage">弾のダメージ</param>
	/// <param name="lifeFrame">弾の寿命（フレーム数）</param>
	WeaponFirearm(const std::string& name, int maxAmmo, int reloadFrame, float bulletSpeed, int damage, int lifeFrame)
		: WeaponBase(name, maxAmmo, reloadFrame)
		, bulletSpeed_(bulletSpeed)
		, damage_(damage)
		, bulletLifeFrame_(lifeFrame) {
	}

	virtual ~WeaponFirearm(void) override = default;

	// 弾速を取得するゲッターを1つ追加しておくと便利です
	float GetBulletSpeed(void) const override { return bulletSpeed_; }

	virtual void Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList) override
	{
		if (!IsReady()) return;

		// 1. 銃口から指定されたターゲット座標への方向ベクトルを計算
		VECTOR fireDir = VSub(targetPos, muzzlePos);
		fireDir = VNorm(fireDir);

		// 2. 弾の速度ベクトルを決定（方向 × 弾速）
		VECTOR bulletVel = VScale(fireDir, bulletSpeed_);

		// 3. 弾を生成してリストに追加
		Bullet* newBullet = new Bullet(muzzlePos, bulletVel, damage_, bulletLifeFrame_);
		bulletList.push_back(newBullet);

		currentAmmo_--;
		reloadTimer_ = reloadFrame_;
	}

private:
	float bulletSpeed_;     // 弾速
	int damage_;            // 1発あたりの威力
	int bulletLifeFrame_;   // 弾が消えるまでのフレーム数（射程距離に関係）
};