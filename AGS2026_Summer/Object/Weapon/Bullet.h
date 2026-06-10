#pragma once
#include <DxLib.h>

class Bullet
{
public:
	// コンストラクタ（初期座標、速度ベクトル、攻撃力、射程寿命、敵弾フラグ）
	Bullet(const VECTOR& pos, const VECTOR& velocity, int damage, int lifeFrame, bool isEnemyBullet, float radius, unsigned int color, float explosionRadius = 0.0f, int explosionDamage = 0);
	virtual~Bullet(void) = default;

	virtual void Update(int stageModelHandle);
	virtual void Draw(void);

	// 寿命が尽きた、または何かに当たったらtrueを返す
	bool IsDead(void) const { return isDead_; }
	void SetDead(void) { isDead_ = true; }

	// 当たり判定用の座標や攻撃力を取得するゲッター（後で使います）
	VECTOR GetPrevPos(void) const { return prevPos_; }
	VECTOR GetPos(void) const { return pos_; }
	int GetDamage(void) const { return damage_; }
	float GetRadius(void) const { return radius_; }
	float GetExplosionRadius(void) const { return explosionRadius_; }
	int GetExplosionDamage(void) const { return explosionDamage_; }

	bool IsEnemyBullet(void) const { return isEnemyBullet_; }
	virtual bool IsMissile(void) const { return false; }

protected:
	VECTOR pos_;         // 現在の3D座標
	VECTOR prevPos_;     // 1フレーム前の3D座標
	VECTOR velocity_;    // 速度ベクトル（1フレームの移動量）
	int damage_;         // 攻撃力
	int lifeTimer_;      // 残り寿命（フレーム数）
	bool isDead_ = false;// 消滅フラグ
	bool isEnemyBullet_ = false; // 敵の弾ならtrue、プレイヤーの弾ならfalse
	float radius_;
	unsigned int color_;
	float explosionRadius_;
	int explosionDamage_;
};