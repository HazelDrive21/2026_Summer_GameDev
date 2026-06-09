#pragma once
#include "../../Object/Weapon/WeaponBase.h"
#include "../../Utility/AsoUtility.h"
#include "EnemyMissileBullet.h"
#include <string>
#include <vector>

class WeaponEnemyMissile : public WeaponBase
{
public:
	// コンストラクタ（威力の damage と 寿命の lifeFrame を追加）
	WeaponEnemyMissile(const std::string& name, int maxAmmo, int reloadFrame, float bulletSpeed, int damage, int lifeFrame)
		// ⚡ 基底クラスに必要な引数をすべて渡す！
		// 射程距離は「弾速 × 寿命フレーム」で自動計算。サイトタイプは敵なのでひとまず STANDARD でOK。
		: WeaponBase(name, maxAmmo, reloadFrame, bulletSpeed* lifeFrame, FCS::SITE_TYPE::STANDARD)
		, bulletSpeed_(bulletSpeed)
		, damage_(damage)
		, lifeFrame_(lifeFrame)
	{
		// ⚡ 敵の武器なので、ここで確定でフラグをONにしておく
		SetEnemyWeapon(true);
	}

	virtual ~WeaponEnemyMissile(void) override = default;

	float GetBulletSpeed(void) const override { return bulletSpeed_; }

	virtual void Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList, bool isEnemy = false) override
	{
		// ターゲット（プレイヤー）への方向を計算
		VECTOR toTarget = VSub(targetPos, muzzlePos);
		if (VSize(toTarget) > 0.0f)
		{
			toTarget = AsoUtility::VNormalize(toTarget);
		}
		else
		{
			toTarget = VGet(0.0f, 0.0f, 1.0f);
		}

		// 本家ACらしさを出すため、初期方向ベクトルを斜め上(Y+)に引き上げる
		float randomSpreadX = ((rand() % 100) - 50) / 150.0f;
		VECTOR launchDir = VAdd(toTarget, VGet(randomSpreadX, 1.2f, -0.2f));
		launchDir = AsoUtility::VNormalize(launchDir);

		// 初期速度ベクトルを確定
		VECTOR initialVelocity = VScale(launchDir, bulletSpeed_);

		// 敵用の誘導弾インスタンスを生成してリストへ登録
		unsigned int missileColor = GetColor(255, 90, 30);
		Bullet* newMissile = new EnemyMissileBullet(muzzlePos, initialVelocity, damage_, lifeFrame_, isEnemy, bulletSpeed_, missileColor);

		bulletList.push_back(newMissile);

		ConsumeAmmo();
	}

private:
	float bulletSpeed_;
	int damage_;
	int lifeFrame_;
};