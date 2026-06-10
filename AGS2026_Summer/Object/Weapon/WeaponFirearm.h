#pragma once
#include "WeaponBase.h"
#include "Bullet.h"
#include "../FCS.h"
#include "../../Utility/AsoUtility.h"
#include "../../Audio/AudioManager.h"

class WeaponFirearm : public WeaponBase
{
public:

	/// <summary>
	/// 通常射撃武器のコンストラクタ
	/// </summary>
	/// <param name="name">武器の名前</param>
	/// <param name="maxAmmo">最大弾数</param>
	/// <param name="reloadFrame">リロードにかかるフレーム数</param>
	/// <param name="bulletSpeed">弾の速度</param>
	/// <param name="damage">弾のダメージ</param>
	/// <param name="range">弾の射程距離</param>
	/// <param name="bulletRadius">弾の半径（初期値: 2.0f）</param>
	/// <param name="bulletColor">弾の色（初期値: 0 = 通常の黄色）</param>
	WeaponFirearm(
		const std::string& name,
		int maxAmmo,
		int reloadFrame,
		float bulletSpeed,
		int damage,
		float range,
		float bulletRadius = 2.0f,
		unsigned int bulletColor = 0,
		FCS::SITE_TYPE siteType = FCS::SITE_TYPE::STANDARD,
		int consumeEN = 0,
		int weight = 0,
		float explosionRadius = 0.0f,  // ⚡ 追加
		int explosionDamage = 0        // ⚡ 追加
	) : WeaponBase(name, maxAmmo, reloadFrame, range, siteType, consumeEN, weight)
		, bulletSpeed_(bulletSpeed)
		, damage_(damage)
		, bulletRadius_(bulletRadius)
		, bulletColor_(bulletColor)
		, consumeEN_(consumeEN)
		, explosionRadius_(explosionRadius) // ⚡ 保持
		, explosionDamage_(explosionDamage) // ⚡ 保持
	{
		// ⚡ 【超重要】弾の「寿命フレーム」は、射程距離と弾速から逆算して保持する！
		// 距離 ÷ 速度 ＝ 必要フレーム数
		if (bulletSpeed_ > 0.0f)
		{
			bulletLifeFrame_ = static_cast<int>(range / bulletSpeed_);
		}
		else
		{
			bulletLifeFrame_ = 180; // 弾速ゼロの安全ガード（約3秒）
		}
	}

	virtual ~WeaponFirearm(void) override = default;

	float GetBulletSpeed(void) const override { return bulletSpeed_; }
	int GetConsumeEN(void) const override { return consumeEN_; }

	virtual void Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList, bool isEnemy = false) override
	{
		if (!IsReady()) return;

		if (!isEnemy) {
			AudioManager::GetInstance()->PlaySE(SoundID::SE_BULLET);
		}

		VECTOR toTarget = VSub(targetPos, muzzlePos);
		VECTOR bulletVelocity = VScale(AsoUtility::VNormalize(toTarget), bulletSpeed_);

		// ⚡ 弾を生成する際、登録された爆発パラメータを一緒に引き渡す！
		Bullet* newBullet = new Bullet(
			muzzlePos, bulletVelocity, damage_, bulletLifeFrame_,
			isEnemy, bulletRadius_, bulletColor_,
			explosionRadius_, explosionDamage_
		);
		bulletList.push_back(newBullet);

		currentAmmo_--;
		ResetReloadTimer();
	}

private:
	float bulletSpeed_;
	int damage_;
	int bulletLifeFrame_;
	float bulletRadius_;
	unsigned int bulletColor_;
	int consumeEN_ = 0; // EN消費量（通常射撃は0）
	float explosionRadius_;
	int explosionDamage_;
};