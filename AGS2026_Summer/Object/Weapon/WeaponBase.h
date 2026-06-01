#pragma once
#include <DxLib.h>
#include <string>
#include <vector>

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
	WeaponBase(const std::string& name, int maxAmmo, int reloadFrame)
		: name_(name), maxAmmo_(maxAmmo), currentAmmo_(maxAmmo), reloadFrame_(reloadFrame) {
	}
	virtual ~WeaponBase(void) = default;

	void SetEnemyWeapon(bool isEnemy) { isEnemyWeapon_ = isEnemy; }
	bool IsEnemyWeapon(void) const { return isEnemyWeapon_; }

	// 毎フレームの更新（リロードタイマーのカウントダウンなど）
	virtual void Update(void)
	{
		if (reloadTimer_ > 0) { reloadTimer_--; }
	}

	// ★最重要：武器を使用する（派生クラスで中身を書き換える）
	// 銃口の位置、FCSへの参照、プレイヤーの座標などを渡せるようにしておく
	virtual void Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList, bool isEnemy = false) = 0;

	// ゲッター群
	std::string GetName(void) const { return name_; }
	int GetCurrentAmmo(void) const { return currentAmmo_; }
	bool IsReady(void) const { return reloadTimer_ <= 0 && currentAmmo_ > 0; }
	virtual float GetBulletSpeed(void) const { return 0.0f; }
	int GetMaxAmmo(void) const { return maxAmmo_; }

protected:
	std::string name_;     // 武器名
	int maxAmmo_;          // 最大弾数
	int currentAmmo_;      // 現在の弾数
	int reloadFrame_;      // 発射間隔（リロードに必要なフレーム数）
	int reloadTimer_ = 0;  // リロード用タイマー
	bool isEnemyWeapon_ = false; // 敵の武器ならtrue、プレイヤーの武器ならfalse
};