#pragma once
#include <DxLib.h>
#include <string>
#include <vector>
#include "../FCS.h"

// 装備スロットの定義
enum class EquipSlot
{
	R_ARM,  // 右手
	L_ARM,  // 左手
	R_BACK, // 右肩
	L_BACK  // 左肩
};

class FCS;
class Bullet;

class WeaponBase
{
public:
	WeaponBase(const std::string& name, int maxAmmo, int reloadFrame, float range, FCS::SITE_TYPE siteType)
		: name_(name)
		, maxAmmo_(maxAmmo)
		, currentAmmo_(maxAmmo)
		, reloadFrame_(reloadFrame)
		, reloadTimer_(0)
		, range_(range)       // ⚡ ここで確実に射程を初期化！
		, siteType_(siteType)
		, isEnemyWeapon_(false)
	{
	}
	virtual ~WeaponBase(void) = default;

	void SetEnemyWeapon(bool isEnemy) { isEnemyWeapon_ = isEnemy; }
	bool IsEnemyWeapon(void) const { return isEnemyWeapon_; }

	// 毎フレームの更新（リロードタイマーのカウントダウンなど）
	virtual void Update(void)
	{
		if (reloadTimer_ > 0) { reloadTimer_--; }
	}

	void ResetReloadTimer(void)
	{
		reloadTimer_ = reloadFrame_;
	}

	// ★最重要：武器を使用する（派生クラスで中身を書き換える）
	// 銃口の位置、FCSへの参照、プレイヤーの座標などを渡せるようにしておく
	virtual void Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList, bool isEnemy = false) = 0;

	// ゲッター群
	std::string GetName(void) const { return name_; }
	int GetCurrentAmmo(void) const { return currentAmmo_; }
	virtual bool IsReady(void) const { return reloadTimer_ <= 0 && currentAmmo_ > 0; }
	virtual float GetBulletSpeed(void) const { return 0.0f; }
	int GetMaxAmmo(void) const { return maxAmmo_; }
	float GetRange(void) const { return range_; }
	FCS::SITE_TYPE GetSiteType(void) const { return siteType_; }

protected:
	std::string name_;     // 武器名
	int maxAmmo_;          // 最大弾数
	int currentAmmo_;      // 現在の弾数
	int reloadFrame_;      // 発射間隔（リロードに必要なフレーム数）
	int reloadTimer_ = 0;  // リロード用タイマー
	bool isEnemyWeapon_ = false; // 敵の武器ならtrue、プレイヤーの武器ならfalse
	float range_ = 0.0f;  // 射程距離
	FCS::SITE_TYPE siteType_; // FCSのサイトタイプ

	void ConsumeAmmo(void)
	{
		if (currentAmmo_ > 0)
		{
			currentAmmo_--;
			reloadTimer_ = reloadFrame_;
		}
	}

	void ConsumeAmmoOnly(void)
	{
		if (currentAmmo_ > 0)
		{
			currentAmmo_--;
		}
	}
};