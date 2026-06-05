#pragma once
#include "../../Object/Weapon/Bullet.h"

class EnemyMissileBullet : public Bullet
{
public:
	EnemyMissileBullet(const VECTOR& pos, const VECTOR& velocity, int damage, int lifeFrame, bool isEnemyBullet, float maxSpeed, unsigned int color);
	virtual ~EnemyMissileBullet(void) override = default;

	// 更新処理をオーバーライド
	virtual void Update(int stageModelHandle) override;
	bool IsMissile(void) const override { return true; }

private:
	float maxSpeed_;           // ミサイルの巡航速度
	int homingDelayTimer_;     // 発射直後、誘導を開始するまでの猶予フレーム（直上へ撃ち上げる演出用）
	float turnSpeed_;          // 誘導の強さ（旋回性能）
};