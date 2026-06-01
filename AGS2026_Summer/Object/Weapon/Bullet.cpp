#include "Bullet.h"

Bullet::Bullet(const VECTOR& pos, const VECTOR& velocity, int damage, int lifeFrame, bool isEnemyBullet)
	: pos_(pos)
	, velocity_(velocity)
	, damage_(damage)
	, lifeTimer_(lifeFrame)
	, isEnemyBullet_(isEnemyBullet)
{
}

void Bullet::Update(void)
{
	// 速度の分だけ座標を進める
	pos_ = VAdd(pos_, velocity_);

	// 寿命のカウントダウン
	if (lifeTimer_ > 0)
	{
		lifeTimer_--;
		if (lifeTimer_ <= 0)
		{
			isDead_ = true; // 寿命が尽きたら消滅
		}
	}
}

void Bullet::Draw(void)
{
	if (isDead_) return;

	// 仮の描画：ひとまず小さな黄色い球体を弾として描画する
	// (慣れてきたら、ここにDxLibの3Dモデルや、残像エフェクトの線を描画すると最高になります)
	DrawSphere3D(pos_, 2.0f, 8, GetColor(255, 255, 0), GetColor(255, 255, 255), TRUE);
}