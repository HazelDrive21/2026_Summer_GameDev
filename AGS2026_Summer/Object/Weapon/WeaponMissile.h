#pragma once
#include "WeaponBase.h"
#include <vector>

class EnemyBase;

class WeaponMissile : public WeaponBase
{
public:

	WeaponMissile(const std::string& name, int maxAmmo, int reloadFrame, float bulletSpeed, int damage, float range, float bulletRadius, unsigned int bulletColor, int launchIntervalFrame, int maxLockCount,
		FCS::SITE_TYPE siteType = FCS::SITE_TYPE::STANDARD)
		: WeaponBase(name, maxAmmo, reloadFrame, range, siteType)
		, bulletSpeed_(bulletSpeed)
		, damage_(damage)
		, bulletRadius_(bulletRadius)
		, bulletColor_(bulletColor)
		, launchIntervalFrame_(launchIntervalFrame)
		, launchTimer_(0)
		, maxLockCount_(maxLockCount)
	{
		// ⚡ 【追加】弾の寿命フレームを射程と弾速から逆算して保持する
		if (bulletSpeed_ > 0.0f)
		{
			bulletLifeFrame_ = static_cast<int>(range / bulletSpeed_);
		}
		else
		{
			bulletLifeFrame_ = 180; // 安全ガード
		}

		if (bulletColor_ == 0)
		{
			bulletColor_ = GetColor(255, 128, 0);
		}
	}

	virtual ~WeaponMissile(void) override = default;

	float GetBulletSpeed(void) const override { return bulletSpeed_; }
	int GetMaxLockCount(void) const { return maxLockCount_; }

	// 既存のインターフェース用
	virtual void Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList, bool isEnemy = false) override;

	// ★マルチロック発射を開始するためのトリガー関数
	void StartMultiLaunch(const std::vector<EnemyBase*>& lockedEnemies, const VECTOR& muzzlePos);

	// 毎フレームの更新処理をオーバーライド（発射タイマーの更新を行うため）
	virtual void Update(void) override;

	// 単発生成用の既存関数
	void FireMissile(const VECTOR& muzzlePos, EnemyBase* targetEnemy, std::vector<Bullet*>& bulletList, bool isEnemy = false);

	bool IsLaunching(void) const { return !launchQueue_.empty(); }

	bool IsReady(void) const override
	{
		// 基底クラスの条件（タイマー0 ＆ 弾あり）を満たし、
		// かつ「発射待ちキューが空（連射中でない）」ときだけ true を返す
		return WeaponBase::IsReady() && launchQueue_.empty();
	}

private:
	float bulletSpeed_;
	int damage_;
	int bulletLifeFrame_;                 // 弾の寿命フレーム（射程と弾速から逆算して設定）
	float bulletRadius_;
	unsigned int bulletColor_;
	int maxLockCount_;                   // 最大ロック数（同時に発射できるミサイルの最大数）
	std::vector<EnemyBase*> launchQueue_; // 発射待ちの敵リスト
	int launchIntervalFrame_;            // ★この値を変更することで発射間隔を調整（3なら3フレーム毎、6なら6フレーム毎）
	int launchTimer_;                    // 次のミサイルを発射するまでの残りフレームタイマー
	VECTOR currentMuzzlePos_;            // 連射中の銃口位置（Updateで参照するため一時保持）
};