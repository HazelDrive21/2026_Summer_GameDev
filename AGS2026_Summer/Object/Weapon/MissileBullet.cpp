#include "MissileBullet.h"
#include "../Enemy/EnemyBase.h" // 敵の座標や死亡フラグを取得するため
#include "../../Utility/AsoUtility.h" // VSizeなどのユーティリティがあれば

MissileBullet::MissileBullet(const VECTOR& pos, const VECTOR& velocity, int damage, int lifeFrame, bool isEnemyBullet, EnemyBase* targetEnemy, float radius, unsigned int color)
	: Bullet(pos, velocity, damage, lifeFrame, isEnemyBullet, radius, color)
	, targetEnemy_(targetEnemy)
{
	maxSpeed_ = VSize(velocity_);
}

void MissileBullet::Update(int stageModelHandle)
{
	// 1. 対象の敵が存在し、まだ生きている場合のみ誘導する
	if (targetEnemy_ != nullptr && !targetEnemy_->IsDead())
	{
		if (homingDelayTimer_ > 0)
		{
			// 発射直後はタイマーを減らすだけ（コンストラクタで渡されたvelocity_（上方向など）に進む）
			homingDelayTimer_--;
		}
		else
		{
			// --- 誘導ロジックの本番 ---

			// 敵の現在位置（カプセルの中心などが取れればベスト）を取得
			VECTOR targetPos = targetEnemy_->GetPos();

			// ★ここに敵のモデル中心となる高さを加算する
			// MT型などの敵であれば、一般的に 70.0f 〜 100.0f 程度が中心になります
			targetPos.y += 75.0f;

			// 現在のミサイル位置から敵への「目標方向ベクトル」を計算
			VECTOR targetDir = VSub(targetPos, pos_);
			targetDir = VNorm(targetDir); // 正規化（長さを1にする）

			// 現在の進行方向（velocity_）を正規化
			VECTOR currentDir = VNorm(velocity_);

			// ★球面線形補間（または単純な線形補間）で、現在の向きを徐々に敵の向きへ近づける
			// currentDir から targetDir へ turnSpeed_ の割合だけ傾ける
			VECTOR newDir = VAdd(currentDir, VScale(VSub(targetDir, currentDir), turnSpeed_));
			newDir = VNorm(newDir); // 補間したベクトルを再正規化

			// 新しい方向ベクトルに本来のスピードを掛け算して、速度を更新
			velocity_ = VScale(newDir, maxSpeed_);
		}
	}

	// 2. 座標更新や寿命処理は基底クラス（Bullet）の標準処理に任せる
	Bullet::Update(stageModelHandle);
}
