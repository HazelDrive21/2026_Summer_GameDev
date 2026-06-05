#include "EnemyMissileBullet.h"
#include "../../Manager/SceneManager.h"
#include "../../Object/Player.h"
#include "../../Utility/AsoUtility.h"

EnemyMissileBullet::EnemyMissileBullet(const VECTOR& pos, const VECTOR& velocity, int damage, int lifeFrame, bool isEnemyBullet, float maxSpeed, unsigned int color)
	: Bullet(pos, velocity, damage, lifeFrame, isEnemyBullet, 20.0f, color) // 半径をミサイル用に少し大きめ(6.0f)に設定
	, maxSpeed_(maxSpeed)
	, homingDelayTimer_(15) // 発射後15フレーム（約0.25秒）は誘導せず上昇させる
	, turnSpeed_(0.2f)    // 旋回力。この数値を上げると急カーブでプレイヤーを追いつめる強誘導になります
{
}

void EnemyMissileBullet::Update(int stageModelHandle)
{
	if (isDead_) return;

	// 1. ターゲットであるプレイヤーのインスタンスを取得
	Player* player = SceneManager::GetInstance().GetPlayer();

	if (player != nullptr && !player->IsDead())
	{
		// 発射直後の撃ち上がり演出タイマーのチェック
		if (homingDelayTimer_ > 0)
		{
			homingDelayTimer_--;
		}
		else
		{
			// --- ⭕ ホーミング（誘導）ロジックの本番 ---

			// プレイヤーの足元ではなく、胴体中心（やや高め）を狙う
			VECTOR targetPos = player->GetTransform().pos;
			targetPos.y += 75.0f;

			// ミサイルからプレイヤーへの「目標方向ベクトル」
			VECTOR targetDir = VSub(targetPos, pos_);

			if (VSize(targetDir) > 1.0f)
			{
				targetDir = AsoUtility::VNormalize(targetDir);

				// 現在の進行方向ベクトルを正規化
				VECTOR currentDir = AsoUtility::VNormalize(velocity_);

				// 現在の向きから目標の向きへ、turnSpeed_ の割合だけじわじわとブレンド（旋回）する
				VECTOR newDir = VAdd(currentDir, VScale(targetDir, turnSpeed_));
				newDir = AsoUtility::VNormalize(newDir);
				// 新しい方向に、本来のミサイル巡航速度（maxSpeed_）を掛け合わせて速度ベクトルを再設定
				velocity_ = VScale(newDir, maxSpeed_);
			}
		}
	}

	// 2. ⭕ 速度ベクトル（velocity_）の修正が完了した状態で、基底クラスの本来の更新処理を実行する
	// これにより、Bullet.cpp内に記述されている「移動」「地形との衝突判定」「寿命カウント減算」が自動的に適用されます。
	Bullet::Update(stageModelHandle);
}