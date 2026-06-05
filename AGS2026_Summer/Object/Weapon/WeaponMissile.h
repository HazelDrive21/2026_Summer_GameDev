#pragma once
#include "WeaponBase.h"
#include <vector>

class EnemyBase;

class WeaponMissile : public WeaponBase
{
public:

	WeaponMissile(const std::string& name, int maxAmmo, int reloadFrame, float bulletSpeed, int damage, int lifeFrame, float bulletRadius = 5.0f, unsigned int bulletColor = 0, int launchIntervalFrame = 4)
		: WeaponBase(name, maxAmmo, reloadFrame)
		, bulletSpeed_(bulletSpeed)
		, damage_(damage)
		, bulletLifeFrame_(lifeFrame)
		, bulletRadius_(bulletRadius)
		, bulletColor_(bulletColor)
		, launchIntervalFrame_(launchIntervalFrame) // ★発射間隔を初期化
		, launchTimer_(0)
	{
		if (bulletColor_ == 0)
		{
			bulletColor_ = GetColor(255, 128, 0);
		}
	}

	virtual ~WeaponMissile(void) override = default;

	float GetBulletSpeed(void) const override { return bulletSpeed_; }

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
	int bulletLifeFrame_;
	float bulletRadius_;
	unsigned int bulletColor_;
	std::vector<EnemyBase*> launchQueue_; // 発射待ちの敵リスト
	int launchIntervalFrame_;            // ★この値を変更することで発射間隔を調整（3なら3フレーム毎、6なら6フレーム毎）
	int launchTimer_;                    // 次のミサイルを発射するまでの残りフレームタイマー
	VECTOR currentMuzzlePos_;            // 連射中の銃口位置（Updateで参照するため一時保持）
};