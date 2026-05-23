#pragma once
#include <DxLib.h>

class Bullet
{
public:
	// コンストラクタ（初期座標、速度ベクトル、攻撃力、射程寿命）
	Bullet(const VECTOR& pos, const VECTOR& velocity, int damage, int lifeFrame);
	~Bullet(void) = default;

	void Update(void);
	void Draw(void);

	// 寿命が尽きた、または何かに当たったらtrueを返す
	bool IsDead(void) const { return isDead_; }
	void SetDead(void) { isDead_ = true; }

	// 当たり判定用の座標や攻撃力を取得するゲッター（後で使います）
	VECTOR GetPos(void) const { return pos_; }
	int GetDamage(void) const { return damage_; }

private:
	VECTOR pos_;         // 現在の3D座標
	VECTOR velocity_;    // 速度ベクトル（1フレームの移動量）
	int damage_;         // 攻撃力
	int lifeTimer_;      // 残り寿命（フレーム数）
	bool isDead_ = false;// 消滅フラグ
};