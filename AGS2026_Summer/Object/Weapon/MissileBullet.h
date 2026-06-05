#pragma once
#include "Bullet.h"

class EnemyBase;

class MissileBullet : public Bullet
{
public:
	// コンストラクタに「ロックオンした敵のポインタ」を追加で受け取る
	MissileBullet(const VECTOR& pos, const VECTOR& velocity, int damage, int lifeFrame, bool isEnemyBullet, EnemyBase* targetEnemy, float radius, unsigned int color);
	virtual ~MissileBullet(void) override = default;

	virtual void Update(int stageModelHandle) override;

private:
	EnemyBase* targetEnemy_ = nullptr; // 追尾対象
	int homingDelayTimer_ = 5;        // 発射後、何フレーム後から誘導を開始するか（例: 15フレーム）
	float turnSpeed_ = 0.5f;          // 誘導の強さ（0.0〜1.0: 大きいほど急旋回する）
	float maxSpeed_ = 0.0f;            // ミサイルの巡航速度
};