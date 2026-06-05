#include "../Enemy/EnemyBase.h"
#include "../../Manager/SceneManager.h"
#include "WeaponMissile.h"
#include "MissileBullet.h"

// 既存のFire（インターフェース合わせ用、ロックしていない場合は直線に飛ぶ）
void WeaponMissile::Fire(const VECTOR& muzzlePos, const VECTOR& targetPos, std::vector<Bullet*>& bulletList, bool isEnemy)
{
	return;
}

// ★ボタンが押されたときにプレイヤー等から1回だけ呼ばれるトリガー関数
void WeaponMissile::StartMultiLaunch(const std::vector<EnemyBase*>& lockedEnemies, const VECTOR& muzzlePos)
{
	// リロード中、またはロックしている敵がいないなら何もしない
	if (!IsReady() || lockedEnemies.empty()) return;

	launchQueue_ = lockedEnemies;
	currentMuzzlePos_ = muzzlePos;
	launchTimer_ = 0; // 1発目は即座に撃ちたいので 0

	// 弾数の先行消費
	for (size_t i = 0; i < launchQueue_.size(); ++i)
	{
		if (GetCurrentAmmo() > 0)
		{
			ConsumeAmmoOnly(); // リロードを発生させずに弾だけ減らす
		}
		else
		{
			launchQueue_.resize(i);
			break;
		}
	}

	// ❌ ここにあった ResetReloadTimer(); は削除します！
	// まだリロードは開始しません。
}

void WeaponMissile::Update(void)
{
	// 基底クラスの更新（リロードタイマーのカウントダウンなど）
	WeaponBase::Update();

	// 発射待ちの敵がキューに残っている場合
	if (!launchQueue_.empty())
	{
		if (launchTimer_ > 0)
		{
			launchTimer_--; // インターバル消化中
		}
		else
		{
			EnemyBase* target = launchQueue_.front();

			if (target != nullptr && !target->IsDead())
			{
				auto& bulletList = SceneManager::GetInstance().GetBulletList();
				FireMissile(currentMuzzlePos_, target, bulletList, IsEnemyWeapon());
			}

			// 発射したターゲットをキューから削除
			launchQueue_.erase(launchQueue_.begin());

			// ★【ここがポイント】先頭を削除した結果、キューが空になったか判定
			if (launchQueue_.empty())
			{
				// 全弾を撃ち尽くしたこのフレームで、初めてリロードを開始する！
				ResetReloadTimer();
				launchTimer_ = 0;
			}
			else
			{
				// まだ次のミサイルが控えているなら、発射ディレイを設定
				launchTimer_ = launchIntervalFrame_;
			}
		}
	}
}

// ★ミサイル専用の射撃関数（1体の敵に対して「1発だけ」生成するように修正）
void WeaponMissile::FireMissile(const VECTOR& muzzlePos, EnemyBase* targetEnemy, std::vector<Bullet*>& bulletList, bool isEnemy)
{
	// ガード処理: ターゲットがヌルポインタ、または既に死亡している場合は生成しない
	if (targetEnemy == nullptr || targetEnemy->IsDead()) return;

	// --- AC風の拡散（バラけ）演出 ---
	// まっすぐ上だけでなく、左右・前後に少しランダムなノイズを加える
	// （1発ずつ発射される際にも、毎回このランダム計算が走るので綺麗にバラけます）
	float noiseX = ((rand() % 100) - 50) * 0.1f; // -5.0 ～ +5.0 のブレ
	float noiseZ = ((rand() % 100) - 50) * 0.1f;

	// 上方向(Y)に強く打ち上げつつ、ノイズで拡散させる初期速度
	VECTOR launchVel = VGet(noiseX, bulletSpeed_ * 0.8f, noiseZ);

	// 誘導弾（MissileBullet）を「1発だけ」生成してリストに追加
	// ターゲットは引数で指定された targetEnemy 1体のみ
	Bullet* newMissile = new MissileBullet(muzzlePos, launchVel, damage_, bulletLifeFrame_, isEnemy, targetEnemy, bulletRadius_, bulletColor_);
	bulletList.push_back(newMissile);

	// ※【注意】弾数の消費（ConsumeAmmo()）は、StartMultiLaunch 側であらかじめ
	// まとめて減算しているため、二重消費（バグ）を防ぐためにここでは呼び出しません。
}