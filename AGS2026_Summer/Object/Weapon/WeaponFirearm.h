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
		float range,             // ⚡ 第6引数を lifeFrame から range（射程）に変更
		float bulletRadius = 2.0f,
		unsigned int bulletColor = 0,
		FCS::SITE_TYPE siteType = FCS::SITE_TYPE::STANDARD)
		// ⚡ 基底クラスの厳格化したコンストラクタへ、射程とサイトタイプを直撃させる！
		: WeaponBase(name, maxAmmo, reloadFrame, range, siteType)
		, bulletSpeed_(bulletSpeed)
		, damage_(damage)
		, bulletRadius_(bulletRadius)
		, bulletColor_(bulletColor)
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

	virtual void Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList, bool isEnemy = false) override
	{
		if (!IsReady()) return;

		if (!isEnemy)
		{
			AudioManager::GetInstance()->PlaySE(SoundID::SE_BULLET);
		}

		// 1. 銃口から指定されたターゲット座標への方向ベクトルを計算
		VECTOR toTarget = VSub(targetPos, muzzlePos);
		VECTOR bulletVelocity = AsoUtility::VNormalize(toTarget);

		// 2. 弾速を掛けて速度ベクトルにする
		bulletVelocity = VScale(bulletVelocity, bulletSpeed_);

		// 3. 弾インスタンスを生成してリストに追加
		// ⚡ 修正：メンバ変数に保存したサイズと色を Bullet のコンストラクタに渡す
		bulletList.push_back(new Bullet(muzzlePos, bulletVelocity, damage_, bulletLifeFrame_, isEnemy, bulletRadius_, bulletColor_));

		// 弾数をデクリメント
		ConsumeAmmo();
	}

private:
	float bulletSpeed_;
	int damage_;
	int bulletLifeFrame_;

	// ⚡ 追加：弾の外見を制御するメンバ変数
	float bulletRadius_;
	unsigned int bulletColor_;
};