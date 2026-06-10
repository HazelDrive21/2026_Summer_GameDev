#include "Bullet.h"

Bullet::Bullet(const VECTOR& pos, const VECTOR& velocity, int damage, int lifeFrame, bool isEnemyBullet, float radius, unsigned int color, float explosionRadius, int explosionDamage)
	: pos_(pos)
	, prevPos_(pos)
	, velocity_(velocity)
	, damage_(damage)
	, lifeTimer_(lifeFrame)
	, isEnemyBullet_(isEnemyBullet)
	, radius_(radius)
	, explosionRadius_(explosionRadius)
	, explosionDamage_(explosionDamage)
{
	// 色が 0（未指定）の場合は、従来の通常弾の色（黄色）にする
	if (color == 0)
	{
		color_ = GetColor(255, 255, 0); // 通常弾の黄色
	}
	else
	{
		color_ = color; // 指定されたミサイル等の色
	}
}

void Bullet::Update(int stageModelHandle)
{
	if (isDead_) return;

	// ① 移動前の座標をキープしておく（線分の開始点）
	prevPos_ = pos_;

	// 速度の分だけ座標を進める（線分の終了点）
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

	// ② ステージのモデルハンドルが正しく渡されていて、まだ弾が生きていれば衝突判定
	if (!isDead_ && stageModelHandle != -1)
	{

		MV1_COLL_RESULT_POLY hitResult = MV1CollCheck_Line(stageModelHandle, -1,prevPos_, pos_);

		// HitFlag が 1 なら障害物のポリゴンに衝突している
		if (hitResult.HitFlag == 1)
		{
			isDead_ = true; // 障害物に当たったので消滅フラグを立てる
		}
	}
}

void Bullet::Draw(void)
{
	if (isDead_) return;

	// 1. 一時的にライトを有効化（これでGetColorの色が100%そのまま出ます）
	SetUseLighting(TRUE);

	// 描画処理（color_ が適用される）
	DrawSphere3D(pos_, radius_, 8, color_, GetColor(255, 255, 255), TRUE);

	// 2. 描画が終わったらライトを元の無効状態（均一な明るさ）に戻す
	SetUseLighting(FALSE);
}